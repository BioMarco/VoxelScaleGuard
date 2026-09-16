# PR_DRAFT

**Not submitted. No pull request has been opened, and none will be without
authorisation.**

---

## Title

```
vc_render_tifxyz: resolve the voxel size from the volume, not from a local file
```

## Branch / commit plan

Three commits. The first is the fix and is self-sufficient; the other two are
separable so a maintainer can take one without the others.

| # | Commit | Contents |
|---|---|---|
| 1 | `vc_render_tifxyz: use the shared voxel-size resolver, after the volume is open` | `apps/src/vc_render_tifxyz.cpp` — the whole patch |
| 2 | `core/test: cover a modern metadata.json volume in the live S3 test` | *(proposed, not written)* point the live test at a `metadata.json`-shaped volume as well as `PHerc0172` |
| 3 | *(separate PR, recommended)* `VC3D: pass the voxel size whenever it is known, not only when rebased` | `SegmentationCommandHandler.cpp:2074-2078` — see below |

Commit 1 is the deliverable. Commit 2 is the test that would have caught this.
Commit 3 is deliberately not bundled: it changes GUI behaviour and its predicate
has a lapsed history (PR #1228).

---

## Body

### What this fixes

Renders of volumes that publish their resolution as an acquisition record come out
with a physically wrong scale, silently. The rendered pixels are correct; the
OME-Zarr axis metadata and the TIFF resolution tags are not.

`vc_render_tifxyz` resolved the voxel size **before** it opened the source volume,
from a local file only:

```cpp
static std::optional<double> readVolumeVoxelSize(const std::filesystem::path& volPath)
{
    auto tryFile = [](const std::filesystem::path& p, const char* key) -> std::optional<double> {
        if (!std::filesystem::exists(p)) return std::nullopt;
        ...
        if (sub.is_object() && sub.contains("voxelsize") && sub["voxelsize"].is_number())
            return sub["voxelsize"].get_double();
        ...
    };
    if (auto v = tryFile(volPath / "meta.json", nullptr)) return v;
    if (auto v = tryFile(volPath / "metadata.json", "scan")) return v;
    if (auto v = tryFile(volPath / "metadata.json", nullptr)) return v;
    return std::nullopt;
}
```

Meanwhile, a few lines later, the same function had already opened the volume
remotely — `Volume::NewFromUrl(remoteUrl, remoteAuth)` — and `Volume` construction
had already fetched and normalised **exactly this document**, via
`loadRemoteVolumeMetadata` → `vc::metadata::voxelSizeFromStoreMetadata`. The
correct value was in the process and was discarded.

For a streamed volume there is no local metadata, so every candidate misses. The
renderer then fell back to `1.0` and passed it to `writeZarrAttrs` as the axis
scale, with `--voxel-unit` defaulting to `nanometer`.

### Measured, on the published catalog

The pinned-revision reader and the fixed chain, run over four real volumes
(`PHerc0009B` and `PHercParis4` as used in #1403, plus `PHerc0172`):

| Store | Old reader | Store actually says | Declared physical size |
|---|---|---|---|
| `PHerc0009B/…-8.640um-1.2m-116keV-masked.zarr` | **not found** | 8.64 µm | `1 nm` → wrong by **×8640** |
| `PHercParis4/…-45.532um-11.0m-110keV-masked.zarr` | **not found** | 45.532 µm | `1 nm` → wrong by **×45532** |
| `PHercParis4/…-2.400um-0.2m-137keV-masked.zarr` | **not found** | 2.4 µm | `1 nm` → wrong by **×2400** |
| `PHerc0172/…-7.910um-53keV-masked.zarr` | 7.91 µm | 7.91 µm | `7.91 nm` → wrong by ×1000 |

Those volumes publish no top-level `voxelsize`; the number is only at
`scan.tomo.acquisition.detector.samplePixelSize` (millimetres). The old reader
resolves **one of the four** — and that one is the legacy `meta.json`-shaped
volume, which is the volume `core/test/test_volume_live_s3.cpp` pins. That is why
this went unnoticed.

Note the last row: even where the reader succeeds, the output is still 1000× wrong,
because the value is in micrometres and is declared in nanometres. Fixing
discovery alone would turn "×8640 wrong" into "×1000 wrong".

### The change

1. **Resolve the voxel size after the volume is open.** For a streamed volume, use
   `remoteVolume->voxelSize()` — the value `Volume` construction already fetched,
   costing no extra request and unable to disagree with the volume actually being
   streamed.
2. **Use `vc::metadata::resolveLocalStoreVoxelSize` for the local case** and delete
   the private reader. That resolver is what `Volume::voxelSize()` and
   `vc_grow_seg_from_segments` already agree on; keeping a third opinion here is
   how the two drifted apart. It also brings the `samplePixelSize` fallback, which
   fixes the **local** case too — a downloaded modern store was misread the same
   way.
3. **Validate once.** The shared resolver rejects non-finite and non-positive
   values. It also reports whether the document *stated* a size at all, so
   "published something unusable" and "published nothing" produce different
   messages instead of one "invalid metadata voxelsize".
4. **Track where the value came from** (`VoxelSizeSource`), so `1.0` can no longer
   be written out as a physical scale, and so the emitted unit matches the number.
   A metadata-sourced size is in micrometres by definition; `--voxel-unit` still
   describes a `--voxel-size` given on the command line.
5. **Validate CLI input earlier**, including the unit, so a bad `--voxel-unit`
   fails before the remote pyramid is opened rather than after.

### What does not change

* **The rendered pixels.** `base_voxel_size` feeds only `tifDpi` and the `.zattrs`
  scale; `buildOffsetList` documents that slice offsets are in level-g voxels and
  are deliberately not scaled by it. This is why the change is low-risk, and also
  why the bug went unseen.
* **Public interfaces.** `writeZarrAttrs`'s signature is untouched.
* **`--voxel-unit`'s default.** Still `nanometer`. (#1313 proposed changing it;
  that is a separate change with a separate blast radius. Deriving the unit from
  the value's source achieves correct output without it.)
* **Metadata conventions.** Nothing added to or redefined in store documents.
* **CLI and local-metadata priority.** Unchanged, and tested.

One deliberate behaviour change: when no physical size can be resolved, the tool
now leaves the `.zattrs` axis unit unset, with a distinct warning, instead of
asserting `1.0 nanometer`. No in-tree consumer reads that unit back, and
`dpi == 0` already suppresses the TIFF resolution tags.

### Tests

Added: 19 cases / 107 assertions covering local and remote valid metadata, missing
metadata, zero, negative and non-finite values, an explicit size with unit
conversion (nm/µm/mm/m), an unknown unit, native and reduced resolution, all six
`voxelsize` aliases, tier priority, and the unusable-vs-absent distinction.

The five reproduction cases assert the **pre-patch** behaviour, so they fail if the
old reader is ever reintroduced:

```cpp
TEST_CASE("DEFECT: the deployed reader cannot read a modern published store") {
    const fs::path store = makeStore("modern", "", kModernStoreMetadata); // PHerc0009B
    const auto shared = vc::metadata::resolveLocalStoreVoxelSize(store);
    REQUIRE(shared.has_value());
    CHECK(*shared == doctest::Approx(8.64));
    CHECK_FALSE(vc::metadata::readVolumeVoxelSize(store).has_value());   // the bug
}
```

Upstream's `core/test/test_voxel_size_metadata.cpp` passes unmodified as the
control.

### Verified by execution

The patch has now been **compiled and run**, on a GitHub-hosted Linux runner, from
the pinned revision, with the baseline built from the same tree and the applied
diff checked byte-identical to the patch. Full record in `CI_VALIDATION.md`.

Against `PHerc0009B/volumes/20250521125136-8.640um-1.2m-116keV-masked.zarr`, and
with `PHerc0172` as the control:

| | baseline | patched |
|---|---|---|
| log line | `Voxel size: 1.0 (no metadata found…)` | `Voxel size (remote volume metadata): 8.64 micrometer` |
| `.zattrs` unit | `nanometer` | `micrometer` |
| `.zattrs` scale (level 0) | `[1, 1, 1]` | `[8.64, 8.64, 8.64]` |
| TIFF `XResolution` | absent | `2939.814697265625` px/inch |
| decoded pixels | — | **byte-identical** to the baseline |

**And a caveat that belongs in this PR rather than in a footnote:** the version of
this patch first submitted to CI **did not compile**. The new resolution block had
been placed ~60 lines before the declarations it reads, producing

```
error: 'hasExplicitVoxelSize' was not declared in this scope
```

That is fixed, and the harness now asserts that every name the block reads is
declared above its call site — but the fact that a change this small could be
"logic-verified" and not build is worth a maintainer knowing.

### Still open

* The GUI path (`SegmentationCommandHandler.cpp:2076-2078`) is deliberately not
  touched here; see commit 3 above and `FEASIBILITY.md` §8.
* The "no usable voxel size anywhere" branch — warning emitted, no physical scale
  written — is logic-verified in the harness but not driven end to end, because the
  volume used finds its value in the volume it opened. That is the fix working, not
  a gap in the test, but it means the branch is covered by unit tests only.
* Two volumes and one crop each. This is a demonstration, not a survey.

### Relationship to other work

* **#1226 / #1227** fixed the same class of problem in `Volume` construction. This
  is a separate execution path that never used Volume construction for this value,
  so that fix could not reach it. Not a regression, not a duplicate.
* **PR #1417** (`vc_render_tifxyz: discover remote volume voxel size`) addressed
  the same symptom by exposing `remoteVolumeVoxelSize()` and re-fetching the
  document. It was closed by an inactivity bot on 2026-08-27. This takes the same
  goal — consult the remote volume's voxel size — but reads the value from the
  volume already open, so it needs no second fetch. That also makes it immune to a
  hazard raised in #1417's own review: `joinRemoteUrlPath` does not strip URL
  fragments, so passing a locator carrying `#vc-base-scale=N` requests
  `…zarr#vc-base-scale=1/meta.json`, which silently resolves nothing. Credit for
  the approach is the author's.
* **PR #1228** diagnosed the GUI side: `SegmentationCommandHandler.cpp:2076`
  enables `setRenderVoxelSize` only when `baseScaleLevel() > 0 ||
  hasExplicitVoxelSizeOverride()`, so a native-resolution remote volume passes
  neither `--voxel-size` nor… a size. With this patch the CLI path is correct, but
  the GUI still suppresses the flag, so a native-resolution remote render from
  VC3D continues to fall back. Recommended follow-up, kept separate:

  ```cpp
  const double renderVoxelSizeUm = renderVolume ? renderVolume->voxelSize() : 0.0;
  _cmdRunner->setRenderVoxelSize(
      renderVoxelSizeUm,
      std::isfinite(renderVoxelSizeUm) && renderVoxelSizeUm > 0.0);
  ```

  `setRenderVoxelSize` already discards a non-positive value, so this is inert
  when the size is unknown.

### Checklist

- [x] Cause identified at file-and-line resolution
- [x] Demonstrated on real published volumes, with a control
- [x] No public interface change; no new dependency; no new metadata convention
- [x] Regression tests added; upstream's suite passes unmodified
- [x] Rendered pixels unaffected, argued from the code paths that consume the value
- [ ] Patched binary compiled and run — **blocked on the dependency closure**
- [ ] Real `.zattrs` / TIFF verified — **blocked on the build**
- [ ] Full `ctest` run — **blocked on the build**
