// Throwaway probe: confirms that villa's VoxelSizeMetadata.hpp is self-contained
// enough to parse without OpenCV, Qt or the rest of volume-cartographer.
//
// That property is what makes the harness possible at all — the shared resolver
// can be compiled and executed on a machine that cannot build the application.
// Kept because it is the cheapest way to re-check that assumption after an
// upstream update:
//
//   cl /nologo /std:c++20 /EHsc /Zs \
//      /I harness/syntax/stubs /I villa/volume-cartographer/core/include \
//      /I villa/volume-cartographer/utils/include harness/syntax/parse_probe.cpp
//
// Exit 0 means the header parsed. Not part of any test suite.

#include "vc/core/util/VoxelSizeMetadata.hpp"

int main() { return 0; }

