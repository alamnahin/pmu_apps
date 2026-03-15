
#include <boost/program_options/options_description.hpp>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <sstream>

#include <boost/program_options.hpp>
#include <string>

#include "qpmu/defs.h"
#include "qpmu/util.h"
#include "qpmu/estimator.h"

using std::string, std::cout, std::cerr;
namespace po = boost::program_options;

int main(int argc, char *argv[])
{
    using namespace qpmu;

    po::options_description desc("Allowed options");
    {
        auto new_option = [](const char *name, const po::value_semantic *s,
                             const char *description) {
            return boost::shared_ptr<po::option_description>(
                    new po::option_description(name, s, description));
        };

        desc.add(new_option("help", po::value<string>(), "Produce help message"));
        desc.add(new_option("fn", po::value<USize>(),
                            "Nominal frequency of the system in Hz (required)"));
        desc.add(new_option("fs", po::value<USize>(),
                            "Sampling frequency of the system in Hz (required)"));
        desc.add(new_option("phasor-estimator", po::value<string>()->default_value("fft"),
                            "Phasor estimation strategy to use: fft, sdft"));
        desc.add(new_option(
                "infmt", po::value<string>()->default_value("b"),
                "Input format: b (binary), s (human-readable string), c (comma separated "
                "\"key=value\" pairs)"));
        desc.add(new_option(
                "outfmt", po::value<string>()->default_value("b"),
                "Output format: b (binary), s (human-readable string), c (comma separated "
                "\"key=value\" pairs)"));
    }

    po::variables_map varmap;
    po::store(po::parse_command_line(argc, argv, desc), varmap);
    po::notify(varmap);

    if (varmap.count("help")) {
        cout << desc << '\n';
        return 0;
    }

    PhasorEstimationStrategy phasorStrategy = PhasorEstimationStrategy::FFT;
    if (varmap.count("phasor-estimator")) {
        if (varmap["phasor-estimator"].as<string>() == "sdft") {
            phasorStrategy = PhasorEstimationStrategy::SDFT;
        }
    }

    USize fn = 0;
    if (!varmap.count("fn")) {
        cerr << "Nominal frequency (`fn`) is required\n";
        cerr << desc << '\n';
        return 1;
    }
    fn = varmap["fn"].as<USize>();

    USize fs = 0;
    if (!varmap.count("fs")) {
        cerr << "Sampling frequency (`fs`) is required\n";
        cerr << desc << '\n';
        return 1;
    }
    fs = varmap["fs"].as<USize>();

    enum Format { FormatReadableStr, FormatCsv, FormatBinary };
    Format inputFormat;
    Format outputFormat;
    if (varmap["infmt"].as<string>() == "s") {
        inputFormat = FormatReadableStr;
    } else if (varmap["infmt"].as<string>() == "c") {
        inputFormat = FormatCsv;
    } else {
        inputFormat = FormatBinary;
    }
    if (varmap["outfmt"].as<string>() == "s") {
        outputFormat = FormatReadableStr;
    } else if (varmap["outfmt"].as<string>() == "c") {
        outputFormat = FormatCsv;
    } else {
        outputFormat = FormatBinary;
    }

    PhasorEstimator estimator(fn, fs, phasorStrategy);
    Estimation phasor;
    Sample sample;

    auto print = [&] {
        switch (outputFormat) {
        case FormatBinary:
            std::fwrite(&phasor, sizeof(Estimation), 1, stdout);
            break;
        case FormatCsv:
            cout << toCsv(phasor) << '\n';
            break;
        default:
            cout << toString(phasor) << '\n';
        }
    };

    if (outputFormat == FormatCsv) {
        cout << estimationCsvHeader() << '\n';
    }

    if (inputFormat == FormatReadableStr) {
        std::string line;
        while (std::getline(std::cin, line)) {
            std::string error;
            sample = parseSample(line.c_str(), &error);
            if (!error.empty()) {
                cerr << "Failed to parse line: " << line << '\n';
                cerr << "Error: " << error << '\n';
                return 1;
            }
            estimator.updateEstimation(sample);
            phasor = estimator.lastEstimation();
            print();
        }
    } else if (inputFormat == FormatBinary) {
        while (fread(&sample, sizeof(Sample), 1, stdin)) {
            estimator.updateEstimation(sample);
            phasor = estimator.lastEstimation();
            print();
        }
    }

    return 0;
}