#include "qpmu/defs.h"
#include "qpmu/util.h"

#include <cassert>
#include <numeric>
#include <sstream>
#include <iomanip>

namespace qpmu {

std::string phasorToString(const Complex &phasor)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << phasor.real() << "+-"[phasor.imag() < 0] << std::abs(phasor.imag()) << 'j';
    return ss.str();
}

std::string phasorPolarToString(const Complex &phasor)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << std::abs(phasor) << "∠" << (std::arg(phasor) * 180 / M_PI) << "°";
    return ss.str();
}

std::string toString(const Sample &sample)
{
    std::stringstream ss;
    ss << "seq=" << sample.seq << ',' << '\t';
    for (USize i = 0; i < CountSignals; ++i) {
        ss << "ch" << i << "=" << std::setw(4) << sample.channels[i] << ", ";
    }
    ss << "ts=" << sample.timestampUs << ',' << '\t';
    ss << "delta=" << sample.timeDeltaUs << ",";
    return ss.str();
}

std::string toString(const Estimation &est)
{
    std::stringstream ss;
    for (USize i = 0; i < CountSignals; ++i) {
        ss << "phasor_" << i << "=" << phasorPolarToString(est.phasors[i]) << ',' << '\t';
    }
    for (USize i = 0; i < CountSignals; ++i) {
        ss << "freq_" << i << "=" << est.frequencies[i] << ',' << '\t';
    }
    for (USize i = 0; i < CountSignals; ++i) {
        ss << "rocof_" << i << "=" << est.rocofs[i] << ',' << '\t';
    }
    ss << "sampling_rate=" << est.samplingRate << ',';
    return ss.str();
}

std::string sampleCsvHeader()
{
    std::string header;
    header += "seq,";
    for (USize i = 0; i < CountSignals; ++i) {
        header += "ch" + std::to_string(i) + ',';
    }
    header += "ts,delta";
    return header;
}

std::string estimationCsvHeader()
{
    std::string header;
    header += "timestamp_micros,";
    for (USize i = 0; i < CountSignals; ++i) {
        header += "phasor_" + std::to_string(i) + ',';
    }
    for (USize i = 0; i < CountSignals; ++i) {
        header += "freq_" + std::to_string(i) + ',';
    }
    for (USize i = 0; i < CountSignals; ++i) {
        header += "rocof_" + std::to_string(i) + ',';
    }
    header += "sampling_rate";
    return header;
}

/// Parse a sample from a string.
/// See `toString(const Sample&)` for the expected format.
Sample parseSample(const char *const s, std::string *errorOut)
{
    U64 sampleVector[CountSignals + 3] = { 0 }; // seq, (channels * 6), timestamp, timeDelta
    U64 value = 0;
    bool readingValue = false;
    std::string error;

    ISize index = 0;
    const char *charPtr = s;
    while (index < 9 && (*charPtr && *charPtr != '\n')) { // Read until end or newline
        const char &c = *charPtr++;
        switch (c) {
        case '=': // End of field's key, start of field's value
            readingValue = true;
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            value = (value * 10) + readingValue * (c - '0');
            break;
        case ',': // End of field's value, start of NEXT field's key
            sampleVector[index] = value;
            ++index;
            value = 0;

            if (!readingValue) {
                error = "Unexpected comma";
                break;
            }
            readingValue = false;
        }
    }

    Sample sample;

    if (error.empty()) {
        U64 *fieldPtr = &sample.timeDeltaUs; /// Iterate in reverse order
        for (--index; index >= 0; --index) {
            *fieldPtr = sampleVector[index];
            --fieldPtr; // Move to the previous field
        }
    }

    if (errorOut) {
        *errorOut = error;
    }
    return sample;
}

std::string toCsv(const Sample &sample)
{
    std::string str;
    str += std::to_string(sample.seq);
    for (size_t i = 0; i < CountSignals; ++i) {
        str += std::to_string(sample.channels[i]);
        str += ',';
    }
    str += std::to_string(sample.timestampUs);
    str += ',';
    str += std::to_string(sample.timeDeltaUs);
    return str;
}

std::string toCsv(const Estimation &est)
{
    std::string str;
    for (size_t i = 0; i < CountSignals; ++i) {
        str += phasorPolarToString(est.phasors[i]);
        str += ',';
    }
    for (size_t i = 0; i < CountSignals; ++i) {
        str += std::to_string(est.frequencies[i]);
        str += ',';
    }
    for (size_t i = 0; i < CountSignals; ++i) {
        str += std::to_string(est.rocofs[i]);
        str += ',';
    }
    str += std::to_string(est.samplingRate);
    return str;
}

std::pair<Float, Float> linearRegression(const std::vector<Float> &x, const std::vector<Float> &y)
{
    assert(x.size() == y.size());
    assert(x.size() > 0);

    Float xMean = std::accumulate(x.begin(), x.end(), (Float)(0.0)) / x.size();
    Float yMean = std::accumulate(y.begin(), y.end(), (Float)(0.0)) / y.size();

    Float numerator = 0.0;
    Float denominator = 0.0;

    for (USize i = 0; i < x.size(); ++i) {
        numerator += (x[i] - xMean) * (y[i] - yMean);
        denominator += (x[i] - xMean) * (x[i] - xMean);
    }

    Float m = denominator ? numerator / denominator : 0.0;
    Float b = yMean - (m * xMean);

    return { m, b }; // Return slope (m) and intercept (b)
}

} // namespace qpmu