#ifndef QPMU_COMMON_UTIL_H
#define QPMU_COMMON_UTIL_H

#include "qpmu/defs.h"

#include <string>
#include <complex>
#include <vector>
#include <utility>

namespace qpmu {

std::string phasorToString(const Complex &phasor);
std::string phasorPolarToString(const Complex &phasor);
std::string estimationCsvHeader();
std::string toCsv(const Estimation &estimation);
std::string toString(const Estimation &estimation);
Sample parseSample(const char *const s, std::string *errorOut = nullptr);
std::string sampleCsvHeader();
std::string toCsv(const Sample &sample);
std::string toString(const Sample &sample);

std::pair<Float, Float> linearRegression(const std::vector<Float> &x, const std::vector<Float> &y);

} // namespace qpmu

#endif // QPMU_COMMON_UTIL_H
