# pepc-viewer
Visualisation of MSMS spectrum pairs

## Overview

**pepc-viewer** is a C++ visualisation tool built with [Dear ImGui](https://github.com/ocornut/imgui) and [ImPlot](https://github.com/epezent/implot), compiled to WebAssembly (WASM) via [Emscripten](https://emscripten.org/) for in-browser use.

It displays a **mirror plot** of two mass spectra side-by-side:

* The **top** spectrum is drawn with positive intensities (peaks upward, blue).
* The **bottom** spectrum is drawn with negated intensities (peaks downward, orange/red).
* m/z values that appear in **both** spectra (within a configurable tolerance) are connected by a **vertical green line**.

![pepc-viewer screenshot](https://github.com/user-attachments/assets/66139e7f-4e1f-4071-b8e6-6394d5994add)

## Data model

```
src/spectrum.h      – Spectrum struct  (name, id, weight, mz[], intensity[])
src/spectrum_pair.h – SpectrumPair class (top, bottom, sharedPeaks())
src/main.cpp        – ImGui + ImPlot application
```

### `Spectrum`

| field       | type                   | description                     |
|-------------|------------------------|---------------------------------|
| `name`      | `std::string`          | human-readable label            |
| `id`        | `std::string`          | unique identifier               |
| `weight`    | `double`               | spectrum weight                 |
| `mz`        | `std::vector<double>`  | m/z values                      |
| `intensity` | `std::vector<double>`  | intensities (same length as mz) |

### `SpectrumPair`

| field/method             | description                                            |
|--------------------------|--------------------------------------------------------|
| `top`                    | spectrum shown upward                                  |
| `bottom`                 | spectrum shown downward                                |
| `sharedPeaks(tolerance)` | peaks present in both spectra within `tolerance` Da   |

## Building

### Prerequisites

On RHEL/AlmaLinux (and similar):

```bash
dnf install cmake make gcc-c++ git SDL2-devel mesa-libGL-devel
```

| tool         | purpose                          |
|--------------|----------------------------------|
| CMake ≥ 3.16 | build system                     |
| SDL2         | windowing (native + WASM builds) |
| OpenGL       | rendering  (native build)        |
| Emscripten   | WASM compilation (web build)     |

CMake automatically downloads ImGui and ImPlot via `FetchContent`.

### Native desktop build (SDL2 + OpenGL)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/pepc-viewer
```

### WebAssembly / WASM build

First install the Emscripten SDK and ensure it is activated in the current
shell (see the official docs for details). Roughly:

```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

Then configure and build the WASM target:

```bash
emcmake cmake -B build-wasm -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm -j$(nproc)
# serve build-wasm/ with any HTTP server, then open pepc-viewer.html
python3 -m http.server --directory build-wasm 8080
```

## Usage

The application currently displays a built-in example pair (`Spectrum A` vs `Spectrum B`).
The plot is fully interactive: pan with left-click drag, zoom with the scroll wheel,
and right-click for context options.
