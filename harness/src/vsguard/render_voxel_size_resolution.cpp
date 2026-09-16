// VoxelScale Guard - pure voxel-size resolution for vc_render_tifxyz.
// See render_voxel_size_resolution.hpp and DOCS/ARCHITECTURE.md.

#include "vsguard/render_voxel_size_resolution.hpp"

#include <cmath>
#include <initializer_list>
#include <string_view>

#include "vc/core/util/RemoteUrl.hpp"
#include "vc/core/util/VoxelSizeMetadata.hpp"

// The upstream resolver for local stores is used directly rather than
// reimplemented. It already knows the schemas the catalog publishes, and it is
// already the resolver the rest of VC3D agrees with (Volume::NewFromUrl and
// vc_grow_seg_from_segments both route through it). Duplicating it is how
// vc_render_tifxyz drifted in the first place.
using vc::metadata::voxelSizeFromStoreMetadata;

namespace vsguard
{

namespace
{

// Serialize a JSON scalar the way a non-numeric value has to be reported.
// Only used for diagnostics.
constexpr const char* kNanometers = "nanometer";
constexpr const char* kMicrometers = "micrometer";
constexpr const char* kMillimeters = "millimeter";
constexpr const char* kMeters = "meter";

// A voxel-size field this module understands, in the order it is preferred.
//
// This list deliberately duplicates the one inside VoxelSizeMetadata.cpp rather
// than exposing it: storeVoxelSize() needs to know whether the document *states*
// a size, which that module's public API cannot report. Any key added there
// must be added here too, and the test
// "stated detection agrees with the resolver on every alias" fails if they
// drift.
constexpr const char* kStatedKeys[] = {
    "voxelsize", "voxel_size_um", "voxelSizeUm",
    "pixel_size_um", "pixelSizeUm", "resolution_um",
};

std::optional<utils::Json> walk(const utils::Json& root,
                                std::initializer_list<std::string_view> keys)
{
    const utils::Json* current = &root;
    for (const std::string_view key : keys) {
        const std::string name(key);
        if (!current->is_object() || !current->contains(name))
            return std::nullopt;
        current = &(*current)[name];
    }
    return *current;
}

// Did the document state a voxel size anywhere this project recognizes,
// regardless of whether the value is usable? Mirrors the tiers in
// voxelSizeFromStoreMetadata(): top-level aliases, then the ESRF/BM18
// acquisition record with and without the `scan` wrapper, then the same under
// `source.metadata` for derived stores.
bool documentStatesAVoxelSize(const utils::Json& document)
{
    if (!document.is_object())
        return false;

    for (const char* key : kStatedKeys) {
        if (document.contains(key))
            return true;
    }

    const auto statesDetectorSize = [](const utils::Json& root) {
        return walk(root, {"scan", "tomo", "acquisition", "detector", "samplePixelSize"})
                   .has_value() ||
               walk(root, {"tomo", "acquisition", "detector", "samplePixelSize"})
                   .has_value();
    };

    if (statesDetectorSize(document))
        return true;

    const auto source = walk(document, {"source"});
    if (!source)
        return false;
    const auto sourceMetadata = walk(*source, {"metadata"});
    if (!sourceMetadata)
        return false;
    return statesDetectorSize(*sourceMetadata);
}

std::optional<double> optOf(const utils::Json& value)
{
    if (!value.is_number())
        return std::nullopt;
    const double number = value.get_double();
    if (!std::isfinite(number) || number <= 0.0)
        return std::nullopt;
    return number;
}

} // namespace

const char* toString(VoxelSizeSource source)
{
    switch (source) {
    case VoxelSizeSource::Cli:                return "command line";
    case VoxelSizeSource::LocalStoreMetadata: return "local store metadata";
    case VoxelSizeSource::RemoteVolume:       return "remote volume metadata";
    case VoxelSizeSource::RemoteMetadata:     return "remote store metadata";
    case VoxelSizeSource::Unspecified:        return "unspecified";
    }
    return "unspecified";
}

std::ostream& operator<<(std::ostream& os, VoxelSizeSource source)
{
    return os << toString(source);
}

bool isUsableMicrometerPerVoxel(double value)
{
    return std::isfinite(value) && value > 0.0;
}

std::optional<double> explicitMicrometerPerVoxel(const ExplicitVoxelSize& explicitSize)
{
    if (!explicitSize.present)
        return std::nullopt;
    if (!std::isfinite(explicitSize.value) || explicitSize.value <= 0.0)
        return std::nullopt;

    const std::string& unit = explicitSize.unit;
    if (unit == kNanometers || unit == "nanometre" || unit == "nm")
        return explicitSize.value * 0.001;
    if (unit == kMicrometers || unit == "micrometre" || unit == "um" || unit == "\xC2\xB5m")
        return explicitSize.value;
    if (unit == kMillimeters || unit == "millimetre" || unit == "mm")
        return explicitSize.value * 1000.0;
    if (unit == kMeters || unit == "metre" || unit == "m")
        return explicitSize.value * 1000000.0;

    // Unknown unit: refuse rather than guess. A wrong unit is a silent
    // 1000x error in every physical number the render emits.
    return std::nullopt;
}

ResolvedVoxelSize resolveVoxelSize(
    const ExplicitVoxelSize& explicitSize,
    const std::optional<double>& localMicrometerPerVoxel,
    const std::optional<double>& volumeMicrometerPerVoxel,
    const std::optional<double>& remoteMicrometerPerVoxel,
    double fallbackMicrometerPerVoxel)
{
    if (const auto cli = explicitMicrometerPerVoxel(explicitSize);
        cli && isUsableMicrometerPerVoxel(*cli)) {
        return {*cli, VoxelSizeSource::Cli};
    }
    if (localMicrometerPerVoxel && isUsableMicrometerPerVoxel(*localMicrometerPerVoxel))
        return {*localMicrometerPerVoxel, VoxelSizeSource::LocalStoreMetadata};
    if (volumeMicrometerPerVoxel && isUsableMicrometerPerVoxel(*volumeMicrometerPerVoxel))
        return {*volumeMicrometerPerVoxel, VoxelSizeSource::RemoteVolume};
    if (remoteMicrometerPerVoxel && isUsableMicrometerPerVoxel(*remoteMicrometerPerVoxel))
        return {*remoteMicrometerPerVoxel, VoxelSizeSource::RemoteMetadata};

    return {fallbackMicrometerPerVoxel, VoxelSizeSource::Unspecified};
}

StoreVoxelSize storeVoxelSize(const utils::Json& document)
{
    return {voxelSizeFromStoreMetadata(document), documentStatesAVoxelSize(document)};
}

std::optional<double> resolveRemoteStoreVoxelSize(
    const std::string& sourceUrl,
    const std::function<std::string(const std::string& url)>& fetch)
{
    if (sourceUrl.empty() || !fetch)
        return std::nullopt;

    for (const char* name : {"meta.json", "metadata.json"}) {
        const std::string url = vc::joinRemoteUrlPath(sourceUrl, name);
        const std::string body = fetch(url);
        if (body.empty())
            continue;
        try {
            const auto resolved = voxelSizeFromStoreMetadata(utils::Json::parse(body));
            if (resolved && isUsableMicrometerPerVoxel(*resolved))
                return resolved;
        } catch (...) {
            // Unparseable body: try the next document, exactly as the local
            // path does.
        }
    }
    return std::nullopt;
}

// --- The .zattrs number/unit pair -------------------------------------------

std::optional<double> micrometersPerUnit(const std::string& unit)
{
    // The same spellings --voxel-unit accepts. Kept in one place so the conversion
    // used to *read* a CLI value and the one used to *check* what was written
    // cannot drift apart.
    if (unit == "nanometer" || unit == "nanometre" || unit == "nm")
        return 0.001;
    if (unit == "micrometer" || unit == "micrometre" || unit == "um" || unit == "\xC2\xB5m")
        return 1.0;
    if (unit == "millimeter" || unit == "millimetre" || unit == "mm")
        return 1000.0;
    if (unit == "meter" || unit == "metre" || unit == "m")
        return 1000000.0;
    return std::nullopt;
}

std::string zarrUnit(const ResolvedVoxelSize& resolved,
                     const std::string& explicitUnit)
{
    switch (resolved.source) {
    case VoxelSizeSource::Cli:
        // The caller's number and unit are kept as a pair, so the declared unit is
        // the caller's.
        return explicitUnit;
    case VoxelSizeSource::LocalStoreMetadata:
    case VoxelSizeSource::RemoteVolume:
    case VoxelSizeSource::RemoteMetadata:
        // All of these resolve to micrometres by definition.
        return "micrometer";
    case VoxelSizeSource::Unspecified:
        return {};
    }
    return {};
}

double zarrScaleValue(const ResolvedVoxelSize& resolved,
                      const ExplicitVoxelSize& explicitSize)
{
    if (resolved.source != VoxelSizeSource::Cli)
        return resolved.micrometerPerVoxel;   // micrometres, as declared
    return explicitSize.value;                // the caller's own number
}

double voxelSizeToDpi(double micrometersPerPixel)
{
    return micrometersPerPixel > 0.0 ? 25400.0 / micrometersPerPixel : 0.0;
}

} // namespace vsguard
