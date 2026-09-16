// VoxelScale Guard - regression tests for voxel-size resolution in
// vc_render_tifxyz.
//
// Two kinds of case live here:
//
//  * Divergence cases. The deployed reader (copied verbatim into
//    upstream_read_volume_voxel_size.hpp) and the shared resolver are run over
//    the same real store documents. Where they disagree, the deployed reader is
//    wrong. These are the defect: they fail before the patch by construction,
//    because the deployed reader is the pre-patch behaviour.
//
//  * Resolution cases. The patched decision procedure is checked on its own,
//    including the URL-fragment and unusable-local-metadata concerns raised in
//    review of ScrollPrize/villa#1417.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "vc/core/util/RemoteUrl.hpp"
#include "vc/core/util/VoxelSizeMetadata.hpp"

#include "vsguard/render_voxel_size_resolution.hpp"
#include "vsguard/upstream_read_volume_voxel_size.hpp"

using vsguard::ExplicitVoxelSize;
using vsguard::ResolvedVoxelSize;
using vsguard::StoreVoxelSize;
using vsguard::VoxelSizeSource;

// doctest's generic stringifier is not SFINAE-safe when both sides of a
// comparison are a scoped enum: it tries to build a detail string by pointer
// arithmetic and fails to compile. Rather than specialise the framework's
// internals, compare the readable name. The messages stay useful and the tests
// stay portable across doctest versions.
namespace vsguard
{
inline std::string sourceName(const ResolvedVoxelSize& resolved)
{
    return std::string(toString(resolved.source));
}
} // namespace vsguard

namespace fs = std::filesystem;

namespace
{

// --- The two real, published store documents, embedded -----------------------
//
// Both were fetched from the anonymous public bucket and are reproduced here so
// the tests need no network and no fixture directory. Shape and values are
// byte-faithful to research/raw_metadata/*.metadata.json; only unrelated keys
// (masking, zarr_export) are trimmed, which the resolver never reads.

// PHerc0009B/volumes/20250521125136-8.640um-1.2m-116keV-masked.zarr
// samplePixelSize 0.00864 mm -> 8.64 um, matching the volume's own name.
const char* kModernStoreMetadata = R"({
  "scan": {
    "tomo": {
      "acquisition": {
        "detector": { "samplePixelSize": 0.00864 }
      }
    }
  }
})";

// PHerc0172/volumes/20241024131838-7.910um-53keV-masked.zarr
// The legacy-shaped entry, and the only catalog volume the deployed reader
// reaches. This is the volume core/test/test_volume_live_s3.cpp pins.
const char* kLegacyMetaJson = R"({
  "height": 7654, "max": 255, "min": 0, "name": "20241024131838",
  "slices": 14830, "type": "vol", "uuid": "20241024131838",
  "voxelsize": 7.91, "width": 8096, "format": "zarr"
})";

fs::path makeTempStore(const std::string& tag)
{
    static int counter = 0;
    const fs::path dir = fs::temp_directory_path() /
        ("vsguard_" + tag + "_" + std::to_string(++counter) + "_" +
         std::to_string(static_cast<unsigned long long>(std::rand())));
    fs::create_directories(dir);
    return dir;
}

void writeTextFile(const fs::path& path, const std::string& text)
{
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    REQUIRE(out.good());
    out << text;
}

// A store root holding the given documents.
fs::path makeStore(const std::string& tag,
                   const std::string& metaJson,
                   const std::string& metadataJson)
{
    const fs::path dir = makeTempStore(tag);
    if (!metaJson.empty())
        writeTextFile(dir / "meta.json", metaJson);
    if (!metadataJson.empty())
        writeTextFile(dir / "metadata.json", metadataJson);
    return dir;
}

// What the deployed reader does for a store, and what the shared resolver does.
struct Divergence {
    std::optional<double> deployed;
    std::optional<double> shared;
};

Divergence compare(const fs::path& store)
{
    const auto shared = vc::metadata::resolveLocalStoreVoxelSize(store);
    return {vsguard::upstream::readVolumeVoxelSize(store), shared};
}

} // namespace

// =============================================================================
// 1. The defect: a modern published store is misread
// =============================================================================

TEST_CASE("DEFECT: deployed reader cannot read a modern published store")
{
    const fs::path store = makeStore("modern", "", kModernStoreMetadata);

    const auto shared = vc::metadata::resolveLocalStoreVoxelSize(store);
    REQUIRE(shared.has_value());
    CHECK(*shared == doctest::Approx(8.64));   // 0.00864 mm * 1000

    // The deployed reader finds nothing, so the renderer falls back to 1.0 and
    // asserts that as a physical scale. Wrong by a factor of 8.64.
    const auto deployed = vsguard::upstream::readVolumeVoxelSize(store);
    CHECK_FALSE(deployed.has_value());
}

TEST_CASE("DEFECT: the legacy store is the only shape the deployed reader reads")
{
    const fs::path store = makeStore("legacy", kLegacyMetaJson, "");

    const auto shared = vc::metadata::resolveLocalStoreVoxelSize(store);
    REQUIRE(shared.has_value());
    CHECK(*shared == doctest::Approx(7.91));

    const auto deployed = vsguard::upstream::readVolumeVoxelSize(store);
    REQUIRE(deployed.has_value());
    CHECK(*deployed == doctest::Approx(7.91));

    // Both agree here. This is precisely why the defect went unnoticed: the one
    // live-S3 test in the suite pins this volume.
    CHECK(*deployed == doctest::Approx(*shared));
}

TEST_CASE("DEFECT: the deployed reader ignores the 45.532 um Paris4 store")
{
    const char* document = R"({
      "scan": { "tomo": { "acquisition": { "detector": { "samplePixelSize": 0.045531999999999996 } } } }
    })";
    const fs::path store = makeStore("paris4", "", document);

    const auto shared = vc::metadata::resolveLocalStoreVoxelSize(store);
    REQUIRE(shared.has_value());
    CHECK(*shared == doctest::Approx(45.532));

    CHECK_FALSE(vsguard::upstream::readVolumeVoxelSize(store).has_value());
}

// =============================================================================
// 2. Concern from review of villa#1417: unusable local metadata
//    ("treat an invalid local value as no usable value and still fall back")
// =============================================================================

TEST_CASE("DEFECT: the deployed reader returns a zero voxel size as a value")
{
    // `voxelsize` present and numeric but zero: the deployed reader checks
    // is_number() and nothing else, so it hands back 0.0 as if it were a
    // measurement. Traced to vc_render_tifxyz.cpp:994-995 - the guarding
    // `std::isfinite(*mv) && *mv > 0.0` lives at the *call site* (:1387), not in
    // the reader, so the reader itself cannot distinguish "0 um/voxel" from a
    // real answer.
    const fs::path store = makeStore("zerolocal", R"({"voxelsize": 0})", "");

    const auto deployed = vsguard::upstream::readVolumeVoxelSize(store);
    REQUIRE(deployed.has_value());
    CHECK(*deployed == doctest::Approx(0.0));

    // The shared resolver refuses it, which is the behaviour the fix installs.
    CHECK_FALSE(vc::metadata::resolveLocalStoreVoxelSize(store).has_value());
}

TEST_CASE("DEFECT: the deployed reader returns a negative voxel size as a value")
{
    // Same root cause as the zero case: the reader validates the *type* of the
    // field (is_number) but never its sign, because the sign check lives at the
    // call site. A negative value therefore escapes the reader and only fails
    // later, at :1387, which then reports it as "invalid metadata voxelsize" -
    // losing the distinction between a store that published nonsense and one
    // that published nothing.
    const fs::path store = makeStore("neglocal", R"({"voxelsize": -3})", "");

    const auto deployed = vsguard::upstream::readVolumeVoxelSize(store);
    REQUIRE(deployed.has_value());
    CHECK(*deployed == doctest::Approx(-3.0));

    // The shared resolver rejects it.
    CHECK_FALSE(vc::metadata::resolveLocalStoreVoxelSize(store).has_value());
}

TEST_CASE("a stated but unusable local size is distinguished from none")
{
    // The document states an unusable size. `statedVoxelSize` records that the
    // store tried to answer, which lets the patched renderer tell a corrupt
    // store (loud) from an old one (quiet) - the practical difference the review
    // comment on #1417 was about.
    const StoreVoxelSize stated = vsguard::storeVoxelSize(utils::Json::parse(R"({"voxelsize": -3})"));
    CHECK(stated.statedVoxelSize);
    CHECK_FALSE(stated.micrometerPerVoxel.has_value());

    const StoreVoxelSize zero = vsguard::storeVoxelSize(utils::Json::parse(R"({"voxelsize": 0})"));
    CHECK(zero.statedVoxelSize);
    CHECK_FALSE(zero.micrometerPerVoxel.has_value());

    const StoreVoxelSize silent = vsguard::storeVoxelSize(utils::Json::parse(R"({"height": 10})"));
    CHECK_FALSE(silent.statedVoxelSize);
}

TEST_CASE("an unusable local value does not block the remote fallback")
{
    // The decision the review comment asked for: a present-but-unusable local
    // value must not be treated as "local metadata answered", because then a
    // perfectly good remote value is never consulted.
    const ResolvedVoxelSize resolved = vsguard::resolveVoxelSize(
        ExplicitVoxelSize{},
        /*local*/ std::nullopt,          // unusable -> reported as nothing usable
        /*volume*/ std::nullopt,
        /*remote*/ 45.532,
        1.0);
    CHECK(vsguard::sourceName(resolved) == vsguard::sourceName({0.0, VoxelSizeSource::RemoteMetadata}));
    CHECK(resolved.micrometerPerVoxel == doctest::Approx(45.532));
    CHECK(resolved.isUsable());
}

TEST_CASE("stated detection agrees with the resolver on every alias")
{
    // Guards the duplicated key list in render_voxel_size_resolution.cpp.
    const std::vector<std::pair<const char*, double>> cases = {
        {R"({"voxelsize": 7.91})", 7.91},
        {R"({"voxel_size_um": 7.91})", 7.91},
        {R"({"voxelSizeUm": 7.91})", 7.91},
        {R"({"pixel_size_um": 7.91})", 7.91},
        {R"({"pixelSizeUm": 7.91})", 7.91},
        {R"({"resolution_um": 7.91})", 7.91},
    };
    for (const auto& [document, expected] : cases) {
        CAPTURE(document);
        const StoreVoxelSize stated = vsguard::storeVoxelSize(utils::Json::parse(document));
        CHECK(stated.statedVoxelSize);
        REQUIRE(stated.micrometerPerVoxel.has_value());
        CHECK(*stated.micrometerPerVoxel == doctest::Approx(expected));
    }

    // A document that says nothing must report `stated == false` even though the
    // resolver also returns nothing. Conflating the two is the bug this bit exists
    // to prevent.
    const StoreVoxelSize silent = vsguard::storeVoxelSize(utils::Json::parse(R"({"scan":{}})"));
    CHECK_FALSE(silent.statedVoxelSize);
    CHECK_FALSE(silent.micrometerPerVoxel.has_value());
}

// =============================================================================
// 3. Concern from review of villa#1417: remote URL fragments
//    ("joinRemoteUrlPath() does not strip URL fragments")
// =============================================================================

TEST_CASE("the fragment concern is real in joinRemoteUrlPath")
{
    const std::string locator =
        "https://example.test/PHercParis4/volumes/x.zarr#vc-base-scale=1";

    const std::string joined = vc::joinRemoteUrlPath(locator, "metadata.json");

    // Reproduced, not assumed: the child is appended after the fragment, giving
    // a path that cannot exist.
    CHECK(joined == "https://example.test/PHercParis4/volumes/x.zarr#vc-base-scale=1/metadata.json");

    // The correct request target keeps the fragment off the request entirely.
    const auto spec = vc::parseRemoteVolumeSpec(locator);
    CHECK(spec.sourceUrl == "https://example.test/PHercParis4/volumes/x.zarr");
    CHECK(vc::joinRemoteUrlPath(spec.sourceUrl, "metadata.json") ==
          "https://example.test/PHercParis4/volumes/x.zarr/metadata.json");
}

TEST_CASE("resolveRemoteStoreVoxelSize must be given a fragment-free URL")
{
    // A fetch stub that only answers the genuinely correct URL, which is what a
    // server does.
    static const std::string correct =
        "https://example.test/x.zarr/metadata.json";
    const auto fetch = [](const std::string& url) -> std::string {
        if (url == correct)
            return R"({"scan":{"tomo":{"acquisition":{"detector":{"samplePixelSize":0.045532}}}}})";
        return {};
    };

    const std::string locator = "https://example.test/x.zarr#vc-base-scale=1";

    // The failure mode the review comment warned about: fed the raw locator, the
    // helper requests a path that cannot exist and reports "no voxel size",
    // which is indistinguishable from a store that publishes none.
    const std::string malformed =
        vc::joinRemoteUrlPath(locator, "metadata.json");
    CHECK(malformed != correct);
    const auto viaRawLocator =
        vsguard::resolveRemoteStoreVoxelSize(locator, fetch);
    CHECK_FALSE(viaRawLocator.has_value());

    // Fed the parsed, fragment-free source URL, it succeeds.
    const auto viaSourceUrl = vsguard::resolveRemoteStoreVoxelSize(
        vc::parseRemoteVolumeSpec(locator).sourceUrl, fetch);
    REQUIRE(viaSourceUrl.has_value());
    CHECK(*viaSourceUrl == doctest::Approx(45.532));
}

// =============================================================================
// 4. The patched decision procedure
// =============================================================================

TEST_CASE("priority: an explicit override outranks every metadata source")
{
    const ResolvedVoxelSize resolved = vsguard::resolveVoxelSize(
        ExplicitVoxelSize{true, 7.91, "micrometer"},
        /*local*/ 8.64, /*volume*/ 9.6, /*remote*/ 45.532, 1.0);
    CHECK(vsguard::sourceName(resolved) == vsguard::sourceName({0.0, VoxelSizeSource::Cli}));
    CHECK(resolved.micrometerPerVoxel == doctest::Approx(7.91));
}

TEST_CASE("priority: local metadata outranks remote")
{
    const ResolvedVoxelSize resolved = vsguard::resolveVoxelSize(
        ExplicitVoxelSize{}, /*local*/ 8.64, /*volume*/ 9.6, /*remote*/ 45.532, 1.0);
    CHECK(vsguard::sourceName(resolved) == vsguard::sourceName({0.0, VoxelSizeSource::LocalStoreMetadata}));
    CHECK(resolved.micrometerPerVoxel == doctest::Approx(8.64));
}

TEST_CASE("priority: the open remote volume outranks a separate remote fetch")
{
    // Reusing the value the renderer already holds costs no request and cannot
    // disagree with the volume being streamed.
    const ResolvedVoxelSize resolved = vsguard::resolveVoxelSize(
        ExplicitVoxelSize{}, std::nullopt, /*volume*/ 8.64, /*remote*/ 8.64, 1.0);
    CHECK(vsguard::sourceName(resolved) == vsguard::sourceName({0.0, VoxelSizeSource::RemoteVolume}));
    CHECK(resolved.micrometerPerVoxel == doctest::Approx(8.64));
}

TEST_CASE("the unresolved case is flagged, not passed off as a measurement")
{
    const ResolvedVoxelSize resolved = vsguard::resolveVoxelSize(
        ExplicitVoxelSize{}, std::nullopt, std::nullopt, std::nullopt, 1.0);
    CHECK(vsguard::sourceName(resolved) == vsguard::sourceName({0.0, VoxelSizeSource::Unspecified}));
    CHECK_FALSE(resolved.isUsable());
    CHECK(resolved.micrometerPerVoxel == doctest::Approx(1.0));
}

TEST_CASE("non-finite and non-positive candidates are rejected at every tier")
{
    const double bad[] = {0.0, -1.0, std::nan(""), std::numeric_limits<double>::infinity()};
    for (const double value : bad) {
        CAPTURE(value);
        CHECK_FALSE(vsguard::isUsableMicrometerPerVoxel(value));
        const ResolvedVoxelSize resolved = vsguard::resolveVoxelSize(
            ExplicitVoxelSize{}, value, value, value, 1.0);
        CHECK(vsguard::sourceName(resolved) == vsguard::sourceName({0.0, VoxelSizeSource::Unspecified}));
    }
}

TEST_CASE("--voxel-unit conversion is applied to the CLI value")
{
    SUBCASE("micrometer is the identity")
    {
        CHECK(vsguard::explicitMicrometerPerVoxel({true, 7.91, "micrometer"}) ==
              doctest::Approx(7.91));
    }
    SUBCASE("nanometer")
    {
        CHECK(vsguard::explicitMicrometerPerVoxel({true, 7910.0, "nanometer"}) ==
              doctest::Approx(7.91));
    }
    SUBCASE("millimeter")
    {
        CHECK(vsguard::explicitMicrometerPerVoxel({true, 0.00791, "millimeter"}) ==
              doctest::Approx(7.91));
    }
    SUBCASE("meter")
    {
        CHECK(vsguard::explicitMicrometerPerVoxel({true, 0.00000791, "meter"}) ==
              doctest::Approx(7.91));
    }
    SUBCASE("an unknown unit is refused rather than guessed")
    {
        CHECK_FALSE(vsguard::explicitMicrometerPerVoxel({true, 7.91, "furlong"}).has_value());
    }
    SUBCASE("a non-positive or non-finite value is refused")
    {
        CHECK_FALSE(vsguard::explicitMicrometerPerVoxel({true, 0.0, "micrometer"}).has_value());
        CHECK_FALSE(vsguard::explicitMicrometerPerVoxel({true, -1.0, "micrometer"}).has_value());
        CHECK_FALSE(vsguard::explicitMicrometerPerVoxel(
            {true, std::numeric_limits<double>::quiet_NaN(), "micrometer"}).has_value());
    }
}
