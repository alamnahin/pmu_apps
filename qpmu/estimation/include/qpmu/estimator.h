#ifndef PHASOR_ESTIMATOR_H
#define PHASOR_ESTIMATOR_H

#include <array>
#include <complex>
#include <vector>

#include <fftw3.h>
#include <sdft/sdft.h>

#include "qpmu/defs.h"

namespace qpmu {

template <typename FloatType = double>
struct FFTW
{
};

#define SPECIALIZE_FFTW(type, suffix)                                   \
  template <>                                                           \
  struct FFTW<type>                                                     \
  {                                                                     \
    using Complex = fftw##suffix##_complex;                             \
    using Plan = fftw##suffix##_plan;                                   \
    static constexpr auto alloc_complex = fftw##suffix##_alloc_complex; \
    static constexpr auto malloc = fftw##suffix##_malloc;               \
    static constexpr auto plan_dft_1d = fftw##suffix##_plan_dft_1d;     \
    static constexpr auto execute = fftw##suffix##_execute;             \
    static constexpr auto destroy_plan = fftw##suffix##_destroy_plan;   \
    static constexpr auto free = fftw##suffix##_free;                   \
  };

SPECIALIZE_FFTW(float, f)
SPECIALIZE_FFTW(double, )
SPECIALIZE_FFTW(long double, l)
#undef SPECIALIZE_FFTW

enum class PhasorEstimationStrategy {
    FFT, // Fast Fourier Transform
    SDFT, // Sliding-window Discrete Fourier Transform
};

class PhasorEstimator
{
public:
    using Float = qpmu::Float;
    using USize = qpmu::USize;
    using ISize = qpmu::ISize;
    using Complex = qpmu::Complex;
    using SdftType = sdft::SDFT<Float, Float>;
    static constexpr USize CountSignals = qpmu::CountSignals;

    struct FftwState
    {
        FFTW<Float>::Complex *inputs[CountSignals];
        FFTW<Float>::Complex *outputs[CountSignals];
        FFTW<Float>::Plan plans[CountSignals];
    };

    struct SdftState
    {
        std::vector<SdftType> workers;
        std::vector<std::vector<Complex>> outputs;
    };

    // ****** Constructors and destructors ******
    PhasorEstimator(const PhasorEstimator &) = default;
    PhasorEstimator(PhasorEstimator &&) = default;
    PhasorEstimator &operator=(const PhasorEstimator &) = default;
    PhasorEstimator &operator=(PhasorEstimator &&) = default;
    ~PhasorEstimator();
    PhasorEstimator(USize fn, USize fs,
                    PhasorEstimationStrategy phasorStrategy = PhasorEstimationStrategy::FFT);

    void updateEstimation(const qpmu::Sample &sample);

    const qpmu::Estimation &lastEstimation() const;
    const qpmu::Sample &lastSample() const;

private:
    PhasorEstimationStrategy m_phasorStrategy = PhasorEstimationStrategy::FFT;
    FftwState m_fftwState = {};
    SdftState m_sdftState = {};

    std::vector<qpmu::Estimation> m_estimationBuffer = {};
    std::vector<qpmu::Sample> m_sampleBuffer = {};
    USize m_estimationBufIdx = 0;
    USize m_sampleBufIdx = 0;

    qpmu::U64 m_windowStartTimeUs = 0;
    qpmu::U64 m_windowEndTimeUs = 0;
};

} // namespace qpmu

#endif // PHASOR_ESTIMATOR_H