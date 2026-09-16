// VoxelScale Guard - pure voxel-size resolution for vc_render_tifxyz.
// See DOCS/ARCHITECTURE.md. No OpenCV/Qt/Volume dependency: this is the decision
// procedure only, so it can be exercised by the harness.

#pragma once

#include <functional>
#include <optional>
#include <ostream>
#include <string>

#include "utils/Json.hpp"

namespace vsguard
{

// Where a resolved physical voxel size came from. The origin is not decoration:
// "unspecified" means the value is a placeholder, and callers must not present
// it as a physical measurement.
enum class VoxelSizeSource {
    Cli,                    // --voxel-size, converted by --voxel-unit
    LocalStoreMetadata,     // the store's own meta.json / metadata.json
    RemoteVolume,           // the already-open remote Volume's resolved metadata
    RemoteMetadata,         // the store's meta.json / metadata.json over HTTP
    Unspecified,            // nothing resolved; value is a documented placeholder
};

[[nodiscard]] const char* toString(VoxelSizeSource source);

// Lets the test framework (and any logger) print the origin readably.
std::ostream& operator<<(std::ostream& os, VoxelSizeSource source);

struct ResolvedVoxelSize {
    double micrometerPerVoxel = 1.0;   // always micrometers per voxel
    VoxelSizeSource source = VoxelSizeSource::Unspecified;

    // The value a caller may use as a physical measurement. (IsUsable() because
    // a placeholder must never be silently treated as a measurement.)
    [[nodiscard]] bool isUsable() const
    {
        return source != VoxelSizeSource::Unspecified;
    }
};

// The CLI-side inputs that take priority over any metadata.
struct ExplicitVoxelSize {
    bool present = false;
    double value = 0.0;
    std::string unit = "micrometer";   // as accepted by --voxel-unit
};

// Convert an explicit --voxel-size/--voxel-unit pair to micrometers per voxel.
//
// Returns nullopt when the pair cannot be resolved to a positive finite size:
// either the value is not finite/positive, or the unit is not one we know how
// to convert. The caller is expected to turn that into a hard error, because a
// unit we cannot read is a user mistake, not a missing measurement.
[[nodiscard]] std::optional<double> explicitMicrometerPerVoxel(
    const ExplicitVoxelSize& explicitSize);

// --- The decision procedure -------------------------------------------------
//
// Priority, highest first:
//   1. explicit CLI override
//   2. local store metadata       (localMicrometerPerVoxel)
//   3. the already-open remote Volume's resolved metadata (volumeMicrometerPerVoxel)
//   4. remote store metadata fetched over HTTP     (remoteMicrometerPerVoxel)
//   5. fallback                    (fallbackMicrometerPerVoxel, source Unspecified)
//
// Step 3 precedes step 4 deliberately. When the renderer is streaming, it has
// already opened the volume through Volume::NewFromUrl, which resolves the
// store document with the same normalization the rest of VC3D uses. Reusing
// that value costs no extra request and cannot disagree with the volume being
// rendered. Step 4 exists for the case where no Volume has been opened.
[[nodiscard]] ResolvedVoxelSize resolveVoxelSize(
    const ExplicitVoxelSize& explicitSize,
    const std::optional<double>& localMicrometerPerVoxel,
    const std::optional<double>& volumeMicrometerPerVoxel,
    const std::optional<double>& remoteMicrometerPerVoxel,
    double fallbackMicrometerPerVoxel = 1.0);

// --- Helpers the app uses to fill the context ------------------------------

// Resolve a store document, as published, to micrometers per voxel.
//
// `statedVoxelSize` reports whether the document states any voxel-size field at
// all, even an unusable one (zero, negative, non-finite, or non-numeric). The
// distinction matters: "the document says nothing" is not the same as "the
// document says something we cannot use", and a caller that conflates them will
// skip a perfectly good fallback.
struct StoreVoxelSize {
    std::optional<double> micrometerPerVoxel;
    bool statedVoxelSize = false;
};

[[nodiscard]] StoreVoxelSize storeVoxelSize(const utils::Json& document);

// True when `value` can be used as a physical voxel size. The single definition
// of "usable" shared by every tier above, so the tiers cannot drift apart.
[[nodiscard]] bool isUsableMicrometerPerVoxel(double value);

// Fetch `meta.json`, else `metadata.json`, from a remote zarr store root and
// resolve it. `sourceUrl` must already be fragment-free: a raw locator such as
// "...zarr#vc-base-scale=1" makes the child join produce the nonsense path
// "...#vc-base-scale=1/meta.json", which silently resolves nothing. Use
// vc::parseRemoteVolumeSpec(...).sourceUrl to get a safe value.
//
// `fetch` returns the body, or an empty string for any failure. Never throws.
[[nodiscard]] std::optional<double> resolveRemoteStoreVoxelSize(
    const std::string& sourceUrl,
    const std::function<std::string(const std::string& url)>& fetch);

} // namespace vsguard
