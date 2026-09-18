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
//
// LICENCE — THIS FILE IS TREATED AS GPL-3.0-or-later, NOT MIT.
//   The tests are original, but they include the verbatim upstream reader and the
//   Volume Cartographer headers, and are compiled together with those translation
//   units. Derivative-or-combined is not determinable here, so the conservative
//   treatment is applied. See LICENSE-GPL-3.0.txt and NOTICE.md sections 2.1 and
//   2.4. Not covered by the MIT licence in LICENSE. (doctest itself is MIT and is
//   fetched at build time, not distributed here.)

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

#include "vsguard/patch_integrity.hpp"
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

TEST_CASE("priority: the open volume outranks the local store document")
{
    // The opened volume is what is actually being streamed, and its metadata was
    // freshly normalised during construction. --volume is often a chunk cache for
    // a remote source rather than the store itself; where such a cache mirrors the
    // store's metadata it can be stale, so the streamed volume wins.
    const ResolvedVoxelSize resolved = vsguard::resolveVoxelSize(
        ExplicitVoxelSize{}, /*local*/ 8.64, /*volume*/ 9.6, /*remote*/ 45.532, 1.0);
    CHECK(vsguard::sourceName(resolved) == vsguard::sourceName({0.0, VoxelSizeSource::RemoteVolume}));
    CHECK(resolved.micrometerPerVoxel == doctest::Approx(9.6));
}

TEST_CASE("priority: the local store document is used when no volume is open")
{
    const ResolvedVoxelSize resolved = vsguard::resolveVoxelSize(
        ExplicitVoxelSize{}, /*local*/ 8.64, std::nullopt, /*remote*/ 45.532, 1.0);
    CHECK(vsguard::sourceName(resolved) == vsguard::sourceName({0.0, VoxelSizeSource::LocalStoreMetadata}));
    CHECK(resolved.micrometerPerVoxel == doctest::Approx(8.64));
}

TEST_CASE("priority: a local document does not pre-empt the open volume")
{
    // Stated as its own case because the opposite order was implemented once: a
    // stale local mirror could then win over the freshly fetched document.
    const ResolvedVoxelSize resolved = vsguard::resolveVoxelSize(
        ExplicitVoxelSize{}, /*local*/ 1.0, /*volume*/ 8.64, std::nullopt, 1.0);
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

    // What the renderer must hand writeZarrAttrs in this case. It must be 0 --
    // "unknown", which makes writeZarrAttrs omit the physical scale -- and NOT
    // the placeholder, which would be published as a measurement of 1.0 in some
    // unit. The placeholder stays only as the struct's initialiser.
    const double declaredWhenUnusable =
        resolved.isUsable() ? resolved.micrometerPerVoxel : 0.0;
    CHECK(declaredWhenUnusable == doctest::Approx(0.0));

    // And a zero base size is what writeZarrAttrs treats as "omit"; this is the
    // contract the renderer relies on.
    CHECK(vsguard::voxelSizeToDpi(declaredWhenUnusable) == doctest::Approx(0.0));
}

TEST_CASE("an unusable local value does not block the open volume")
{
    // local 0 / negative / non-finite must fall through, not resolve.
    for (const double bad : {0.0, -3.0, std::numeric_limits<double>::quiet_NaN()}) {
        CAPTURE(bad);
        const ResolvedVoxelSize resolved = vsguard::resolveVoxelSize(
            ExplicitVoxelSize{}, bad, /*volume*/ 8.64, std::nullopt, 1.0);
        CHECK(vsguard::sourceName(resolved) == vsguard::sourceName({0.0, VoxelSizeSource::RemoteVolume}));
        CHECK(resolved.micrometerPerVoxel == doctest::Approx(8.64));
    }
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

// =============================================================================
// 4b. The .zattrs number/unit pair, in physical terms
// =============================================================================
//
// These assert what a reader of the output would conclude, not what code ran.
// The defect they exist for: the first version of this patch converted a CLI value
// to micrometres and then wrote that number under the caller's unit label, so
// `--voxel-size 8640 --voxel-unit nanometer` declared 8.64 nm instead of 8640 nm --
// a silent 1000x error, in the very output this project exists to correct.

namespace {

// What a .zattrs reader multiplies: the declared number times the declared unit.
double physicalMicrometers(const ResolvedVoxelSize& resolved,
                           const ExplicitVoxelSize& explicitSize)
{
    const std::string unit = vsguard::zarrUnit(resolved, explicitSize.unit);
    if (unit.empty()) return 0.0;
    const auto perUnit = vsguard::micrometersPerUnit(unit);
    if (!perUnit) return 0.0;
    return vsguard::zarrScaleValue(resolved, explicitSize) * *perUnit;
}

} // namespace

TEST_CASE("a CLI size means the same physical size in every supported unit")
{
    // 8640 nm, 8.64 um, 0.00864 mm and 0.00000864 m are the same physical size, so
    // all four must resolve identically AND declare identical physical scale.
    struct Case { double value; const char* unit; };
    const Case cases[] = {
        {8640.0,        "nanometer"},
        {8.64,          "micrometer"},
        {0.00864,       "millimeter"},
        {0.00000864,    "meter"},
    };

    for (const Case& c : cases) {
        const ExplicitVoxelSize explicitSize{true, c.value, c.unit};
        CAPTURE(c.value);
        CAPTURE(c.unit);

        // The internal value is micrometres and must agree across all four.
        const auto um = vsguard::explicitMicrometerPerVoxel(explicitSize);
        REQUIRE(um.has_value());
        CHECK(*um == doctest::Approx(8.64));

        const auto resolved = vsguard::resolveVoxelSize(explicitSize, std::nullopt,
                                                       std::nullopt, std::nullopt);
        CHECK(resolved.micrometerPerVoxel == doctest::Approx(8.64));

        // And so must what .zattrs declares, once the unit is applied.
        CHECK(physicalMicrometers(resolved, explicitSize) == doctest::Approx(8.64));
    }
}

TEST_CASE("the .zattrs number and its unit always describe the same physical size")
{
    // The invariant zarrScaleValue()/zarrUnit() exist to maintain. This is the
    // assertion that would have failed on `--voxel-size 8640 --voxel-unit
    // nanometer` before the fix: number 8.64 under unit "nanometer" = 8.64 nm.
    struct Case { double value; const char* unit; double expectedUm; };
    const Case cases[] = {
        {8640.0,     "nanometer",     8.64},
        {8.64,       "micrometer",    8.64},
        {0.00864,    "millimeter",    8.64},
        {0.00000864, "meter",         8.64},
        {8640.0,     "nm",            8.64},   // accepted short spellings
        {8.64,       "um",            8.64},
        {0.00864,    "mm",            8.64},
        {0.00000864, "m",             8.64},
        {7.91,       "micrometer",    7.91},
        {7910.0,     "nanometer",     7.91},
    };

    for (const Case& c : cases) {
        const ExplicitVoxelSize explicitSize{true, c.value, c.unit};
        const auto resolved = vsguard::resolveVoxelSize(explicitSize, std::nullopt,
                                                       std::nullopt, std::nullopt);
        CAPTURE(c.value);
        CAPTURE(c.unit);
        CAPTURE(c.expectedUm);

        const std::string unit = vsguard::zarrUnit(resolved, explicitSize.unit);
        REQUIRE_FALSE(unit.empty());

        // The caller's own number is kept, so the pair stays exactly as supplied.
        CHECK(vsguard::zarrScaleValue(resolved, explicitSize) == doctest::Approx(c.value));
        CHECK(unit == std::string(c.unit));

        // ...and it denotes the physical size the caller asked for.
        CHECK(physicalMicrometers(resolved, explicitSize) == doctest::Approx(c.expectedUm));
    }
}

TEST_CASE("a size read from metadata is declared in micrometers")
{
    // The metadata path resolves micrometres, so both the number and the unit are
    // micrometres whatever --voxel-unit says: the flag describes a value the caller
    // supplied, and here they supplied none.
    const ExplicitVoxelSize none{};
    for (const char* unit : {"nanometer", "micrometer", "millimeter", "m"}) {
        const auto resolved = vsguard::resolveVoxelSize(none, /*local*/ 8.64,
                                                       std::nullopt, std::nullopt);
        ExplicitVoxelSize withUnit{};
        withUnit.unit = unit;
        CAPTURE(unit);
        // Compare via toString(): doctest cannot stringify a bare scoped enum.
        CHECK(std::string(vsguard::toString(resolved.source)) ==
              std::string(vsguard::toString(vsguard::VoxelSizeSource::LocalStoreMetadata)));
        CHECK(vsguard::zarrUnit(resolved, withUnit.unit) == std::string("micrometer"));
        CHECK(vsguard::zarrScaleValue(resolved, withUnit) == doctest::Approx(8.64));
        CHECK(physicalMicrometers(resolved, withUnit) == doctest::Approx(8.64));
    }
}

TEST_CASE("the TIFF resolution agrees with the declared .zattrs size")
{
    // Both outputs must describe the same physical size. The TIFF tag is derived
    // from the internal micrometre value; .zattrs from the declared pair. They are
    // computed from different numbers, so this is worth asserting rather than
    // assuming -- it is the second half of the same defect.
    struct Case { double value; const char* unit; };
    const Case cases[] = {
        {8640.0,     "nanometer"},
        {8.64,       "micrometer"},
        {0.00864,    "millimeter"},
        {0.00000864, "meter"},
        {7910.0,     "nanometer"},
    };

    for (const Case& c : cases) {
        const ExplicitVoxelSize explicitSize{true, c.value, c.unit};
        const auto resolved = vsguard::resolveVoxelSize(explicitSize, std::nullopt,
                                                       std::nullopt, std::nullopt);
        CAPTURE(c.value);
        CAPTURE(c.unit);

        // At --scale 1 and ds_scale 1, one output pixel spans exactly one voxel.
        const double umPerPixel = resolved.micrometerPerVoxel;
        const double dpi = vsguard::voxelSizeToDpi(umPerPixel);
        REQUIRE(dpi > 0.0);

        // Recover the physical pixel size from the TIFF tag alone...
        const double umFromTiff = 25400.0 / dpi;
        // ...and from .zattrs alone.
        const double umFromZarr = physicalMicrometers(resolved, explicitSize);

        CAPTURE(umFromTiff);
        CAPTURE(umFromZarr);
        CHECK(umFromTiff == doctest::Approx(umFromZarr).epsilon(1e-9));
        CHECK(umFromTiff == doctest::Approx(resolved.micrometerPerVoxel).epsilon(1e-9));
    }
}

TEST_CASE("an unusable size declares nothing, in either output")
{
    const ResolvedVoxelSize unresolved{};   // source Unspecified
    const ExplicitVoxelSize none{};
    CHECK_FALSE(unresolved.isUsable());
    CHECK(vsguard::zarrUnit(unresolved, "nanometer").empty());
    CHECK(vsguard::voxelSizeToDpi(0.0) == doctest::Approx(0.0));
    CHECK(vsguard::voxelSizeToDpi(-1.0) == doctest::Approx(0.0));
}

// =============================================================================
// 5. The committed patch itself, as an artefact
// =============================================================================
//
// Everything above tests the decision procedure. These test the file that is
// actually shipped for review. They exist because the patch was "logic-verified"
// and, on first compilation in CI, did not compile: the new block used
// variables declared sixty lines below it. See patch_integrity.hpp.

TEST_CASE("the pristine copy and the patched tree are the before and after states")
{
    const auto root = vsguard_patch::repoRoot();
    REQUIRE_FALSE(root.empty());

    const auto pristine =
        vsguard_patch::readWholeFile(vsguard_patch::pristineTargetFile(root));
    const auto patched =
        vsguard_patch::readWholeFile(vsguard_patch::patchedTargetFile(root));
    REQUIRE_FALSE(pristine.empty());
    REQUIRE_FALSE(patched.empty());

    // The pristine copy must really be the unpatched revision, and the working
    // tree must really be the patched one. Without both, the ordering check below
    // would be measuring nothing.
    CHECK(pristine.find("readVolumeVoxelSize") != std::string::npos);
    CHECK(pristine.find("resolveRenderVoxelSize") == std::string::npos);
    CHECK(patched.find("readVolumeVoxelSize") == std::string::npos);
    CHECK(patched.find("resolveRenderVoxelSize") != std::string::npos);
}

TEST_CASE("the patched translation unit declares every name it uses before using it")
{
    // This is the defect the first CI compile found. It is asserted against the
    // real patched file, so it cannot silently return.
    const auto root = vsguard_patch::repoRoot();
    REQUIRE_FALSE(root.empty());

    const auto patched =
        vsguard_patch::readWholeFile(vsguard_patch::patchedTargetFile(root));
    REQUIRE_FALSE(patched.empty());
    const auto lines = vsguard_patch::splitTextLines(patched);

    // Anchor on the CALL SITE inside main, not on the helper's definition. The
    // call reads
    //     const ResolvedVoxelSize resolved = resolveRenderVoxelSize(
    //         vol_path, remoteVolume.get(), ...
    // and the definition's signature never mentions vol_path.
    const std::size_t callUse = vsguard_patch::lineOfCallSite(
        lines, "resolveRenderVoxelSize(", "vol_path");
    CAPTURE(callUse);
    REQUIRE(callUse > 0);

    const char* names[] = {
        "hasExplicitVoxelSize",
        "explicitVoxelSize",
        "base_voxel_size",
        "render_level_voxel_size",
        "zarr_voxel_unit",
    };
    for (const char* name : names) {
        const std::size_t decl = vsguard_patch::declarationLineOf(lines, name);
        CAPTURE(name);
        CAPTURE(decl);
        REQUIRE(decl > 0);
        CHECK(decl < callUse);
    }

    const std::size_t voxelUnitDecl =
        vsguard_patch::firstLineContaining(lines, "const std::string voxel_unit =");
    CAPTURE(voxelUnitDecl);
    REQUIRE(voxelUnitDecl > 0);
    CHECK(voxelUnitDecl < callUse);
}

TEST_CASE("fixture: the pre-fix ordering is rejected by the same check")
{
    // Guard the guard. If this logic cannot tell the broken ordering from the
    // fixed one, it is not testing anything. This fixture is the shape the patch
    // had when CI rejected it, quoted from the compiler's own error.
    const std::vector<std::string> broken = {
        "int main() {",
        "    {",
        "        const ResolvedVoxelSize resolved = resolveRenderVoxelSize(",
        "            vol_path, remoteVolume.get(), hasExplicitVoxelSize, explicitVoxelSize,",
        "            voxel_unit);",
        "    }",
        "    const std::string voxel_unit = parsed[\"voxel-unit\"].as<std::string>();",
        "    bool hasExplicitVoxelSize = false;",
        "    double explicitVoxelSize = 0.0;",
        "    double base_voxel_size = 1.0;",
        "    double render_level_voxel_size = 1.0;",
        "    std::string zarr_voxel_unit = voxel_unit;",
        "}",
    };
    const std::size_t use = vsguard_patch::firstLineContaining(broken, "resolveRenderVoxelSize(");
    const std::size_t decl = vsguard_patch::declarationLineOf(broken, "hasExplicitVoxelSize");
    REQUIRE(use > 0);
    REQUIRE(decl > 0);
    CHECK(decl > use);   // the old ordering violates the property
}
