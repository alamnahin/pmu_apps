# PMU Apps

Applications and libraries for generating sampled electrical signals, estimating phasors, and visualizing PMU data.

This repository contains a C++/Qt toolkit with three main applications:

- `qpmu-sampling-app`: streams synthetic PMU-like samples from prepared datasets.
- `qpmu-estimation-app`: consumes samples and estimates phasors/frequency metrics.
- `qpmu-app`: desktop Qt GUI for monitoring and configuration.

## Repository Layout

- `qpmu/common`: shared types and utility functions.
- `qpmu/sampling`: dataset-driven sample generator and Python simulator scripts.
- `qpmu/estimation`: phasor estimator library and CLI app.
- `qpmu/app`: Qt desktop application.
- `external`: third-party dependencies (including SDFT sources).

## Requirements

Minimum toolchain:

- CMake `>= 3.10`
- C++17 compiler
- Qt (`Core`, `Widgets`, `Charts`, `Network`) via Qt5 or Qt6
- Boost `>= 1.70` (`program_options`)
- FFTW3 (`fftw3`, `fftw3f`)

### macOS (Homebrew example)

Install common dependencies:

```bash
brew install cmake boost fftw qt
```

If Qt is not auto-detected, export its CMake path (example for Qt 6):

```bash
export CMAKE_PREFIX_PATH="$(brew --prefix qt)"
```

## Build

From the repository root:

```bash
cmake -S . -B build
cmake --build build -j
```

### Floating-point type selection

The project defaults to `double` precision (`QPMU_FLOAT_TYPE=double`).
You can switch precision at configure time:

```bash
cmake -S . -B build -DQPMU_FLOAT_TYPE=float
```

Possible values:

- `float`  -> project prefix `qpmuf`
- `double` -> project prefix `qpmu` (default)
- `long double` -> project prefix `qpmul`

This prefix affects binary names (for example `qpmu-sampling-app` vs `qpmuf-sampling-app`).

## Run

### 1) Sampling app

Generate sample stream for a voltage/power dataset:

```bash
./build/qpmu-sampling-app --v 240 --p 1000 --outfmt c --once
```

Useful options:

- `--v <value>`: voltage (required), supports suffixes `mv`, `v`, `kv`
- `--p <value>`: power (required), supports suffixes `mw`, `w`, `kw`
- `--outfmt <b|s|c>`: binary, readable string, or CSV output
- `--once`: print one cycle and exit
- `--sleep`: sleep between emitted samples

### 2) Estimation app

Consume stream from stdin and output estimations:

```bash
./build/qpmu-sampling-app --v 240 --p 1000 --outfmt b --sleep | \
./build/qpmu-estimation-app --fn 50 --fs 1200 --infmt b --outfmt c
```

Useful options:

- `--fn <hz>`: nominal frequency (required)
- `--fs <hz>`: sampling frequency (required)
- `--phasor-estimator <fft|sdft>`: estimator backend
- `--infmt <b|s|c>`: stdin format
- `--outfmt <b|s|c>`: stdout format

### 3) Qt desktop app

On macOS, CMake may produce an app bundle:

```bash
open build/qpmu-app.app
```

If a plain executable is generated in your environment, run:

```bash
./build/qpmu-app
```

## Data Files

Sampling datasets are located in:

- `qpmu/sampling/data`

Files follow the naming convention:

- `<voltage>v_<power>w.csv`
- example: `240v_1000w.csv`

## Python Simulator (Optional)

An additional simulator exists under:

- `qpmu/sampling/simulator`

Run example:

```bash
python3 qpmu/sampling/simulator/main.py --sampling-rate 1200 --frequency 50 --voltage 240
```

## Notes

- The top-level `requirements.txt` includes many environment-specific Python packages and is not required for the core C++ build.
- For reproducible development, use a clean build directory when switching compilers or major dependency versions.
