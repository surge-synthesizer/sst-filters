# SST Filters

Tests: [![CI Build Status](https://dev.azure.com/surge-synthesizer/surge/_apis/build/status/surge-synthesizer.sst-filters?branchName=main)](https://dev.azure.com/surge-synthesizer/surge/_apis/build/status/surge-synthesizer.sst-filters?branchName=main)

This repository contains the filters from the
[Surge Synthesizer](https://github.com/surge-synthesizer/surge)
as a submodule.

Current filters include:
- Original Surge filters:
  - LPF/HPF/BPF/Notch/APF
  - "Legacy" Ladder
  - Comb
  - Sample & Hold
- "Vintage" Ladder filters
- OB-Xd filters (ported from [OB-Xd](https://github.com/reales/OB-Xd))
- K35 and Diode Ladder filters (ported from [Odin 2](https://github.com/TheWaveWarden/odin2))
- Cutoff/Resonance Warp filters
- Tri-Pole filter

For more information, see the [API documentation](https://surge-synthesizer.github.io/sst-docs/docs/sst-filters).

## Usage

`sst-filters` is a header-only library, so it is possible to use the library
simply by adding `include/` to your header search paths, and writing
`#include <sst/filters.h>` somewhere in your source code. However, it is
recommended to use the provided CMake configuration.

```cmake
add_subdirectory(path/to/sst-filters)
target_link_libraries(My-Killer-App PUBLIC  sst-filters)
```

**Note: the sst-filters library uses raw SSE SIMD intrinsics. If you
are planning to use this library in a context that may not support SSE
intrisics, it is recommended to also link with
[simde](https://github.com/simd-everywhere/simde).**

## Building Unit Tests

To build and run the sst-filters unit tests:
```bash
cmake -Bbuild
cmake --build build
./build/test-binary/sst-filters-tests
```

## Building Examples

To build the sst-filters examples:
```bash
cmake -Bbuild -DSST_FILTERS_BUILD_EXAMPLES=ON
cmake --build build
```

Example binaries will be located in `build/example-binaries/`.

## License
The code in this repository is licensed under the General Public License v3.
