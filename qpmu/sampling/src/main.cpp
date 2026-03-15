#include <algorithm>
#include <cassert>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/fusion/sequence.hpp>

#include "qpmu/defs.h"
#include "qpmu/util.h"

namespace fs = std::filesystem;
namespace po = boost::program_options;
using std::string, std::vector, std::cerr, std::cout;

int main(int argc, char *argv[])
{
    using namespace qpmu;
    /// Parse command line arguments
    /// ----------------------------

    po::options_description desc("Allowed options");
    desc.add_options()("help", "Produce help message")("v", po::value<string>(),
                                                       "Voltage in volts")("p", po::value<string>(),
                                                                           "Power in watts")(
            "outfmt", po::value<string>()->default_value("b"),
            "Output format: b (binary), s (human-readable string), c (comma separated "
            "\"key=value\" "
            "pairs)")("once", "Print samples only once")("sleep",
                                                         "Sleep for delta time between samples");

    po::variables_map varmap;
    po::store(po::parse_command_line(argc, argv, desc), varmap);
    po::notify(varmap);

    if (varmap.count("help")) {
        cout << desc << '\n';
        return 0;
    }

    if (!varmap.count("v") || !varmap.count("p")) {
        cerr << "v and p are required arguments\n";
        cerr << desc << '\n';
        return 1;
    }

    auto voltage_str = varmap["v"].as<string>();
    auto power_str = varmap["p"].as<string>();
    auto format = varmap["outfmt"].as<string>();
    auto once = varmap.count("once");
    const bool do_sleep = varmap.count("sleep");

    uint64_t voltage;
    {
        vector<string> tokens;
        auto first_non_digit = voltage_str.find_first_not_of("0123456789");

        auto voltage_unit = voltage_str.substr(std::min(voltage_str.size(), first_non_digit));
        voltage = std::stoull(voltage_str.substr(0, first_non_digit));

        boost::to_lower(voltage_unit);
        if (voltage_unit.empty()) {
            voltage_unit = "v";
        }

        if (voltage_unit == "mv") {
            voltage /= 1000;
        } else if (voltage_unit == "kv") {
            voltage *= 1000;
        } else if (voltage_unit != "v") {
            cerr << "Invalid voltage unit: " << voltage_unit << '\n';
            return 1;
        }
    }

    uint64_t power;
    {
        vector<string> tokens;
        auto first_non_digit = power_str.find_first_not_of("0123456789");

        auto power_unit = power_str.substr(std::min(power_str.size(), first_non_digit));
        power = std::stoull(power_str.substr(0, first_non_digit));

        boost::to_lower(power_unit);
        if (power_unit.empty()) {
            power_unit = "w";
        }

        if (power_unit == "mw") {
            power /= 1000;
        } else if (power_unit == "kw") {
            power *= 1000;
        } else if (power_unit != "w") {
            cerr << "Invalid power unit: " << power_unit << '\n';
            return 1;
        }
    }

    enum { FormatReadableStr, FormatCsv, FormatBinary } outputFormat;

    if (format == "s") {
        outputFormat = FormatReadableStr;
    } else if (format == "c") {
        outputFormat = FormatCsv;
    } else if (format == "b") {
        outputFormat = FormatBinary;
    } else {
        cerr << "Invalid format: " << format << '\n';
        return 1;
    }

    boost::to_lower(format);

    /// Open samples file and fill in the samples
    /// -----------------------------------------

    auto currfile = fs::path(__FILE__);
    auto rootdir = currfile.parent_path().parent_path();
    string filename = rootdir.string() + "/data/" + std::to_string(voltage) + "v_"
            + std::to_string(power) + "w.csv";
    if (!fs::is_regular_file(filename)) {
        cerr << "File not found: " << filename << '\n';
        return 1;
    }
    std::ifstream file(filename);
    if (!file.good() || !file.is_open()) {
        cerr << "Failed to open file: " << filename << '\n';
        return 1;
    }

    vector<Sample> samples;
    Sample sample;
    vector<string> lines;
    string line;
    while (std::getline(file, line)) {
        string error;
        sample = parseSample(line.c_str(), &error);
        if (!error.empty()) {
            cerr << "Failed to parse line: " << line << '\n';
            cerr << "Error: " << error << '\n';
            return 1;
        }
        samples.push_back(sample);
        lines.push_back(line);
    }

    /// Cycle through the samples and print them
    /// ----------------------------------------

    auto print = [outputFormat](const Sample &sample) {
        if (outputFormat == FormatCsv) {
            cout << toCsv(sample) << '\n';
            return;
        }
        if (outputFormat == FormatReadableStr) {
            cout << toString(sample) << '\n';
            return;
        }
        std::fwrite(&sample, sizeof(Sample), 1, stdout);
    };

    if (once) {
        for (const auto &sample : samples) {
            print(sample);
        }
        return 0;
    }

    if (outputFormat == FormatCsv) {
        cout << sampleCsvHeader() << '\n';
    }

    auto lastSample = samples[0];
    lastSample.seq = 0;
    lastSample.timestampUs = std::chrono::duration_cast<std::chrono::microseconds>(
                                     std::chrono::system_clock::now().time_since_epoch())
                                     .count();
    for (size_t i = 1;; ++i) {
        auto sample = samples[i % samples.size()];
        sample.seq = i;
        sample.timestampUs = lastSample.timestampUs + sample.timeDeltaUs;

        print(sample);

        std::swap(lastSample, sample);

        if (do_sleep) {
            // sleep for the delta time
            std::this_thread::sleep_for(std::chrono::microseconds((U64)(sample.timeDeltaUs * 0.9)));
        }
    }

    return 0;
}