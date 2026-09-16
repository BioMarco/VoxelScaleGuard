// VoxelScale Guard - before/after demonstrator.
//
// Runs the deployed reader (vc_render_tifxyz as shipped) and the patched
// resolver over the *real* metadata documents published by the open-data
// catalog, and reports what physical voxel size the renderer would use, and
// what that costs it downstream.
//
// The documents are the ones saved by research/fetch_volume_metadata.mjs; pass
// their paths as arguments, or let it default to the raw_metadata directory.
//
// Usage:
//   probe_render_voxel_size <document> [<document> ...]
//   probe_render_voxel_size            (uses ../research/raw_metadata)

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "vc/core/util/VoxelSizeMetadata.hpp"

#include "vsguard/render_voxel_size_resolution.hpp"
#include "vsguard/upstream_read_volume_voxel_size.hpp"

namespace fs = std::filesystem;

namespace
{

// vc_render_tifxyz declares its OME-Zarr axis unit from `--voxel-unit`, whose
// default is "nanometer", and its scale from the resolved voxel size, which is
// in micrometers. Reproduced from vc_render_tifxyz.cpp:1084 (the default) and
// :1581 (writeZarrAttrs). The consequence is that the declared physical size is
// the numeric value interpreted as nanometres, i.e. 1000x too small on top of
// whatever error the resolution itself carries.
double declaredMicrometerPerVoxel(double value, const char* unit)
{
    return std::string(unit) == "nanometer" ? value * 0.001 : value;
}

std::string readFile(const fs::path& path)
{
    std::ifstream in(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

} // namespace

int main(int argc, char** argv)
{
    std::vector<fs::path> documents;
    if (argc > 1) {
        for (int i = 1; i < argc; ++i)
            documents.emplace_back(argv[i]);
    } else {
        // The harness binary lives in harness/build/<config>/, so the raw
        // documents are three levels up.
        const fs::path candidates[] = {
            fs::path("..") / ".." / ".." / "research" / "raw_metadata",
            fs::path(".") / ".." / ".." / "research" / "raw_metadata",
        };
        fs::path dir;
        for (const auto& candidate : candidates) {
            if (fs::exists(candidate)) { dir = candidate; break; }
        }
        if (dir.empty()) {
            std::fprintf(stderr,
                         "no documents given and no research/raw_metadata directory found\n");
            return 2;
        }
        for (const auto& entry : fs::directory_iterator(dir))
            documents.push_back(entry.path());
    }

    std::printf("VoxelScale Guard - vc_render_tifxyz voxel-size resolution\n");
    std::printf("deployed reader executed: yes (verbatim copy of "
                "vc_render_tifxyz.cpp:986-1003 @ 757f70c)\n\n");

    int divergences = 0;

    for (const fs::path& document : documents) {
        const std::string body = readFile(document);
        utils::Json parsed;
        try {
            parsed = utils::Json::parse(body);
        } catch (...) {
            std::printf("--- %s\n    SKIP: not valid JSON\n\n", document.filename().string().c_str());
            continue;
        }

        const fs::path store = fs::temp_directory_path() / "vsguard_probe_store";
        fs::create_directories(store);

        const bool looksLikeMetaJson = document.filename().string().ends_with(".meta.json");
        const fs::path metaPath = store / "meta.json";
        const fs::path metadataPath = store / "metadata.json";
        std::error_code ignored;
        fs::remove(metaPath, ignored);
        fs::remove(metadataPath, ignored);
        std::ofstream(looksLikeMetaJson ? metaPath : metadataPath, std::ios::binary) << body;

        // 1. What the shipped tool computes.
        const auto deployed = vsguard::upstream::readVolumeVoxelSize(store);

        // 2. What the store actually says.
        const auto shared = vc::metadata::resolveLocalStoreVoxelSize(store);

        std::printf("--- %s\n", document.filename().string().c_str());
        if (deployed) {
            std::printf("    deployed reader  : %g um/voxel\n", *deployed);
        } else {
            std::printf("    deployed reader  : NOT FOUND -> falls back to 1.0, "
                        "declared as nanometer\n");
        }
        if (shared) {
            std::printf("    store actually says: %g um/voxel\n", *shared);
        } else {
            std::printf("    store actually says: no usable voxel size published\n");
        }

        if (shared && (!deployed || std::abs(*deployed - *shared) > 1e-9)) {
            const double used = deployed ? *deployed : 1.0;

            // What the shipped binary writes into .zattrs on this document.
            const char* const unpatchedUnit = "nanometer";   // --voxel-unit default
            const double unpatchedPhysical =
                declaredMicrometerPerVoxel(used, unpatchedUnit);

            std::printf("    .zattrs as shipped  : scale %g with unit \"%s\""
                        "  =>  %g um/voxel\n",
                        used, unpatchedUnit, unpatchedPhysical);
            std::printf("    .zattrs if corrected: scale %g with unit \"micrometer\""
                        "  =>  %g um/voxel\n",
                        *shared, *shared);
            std::printf("    DIVERGENCE: declared physical voxel size wrong by %.6gx "
                        "(numeric %.6gx from the unresolved size, 1000x from the "
                        "nanometre default)\n",
                        *shared / unpatchedPhysical, *shared / used);
            ++divergences;
        } else if (!shared && !deployed) {
            std::printf("    (agree: neither finds a size)\n");
        } else {
            std::printf("    (agree)\n");
        }

        // 3. What the patched renderer would use, resolving through the same
        //    tiers the fix installs. There is no Volume here, so the remote
        //    tier is exercised through the store document alone.
        const vsguard::ResolvedVoxelSize patched = vsguard::resolveVoxelSize(
            vsguard::ExplicitVoxelSize{},
            shared,
            std::nullopt,
            std::nullopt,
            /*fallback=*/1.0);
        std::printf("    patched renderer : %g um/voxel  (source: %s, usable: %s)\n\n",
                    patched.micrometerPerVoxel,
                    vsguard::toString(patched.source),
                    patched.isUsable() ? "yes" : "NO - placeholder, must not be "
                                                "presented as a measurement");
    }

    std::printf("documents examined: %zu, divergences: %d\n",
                documents.size(), divergences);
    return divergences > 0 ? 0 : 0;   // informational tool: never fails the run
}
