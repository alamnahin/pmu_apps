#include "qpmu/estimator.h"
#include "qpmu/defs.h"

#include <algorithm>
#include <cmath>
#include <cassert>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>

using namespace qpmu;

template <class Numeric>
constexpr bool isPositive(Numeric x)
{
    if constexpr (std::is_integral<Numeric>::value) {
        return x > 0;
    } else {
        return x > 0 + std::numeric_limits<Numeric>::epsilon();
    }
}

constexpr Float zeroCrossingTime(Float t0, Float x0, Float t1, Float x1)
{
    return t0 + (0 - x0) * (t1 - t0) / (x1 - x0);
}

PhasorEstimator::~PhasorEstimator()
{
    if (m_phasorStrategy == PhasorEstimationStrategy::FFT) {
        for (USize i = 0; i < CountSignals; ++i) {
            FFTW<Float>::destroy_plan(m_fftwState.plans[i]);
            FFTW<Float>::free(m_fftwState.inputs[i]);
            FFTW<Float>::free(m_fftwState.outputs[i]);
        }
    }

    // The SDFT state is trivially destructible
}

PhasorEstimator::PhasorEstimator(USize fn, USize fs, PhasorEstimationStrategy phasorStrategy)
    : m_phasorStrategy(phasorStrategy)
{
    assert(fn > 0);
    assert(fs % fn == 0);

    m_estimationBuffer.resize(fs
                              / fn); // hold one full cycle (fs / fn = number of samples per cycle)
    m_sampleBuffer.resize(5 * fs); // hold at least 1 second of samples

    if (m_phasorStrategy == PhasorEstimationStrategy::FFT) {
        for (USize i = 0; i < CountSignals; ++i) {
            m_fftwState.inputs[i] = FFTW<Float>::alloc_complex(m_estimationBuffer.size());
            m_fftwState.outputs[i] = FFTW<Float>::alloc_complex(m_estimationBuffer.size());
            m_fftwState.plans[i] =
                    FFTW<Float>::plan_dft_1d(m_estimationBuffer.size(), m_fftwState.inputs[i],
                                             m_fftwState.outputs[i], FFTW_FORWARD, FFTW_ESTIMATE);
            for (USize j = 0; j < m_estimationBuffer.size(); ++j) {
                m_fftwState.inputs[i][j][0] = 0;
                m_fftwState.inputs[i][j][1] = 0;
            }
        }

    } else if (m_phasorStrategy == PhasorEstimationStrategy::SDFT) {
        m_sdftState.workers = std::vector<SdftType>(
                CountSignals, SdftType(m_estimationBuffer.size(), sdft::Window::Hann, 1));
        m_sdftState.outputs.assign(CountSignals,
                                   std::vector<Complex>(m_estimationBuffer.size(), 0));
    }
}

#define NEXT(i, x) (((i) != (m_##x##Buffer.size() - 1)) * ((i) + 1))
#define PREV(i, x) (((i) != 0) * ((i)-1) + ((i) == 0) * (m_##x##Buffer.size() - 1))

#define ESTIMATION_NEXT(i) NEXT(i, estimation)
#define ESTIMATION_PREV(i) PREV(i, estimation)

#define SAMPLE_NEXT(i) NEXT(i, sample)
#define SAMPLE_PREV(i) PREV(i, sample)

const Estimation &PhasorEstimator::lastEstimation() const
{
    return m_estimationBuffer[m_estimationBufIdx];
}

const Sample &PhasorEstimator::lastSample() const
{
    return m_sampleBuffer[m_sampleBufIdx];
}

void PhasorEstimator::updateEstimation(const Sample &sample)
{
    m_sampleBuffer[m_sampleBufIdx] = sample;

    const Estimation &prevEstimation = m_estimationBuffer[ESTIMATION_PREV(m_estimationBufIdx)];
    Estimation &currEstimation = m_estimationBuffer[m_estimationBufIdx];

    { /// Estimate phasors

        if (m_phasorStrategy == PhasorEstimationStrategy::FFT) {
            /// Shift the previous inputs
            for (USize i = 0; i < CountSignals; ++i) {
                for (USize j = 1; j < m_estimationBuffer.size(); ++j) {
                    m_fftwState.inputs[i][j - 1][0] = m_fftwState.inputs[i][j][0];
                    m_fftwState.inputs[i][j - 1][1] = m_fftwState.inputs[i][j][1];
                }
            }
            /// Add the new sample's data
            for (USize i = 0; i < CountSignals; ++i) {
                m_fftwState.inputs[i][m_estimationBuffer.size() - 1][0] = sample.channels[i];
                m_fftwState.inputs[i][m_estimationBuffer.size() - 1][1] = 0;
            }
            /// Execute the FFT plan
            for (USize i = 0; i < CountSignals; ++i) {
                FFTW<Float>::execute(m_fftwState.plans[i]);
            }
            /// Phasor = output corresponding to the fundamental frequency
            for (USize i = 0; i < CountSignals; ++i) {
                Complex phasor = { m_fftwState.outputs[i][1][0], m_fftwState.outputs[i][1][1] };
                phasor /= Float(m_estimationBuffer.size());
                currEstimation.phasors[i] = phasor;
            }

        } else if (m_phasorStrategy == PhasorEstimationStrategy::SDFT) {
            /// Run the SDFT on the new sample
            for (USize i = 0; i < CountSignals; ++i) {
                m_sdftState.workers[i].sdft(sample.channels[i], m_sdftState.outputs[i].data());
            }
            /// Phasor = output corresponding to the fundamental frequency
            for (USize i = 0; i < CountSignals; ++i) {
                const auto &phasor = m_sdftState.outputs[i][1] / Float(m_estimationBuffer.size());
                currEstimation.phasors[i] = phasor;
            }
        }
    }

    { /// Estimate frequency and ROCOF, and sampling rate

        if (m_windowStartTimeUs == 0) {
            m_windowStartTimeUs = sample.timestampUs;
            m_windowEndTimeUs = m_windowStartTimeUs + (USize)1e6;
        }

        std::copy(prevEstimation.frequencies, prevEstimation.frequencies + CountSignals,
                  currEstimation.frequencies);

        std::copy(prevEstimation.rocofs, prevEstimation.rocofs + CountSignals,
                  currEstimation.rocofs);

        currEstimation.samplingRate = prevEstimation.samplingRate;

        if (m_windowEndTimeUs <= sample.timestampUs) {
            /// The 1-second window has ended, hence
            /// - estimate
            ///   * channel frequencies,
            ///   * channel ROCOFs, and
            ///   * sampling rate
            /// - reset the window variables

            for (USize ch = 0; ch < CountSignals; ++ch) {
                { /// Frequency estimation

                    /// Get the "zero" value -- the value that is halfway between the maximum and
                    /// minimum values in the window
                    U64 maxValue = 0;
                    for (USize i = 0; i <= m_sampleBufIdx; ++i) {
                        if (m_sampleBuffer[i].channels[ch] > maxValue) {
                            maxValue = m_sampleBuffer[i].channels[ch];
                        }
                    }
                    const I64 zeroValue = maxValue / 2;
                    assert(zeroValue >= 0);

                    /// Get the zero crrossing count and the first and last zero crossing times
                    U64 countZeroCrossings = 0;
                    U64 firstCrossingUs = 0;
                    U64 lastCrossingUs = 0;
                    for (USize i = 1; i <= m_sampleBufIdx; ++i) {
                        const I64 &x0 = (I64)m_sampleBuffer[i - 1].channels[ch] - zeroValue;
                        const I64 &x1 = (I64)m_sampleBuffer[i].channels[ch] - zeroValue;
                        const U64 &t0 = m_sampleBuffer[i - 1].timestampUs;
                        const U64 &t1 = m_sampleBuffer[i].timestampUs;

                        if (isPositive(x0) != isPositive(x1)) {
                            ++countZeroCrossings;
                            auto t = (U64)(std::round(zeroCrossingTime(t0, x0, t1, x1)) + 0.2);
                            assert(t0 <= t && t <= t1);
                            if (firstCrossingUs == 0) {
                                firstCrossingUs = t;
                            }
                            lastCrossingUs = t;
                        }
                    }

                    auto crossingWindowSec = (Float)(lastCrossingUs - firstCrossingUs) * 1e-6;
                    auto residueSec = (Float)1.0 - crossingWindowSec;

                    /// 2 zero crossings per cycle + 1 crossing starts the count
                    Float freq = std::max(0.0, (Float)(countZeroCrossings - 1)) / 2.0;

                    /// Adjust the frequency to the window, because it is not exactly 1 second
                    currEstimation.frequencies[ch] = freq * (1.0 + residueSec);
                }

                { /// ROCOF estimation
                    currEstimation.rocofs[ch] =
                            (currEstimation.frequencies[ch] - prevEstimation.frequencies[ch])
                            / sample.timeDeltaUs * 1e6;
                }
            }

            { /// Sampling rate estimation
                auto samplesWindowSec = (Float)(sample.timestampUs - m_windowStartTimeUs) * 1e-6;
                auto residueSec = (Float)1.0 - samplesWindowSec;
                USize countSamples = m_sampleBufIdx + 1;

                /// Adjust the sampling rate to the window, because it is not exactly 1 second
                currEstimation.samplingRate = countSamples * (1.0 + residueSec);
            }

            { /// Reset window variables
                m_sampleBufIdx = m_sampleBuffer.size() - 1;
                m_windowStartTimeUs = sample.timestampUs;
                m_windowEndTimeUs = m_windowStartTimeUs + (USize)1e6;
            }
        }
    }

    // Update the indexes
    m_sampleBufIdx = (m_sampleBufIdx + 1) % m_sampleBuffer.size();
    m_estimationBufIdx = (m_estimationBufIdx + 1) % m_estimationBuffer.size();
}

#undef NEXT
#undef PREV

#undef ESTIMATION_NEXT
#undef ESTIMATION_PREV

#undef SAMPLE_NEXT
#undef SAMPLE_PREV