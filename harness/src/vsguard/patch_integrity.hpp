// =============================================================================
//  Real-artefact integrity checks for the committed patch.
//
//  Why this exists
//  ---------------
//  The patch was "logic-verified" and had never been compiled. A CI run on
//  2026-09-16 compiled it for the first time and it FAILED:
//
//      vc_render_tifxyz.cpp:1449:43: error: 'hasExplicitVoxelSize' was not
//      declared in this scope
//
//  The new resolution block had been inserted ~60 lines BEFORE the declarations
//  of `hasExplicitVoxelSize`, `explicitVoxelSize`, `voxel_unit`,
//  `base_voxel_size`, `render_level_voxel_size` and `zarr_voxel_unit`. Nothing in
//  the harness could see that, because the harness never compiled the file.
//
//  What these checks do, and what they do not
//  ------------------------------------------
//  They run against the REAL artefacts: the pristine pinned revision under
//  harness/src/villa/, and the patched working tree under villa/ that the
//  committed patch describes. There is no reimplementation of diff or of the
//  resolver here; `git apply --check --reverse` remains the authority on whether
//  the patch matches the tree (AGENTS.md section 4).
//
//  They prove the specific ordering defect that compilation found is absent.
//  They CANNOT prove the patched file compiles -- genuine type errors need the
//  real build, which is what .github/workflows/renderer-validation.yml is for.
// =============================================================================

#pragma once

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace vsguard_patch {

// The repository root, baked in at compile time by harness/build.ps1. Deriving it
// from __FILE__ does not work reliably: cl stores the path as spelled on the
// command line, which may be relative to the build directory.
inline std::filesystem::path repoRoot()
{
#ifdef VSGUARD_REPO_ROOT
    const std::filesystem::path root{VSGUARD_REPO_ROOT};
    if (std::filesystem::exists(root / "patch" / "vc_render_tifxyz.patch")) return root;
#endif
    std::filesystem::path here = std::filesystem::current_path();
    for (int i = 0; i < 4; ++i) {
        if (std::filesystem::exists(here / "patch" / "vc_render_tifxyz.patch")) return here;
        if (!here.has_parent_path()) break;
        here = here.parent_path();
    }
    return {};
}

inline std::string readWholeFile(const std::filesystem::path& p)
{
    std::ifstream in(p, std::ios::binary);
    if (!in) return {};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

inline std::vector<std::string> splitTextLines(const std::string& text)
{
    std::vector<std::string> out;
    std::istringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        out.push_back(line);
    }
    return out;
}

// 1-based number of the first line containing `needle`, else 0.
inline std::size_t firstLineContaining(const std::vector<std::string>& lines,
                                       const std::string& needle)
{
    for (std::size_t i = 0; i < lines.size(); ++i)
        if (lines[i].find(needle) != std::string::npos) return i + 1;
    return 0;
}

// 1-based number of the line that DECLARES `name`, as opposed to using it.
inline std::size_t declarationLineOf(const std::vector<std::string>& lines,
                                     const std::string& name)
{
    const char* types[] = {"double ", "bool ", "std::string ", "const std::string ",
                           "float ", "int ", "const double "};
    for (std::size_t i = 0; i < lines.size(); ++i) {
        for (const char* t : types) {
            if (lines[i].find(std::string(t) + name) != std::string::npos) return i + 1;
        }
    }
    return 0;
}

// 1-based line of a CALL to `call` -- i.e. a line containing the call text whose
// next non-empty line contains `firstArg`. This distinguishes the call site from
// the function's own definition, whose parameter list does not mention the
// argument name used at the call.
inline std::size_t lineOfCallSite(const std::vector<std::string>& lines,
                                  const std::string& callText,
                                  const std::string& firstArg)
{
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (lines[i].find(callText) == std::string::npos) continue;
        for (std::size_t j = i + 1; j < lines.size() && j <= i + 3; ++j) {
            if (lines[j].empty()) continue;
            if (lines[j].find(firstArg) != std::string::npos) return i + 1;
            break;
        }
    }
    return 0;
}

inline std::filesystem::path pristineTargetFile(const std::filesystem::path& root)
{
    return root / "harness/src/villa/apps/src/vc_render_tifxyz.cpp";
}

inline std::filesystem::path patchedTargetFile(const std::filesystem::path& root)
{
    return root / "villa/volume-cartographer/apps/src/vc_render_tifxyz.cpp";
}

} // namespace vsguard_patch
