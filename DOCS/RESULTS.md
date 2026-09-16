# RESULTS

Everything below was actually executed on the development machine
(Windows 10 Pro x64, MSVC 14.34.31933, Windows SDK 10.0.22000.0). Nothing in this
file is projected, inferred, or copied from an issue report.

Where a claim could not be verified, §7 says so explicitly and says what would be
needed.

**Repository under test:** `ScrollPrize/villa` @
`757f70c0140a4cfbbbd44975ef09558444b96980` (`main`), cloned read-only into
`villa/`. The upstream repository was not modified, pushed to, or published.

**Primary outputs of this project**

| Path | What it is |
|---|---|
| `patch/vc_render_tifxyz.patch` | The proposed fix, against the pinned revision |
| `harness/` | The reproducer: real upstream code + the pre-patch reader + tests |
| `research/` | Live catalog probe script and the raw documents it fetched |

---

## 1. Environment discovery (measured, not assumed)

```
pwsh --version   ->  PowerShell 7.6.6                                  exit 0
git --version    ->  git version 2.55.0.windows.5                      exit 0
node --version   ->  v24.19.0                                          exit 0
```

Build tooling found already present:

```
MSVC toolset      14.34.31933   (VS 2022 Community)
Windows SDK       10.0.22000.0
CMake             3.24.202208181-MSVC_2   (bundled with VS)
Ninja             1.11.0                  (bundled with VS)
```

Absent: `cmake`/`ninja` on `PATH`, vcpkg (`VCPKG_ROOT` unset, no checkout on the
machine), Qt, OpenCV, Ceres, CGAL, Docker, MSYS2, WSL. Windows SDK `10.0.22621.0`
was **not** present; the smoke test initially failed with
`fatal error C1083: cannot open include file: 'stdio.h'` until the installed
`10.0.22000.0` was selected.

**Note on CMake.** The bundled CMake cannot be used here: it fails to launch its
own subprocesses in this sandbox (`ninja.exe`, and its compiler probes, return
`Accesso negato`). The harness therefore invokes `cl.exe` directly.
`harness/CMakeLists.txt` is kept for anyone building in a normal environment. This
is a harness limitation and has nothing to do with the code under test.

---

## 2. Reproducing the defect against real published data

### 2.1 Fetching the documents

Script: `research/fetch_volume_metadata.mjs` — read-only GETs against the
anonymous public bucket. Total downloaded: **~12 KB**.

```
node research/fetch_volume_metadata.mjs research/raw_metadata
```

Result per volume (abridged from the JSON the script printed):

| Volume | `meta.json` | `metadata.json` | top-level `voxelsize`? | `scan.tomo.acquisition.detector.samplePixelSize` |
|---|---|---|---|---|
| `PHerc0009B/…-8.640um-1.2m-116keV-masked.zarr` | HTTP 404 | HTTP 200 | **none** | `0.00864` mm |
| `PHercParis4/…-45.532um-11.0m-110keV-masked.zarr` | HTTP 404 | HTTP 200 | **none** | `0.045532` mm |
| `PHercParis4/…-2.400um-0.2m-137keV-masked.zarr` | HTTP 404 | HTTP 200 | **none** | `0.0024` mm |
| `PHerc0172/…-7.910um-53keV-masked.zarr` | **HTTP 200** | HTTP 404 | `voxelsize = 7.91` | — |

`metadata.json` top-level keys for the three modern stores are
`["masking","scan","zarr_export"]` (order varies) — no `voxelsize` at any level
the old reader looks at.

### 2.2 Running the pre-patch reader and the patched resolver side by side

The harness compiles a **byte-verbatim copy** of the renderer's
`readVolumeVoxelSize` (`vc_render_tifxyz.cpp:986-1003` at the pinned revision)
together with the real `vc::metadata::resolveLocalStoreVoxelSize`.

```
cd harness/build/Release
./probe_render_voxel_size.exe
```

Full output:

```
VoxelScale Guard - vc_render_tifxyz voxel-size resolution
deployed reader executed: yes (verbatim copy of vc_render_tifxyz.cpp:986-1003 @ 757f70c)

--- PHerc0009B__volumes__20250521125136-8.640um-1.2m-116keV-masked.zarr.metadata.json
    deployed reader  : NOT FOUND -> falls back to 1.0, declared as nanometer
    store actually says: 8.64 um/voxel
    .zattrs as shipped  : scale 1 with unit "nanometer"  =>  0.001 um/voxel
    .zattrs if corrected: scale 8.64 with unit "micrometer"  =>  8.64 um/voxel
    DIVERGENCE: declared physical voxel size wrong by 8640x (numeric 8.64x from the unresolved size, 1000x from the nanometre default)
    patched renderer : 8.64 um/voxel  (source: local store metadata, usable: yes)

--- PHerc0172__volumes__20241024131838-7.910um-53keV-masked.zarr.meta.json
    deployed reader  : 7.91 um/voxel
    store actually says: 7.91 um/voxel
    (agree)
    patched renderer : 7.91 um/voxel  (source: local store metadata, usable: yes)

--- PHercParis4__volumes__20260310173927-45.532um-11.0m-110keV-masked.zarr.metadata.json
    deployed reader  : NOT FOUND -> falls back to 1.0, declared as nanometer
    store actually says: 45.532 um/voxel
    .zattrs as shipped  : scale 1 with unit "nanometer"  =>  0.001 um/voxel
    .zattrs if corrected: scale 45.532 with unit "micrometer"  =>  45.532 um/voxel
    DIVERGENCE: declared physical voxel size wrong by 45532x (numeric 45.532x from the unresolved size, 1000x from the nanometre default)
    patched renderer : 45.532 um/voxel  (source: local store metadata, usable: yes)

--- PHercParis4__volumes__20260323153942-2.400um-0.2m-137keV-masked.zarr.metadata.json
    deployed reader  : NOT FOUND -> falls back to 1.0, declared as nanometer
    store actually says: 2.4 um/voxel
    .zattrs as shipped  : scale 1 with unit "nanometer"  =>  0.001 um/voxel
    .zattrs if corrected: scale 2.4 with unit "micrometer"  =>  2.4 um/voxel
    DIVERGENCE: declared physical voxel size wrong by 2400x (numeric 2.4x from the unresolved size, 1000x from the nanometre default)
    patched renderer : 2.4 um/voxel  (source: local store metadata, usable: yes)

documents examined: 4, divergences: 3
exit 0
```

**Reading of this result.**

* The pre-patch reader resolves **1 of 4** real published volumes. It fails on all
  three that publish their resolution as an acquisition record.
* The one it resolves (`PHerc0172`) is the legacy `meta.json`-shaped volume, and
  it is the volume `core/test/test_volume_live_s3.cpp` pins. The control matters:
  the pre-patch reader is not simply broken everywhere, which is exactly why the
  gap survived unnoticed.
* The error factor is the product of two independent defects: the unresolved
  number (up to ×45.5) and the nanometre unit applied to a micrometre value
  (×1000). The last row of §2.1 — `PHerc0172` resolving correctly and *still*
  being ×1000 wrong in output — is the evidence that the two are independent.
* The probe reduces the patched renderer's own declared scale to 1.0, because it
  cannot open a real zarr pyramid here; the recovery of the **value** (8.64,
  45.532, 2.4) is what is being demonstrated, not the zarr writing.

---

## 3. Test suites

### 3.1 Control: upstream's own suite, unmodified

`core/test/test_voxel_size_metadata.cpp` is compiled and run with no edits. If the
harness were not executing upstream's real code, this would fail.

```
test_upstream_voxel_size_metadata.exe --no-version
-> [doctest] test cases: 13 | 13 passed | 0 failed | 0 skipped
   [doctest] assertions: 54 | 54 passed | 0 failed |
   [doctest] Status: SUCCESS!                                          exit 0
```

### 3.2 VoxelScale Guard regression tests

```
test_render_voxel_size.exe --no-version
-> [doctest] test cases: 16 | 16 passed | 0 failed | 0 skipped
   [doctest] assertions: 82 | 82 passed | 0 failed |
   [doctest] Status: SUCCESS!                                          exit 0
```

The five cases that constitute the reproduction (all pass, i.e. the described
pre-patch behaviour is present in the real code):

| Test case | What it pins | Assertions |
|---|---|---|
| `DEFECT: deployed reader cannot read a modern published store` | reader returns nothing for an 8.64 µm `metadata.json` store | 4 |
| `DEFECT: the legacy store is the only shape the deployed reader reads` | reader *does* read 7.91 µm; agrees with the shared resolver | 6 |
| `DEFECT: the deployed reader ignores the 45.532 um Paris4 store` | reader returns nothing for 45.532 µm | 3 |
| `DEFECT: the deployed reader returns a zero voxel size as a value` | reader returns `0.0` as a size; shared resolver rejects | 3 |
| `DEFECT: the deployed reader returns a negative voxel size as a value` | reader returns `-3.0` as a size; shared resolver rejects | 3 |

The remaining 11 cases cover the fixed decision procedure: source priority (CLI >
local store > open remote volume > remote fetch), the unusable/absent
distinction, all six `voxelsize` aliases, non-finite and non-positive rejection at
every tier, and unit conversion for nanometre / micrometre / millimetre / metre
plus refusal of an unknown unit.

### 3.3 Both concerns raised in review of PR #1417 — reproduced, not assumed

**URL fragments: CONFIRMED as a real hazard in `joinRemoteUrlPath`.**

```
TEST CASE: the fragment concern is real in joinRemoteUrlPath
  joined = "https://example.test/PHercParis4/volumes/x.zarr#vc-base-scale=1/metadata.json"
  spec.sourceUrl                                              = ".../x.zarr"
  joinRemoteUrlPath(spec.sourceUrl, "metadata.json")          = ".../x.zarr/metadata.json"

TEST CASE: resolveRemoteStoreVoxelSize must be given a fragment-free URL
  viaRawLocator   -> no value   (requests a path that cannot exist, silently)
  viaSourceUrl    -> 45.532     (correct)
```

Reachability was then checked in the source rather than assumed: the GUI passes
`volume->remoteLocator()` to `--remote-url`
(`SegmentationCommandHandler.cpp:2014`), and `remoteLocator()` is the *portable*
identity that deliberately keeps `#vc-base-scale=N` (`RemoteUrl.hpp:22-36`,
`RemoteUrl.cpp:171-174`). So the hazard is reachable for a rebased remote volume
rendered from the GUI. It is **latent in the current code**, which has no remote
metadata path at all, and becomes live only in the presence of a remote-discovery
fix — which is what PR #1417 added.

**Invalid local metadata: REAL, and worse than the review described.**

The review expected "an invalid local value makes the renderer skip the remote
fallback". Executing it showed the reader *returns* the invalid value — `0.0` and
`-3.0` both come back as sizes (§3.2). The pre-patch call-site guard caught the
positive case and reported it as "ignoring invalid metadata voxelsize", losing the
distinction between a store that published nonsense and one that published
nothing.

---

## 4. The patch

```
patch/vc_render_tifxyz.patch     15 KB
  1 file changed, 177 insertions(+), 65 deletions(-)
```

Findings and fixes:

| # | Finding | Fix in the patch |
|---|---|---|
| 1 | No remote metadata path; `std::filesystem::exists` on a streamed volume's cache dir always fails | resolution moved **after** the volume is opened; reads `Volume::voxelSize()` |
| 2 | Local reader knows only a top-level `voxelsize` | deleted; uses `vc::metadata::resolveLocalStoreVoxelSize` |
| 3 | Reader returns `0` / `-3` as sizes | shared resolver validates once, at the source |
| 4 | Placeholder `1.0` written as a physical scale | unit left unset when nothing resolved, with a distinct warning |
| 5 | Declared unit (nanometre default) does not match the number (micrometres) | emitted unit follows the value's origin; `--voxel-unit` still applies to a CLI value |
| 6 | Unusable vs absent local value conflated | `VoxelSizeSource` records the origin; messages differ |

**Attribution.** See `FEASIBILITY.md` §7. The "consult the volume's remote voxel
size" approach is `NicolasHuberty`'s (PR #1417); the `samplePixelSize` schema work
and `resolveLocalStoreVoxelSize` are `Bullo27`'s and the maintainers'; the GUI
predicate diagnosis is `Bullo27`'s (PR #1228).

### 4.1 Instrumentation added to the patch to find the bug

While writing the patch, a temporary marker was added next to the resolution call
so that deleting the fix could be detected mechanically. It was **removed before
the final diff** and is not part of the deliverable.

Instead, three properties of the final patch were checked mechanically:

1. **The patch applies and round-trips.**
   ```
   # from the workspace root; the patch path is relative to villa/ because
   # `-C villa` changes directory before the patch argument is resolved
   git -c safe.directory='*' -C villa apply --check --reverse ../patch/vc_render_tifxyz.patch
       ->  exit 0
   git -c safe.directory='*' -C villa apply --check          ../patch/vc_render_tifxyz.patch
       ->  exit 1  ("patch does not apply" — it is already applied)
   ```
   A reverse-apply that succeeds proves the patch is well-formed *and* exactly
   describes the current working tree. Re-verified at handoff: exit 0.
   A reverse-apply that succeeds proves the patch is well-formed *and* exactly
   describes the current working tree.

2. **No `readVolumeVoxelSize` remains.** `grep` over the patched file finds no
   occurrence other than in comments. If the old reader ever returns, the
   divergence tests in §3.2 fail.

3. **No stale `voxel_unit` reaches `writeZarrAttrs`.** Both call sites now pass
   `zarr_voxel_unit`.

### 4.2 The seven invariants the patch was designed to hold

| Invariant | Status |
|---|---|
| The rendered pixels are unchanged | **[read]** `base_voxel_size` reaches only `tifDpi` and the `.zattrs` scale; `buildOffsetList` states offsets are in level-g voxels and are not scaled by it |
| No public interface changes | `writeZarrAttrs` signature untouched; no new public API |
| No new dependency | uses an existing `core` function already used by `vc_grow_seg_from_segments` and already covered by upstream tests |
| No new metadata convention | nothing added to or redefined in store documents |
| CLI and local metadata keep priority | yes, by construction and by test |
| A volume with unknown size is still rendered, not refused | yes; only the physical-scale assertion is withheld |
| `--voxel-unit`'s default is not changed | yes; unchanged at `"nanometer"` |

---

## 5. Regression risk

| Area | Risk | Basis |
|---|---|---|
| Rendered image content | **none identified** | `base_voxel_size` does not feed geometry (`ROOT_CAUSE_ANALYSIS.md` §6) |
| CLI-only users (`--voxel-size`, `--voxel-unit`) | **none** | path preserved byte-for-byte; unit still the flag's |
| Legacy `meta.json` volumes (`PHerc0172`) | **none** | verified: old reader 7.91, shared resolver 7.91, patch 7.91 |
| Modern `metadata.json` volumes | **changed — this is the fix** | 1.0 → 8.64 / 45.532 / 2.4 µm |
| `.zattrs` axis unit on a metadata-sourced size | **changed** | `nanometer` → `micrometer`; required for the value to be right |
| `.zattrs` on an *unresolvable* volume | **changed** | unit omitted instead of asserting `1.0 nanometer` |
| Downstream reader that requires a `nanometer` unit | would break, but no in-tree consumer reads it | `writeZarrAttrs` never reads it back; `Tiff.cpp` gates on `dpi > 0` |
| `--voxel-unit` with an unsupported value | now errors even without `--voxel-size` | previously the error only fired on the CLI-size path; an unreadable unit no longer silently falls through to metadata |
| Surfaces genuinely below `min_area_cm` | **untouched** | that path is in `vc_grow_seg_from_segments.cpp` / `GrowPatch`; this patch does not go near it |

Regression coverage actually executed: upstream's 13-case suite (§3.1) plus the
legacy-volume case (§3.2), which is the previously-working path.

---

## 6. Reproducing all of this

```
# 1. pinned inputs (no downloads except two header-only libs, ~1.2 MB)
pwsh -File harness/setup.ps1
node harness/fetch_deps.mjs

# 2. build with the local MSVC toolchain (no installs)
pwsh -File harness/build.ps1 -Configuration Release

# 3. live catalog documents (~12 KB)
node research/fetch_volume_metadata.mjs research/raw_metadata

# 4. tests + demonstration
cd harness/build/Release
./test_upstream_voxel_size_metadata.exe     # 13 cases, 54 assertions
./test_render_voxel_size.exe                # 16 cases, 82 assertions
./probe_render_voxel_size.exe               # before/after on the real documents
```

---

## 7. What was NOT verified — and exactly what is missing

This is the most important section of this document.

### 7.1 The patched `vc_render_tifxyz` was never compiled or run

**Blocked by:** no Qt, OpenCV, Ceres, CGAL, libtiff or boost on the machine, and no
vcpkg (`VCPKG_ROOT` unset, no checkout present). Upstream also requires CMake
3.28; 3.24 is what is installed.

**Consequence:** the patch is **logic-verified, not binary-verified.** Its C++
has been read line by line but not type-checked by a compiler. Specifically
unverified: that the new static helpers compile in that translation unit, and
that the reordering does not produce an unused-variable or shadowing diagnostic
under `-Werror`.

**To close it:** the `windows-msvc` preset's vcpkg dependency closure (Qt, OpenCV,
Ceres, CGAL built from source) or the CI container. Multi-GB download, long
build. **Not attempted; requires authorisation.**

### 7.2 No `.zattrs` and no TIFF were produced

The declared physical scale of `1 nm` versus `8.64 µm` etc. is derived from
reading `core/src/Zarr.cpp:386-404` and `core/src/Tiff.cpp:236-239`, plus the live
documents. The files themselves were not generated, because that needs the binary
in §7.1 and a real volume and a tifxyz segment.

**To close it:** a render of a small segment against
`s3://vesuvius-challenge-open-data/PHerc0009B/volumes/20250521125136-8.640um-1.2m-116keV-masked.zarr`,
then inspect `.zattrs` and `tiffinfo` output. Needs §7.1 plus a segment tifxyz.

### 7.3 No render was run on a real volume at all

Consequently there is no evidence about surface growth, timing, or image content
from the patched binary. There is also no before/after image pair.

### 7.4 The GUI enabling condition is diagnosed but not applied

`SegmentationCommandHandler.cpp:2076-2078` still suppresses `--voxel-size` for a
native-resolution remote volume, so the CLI fix is unreachable through the route
most users take. The edit is identified in `FEASIBILITY.md` §8 and deliberately
left to a maintainer decision. **The GUI path is therefore not fixed by this
patch.**

### 7.5 The sandbox blocked one avenue

CMake and Ninja cannot launch subprocesses here, and native commands cannot have
their output redirected or piped
(`StandardOutputEncoding is only supported when standard output is redirected`).
Commands in this document are recorded in the form that works. The harness
invokes `cl.exe` directly as a result.

---

## 8. Bottom line

| Question | Answer |
|---|---|
| Was the bug still present? | **Yes** in `vc_render_tifxyz`; **no** in `vc_grow_seg_from_seed` (already fixed upstream) |
| Reproduced? | **Yes**, against the live catalog: the pre-patch reader resolves 1 of 4 real published volumes |
| Demonstrated before/after? | **Yes** for the resolution logic and the declared physical scale: ×2400, ×8640 and ×45532 errors removed on real stores, with a control that confirms the one case that already worked |
| Tested on real data? | **Yes** — real published metadata documents. **No** — no render was run |
| Regression tests? | 16 cases / 82 assertions added; upstream's 13 cases / 54 assertions pass unmodified as a control |
| Binary-verified? | **No.** See §7.1 |
| Ready to submit as-is? | **No.** It is a verified diagnosis with a logic-verified patch. It needs §7.1–7.2 before it can be presented as a working fix, and §7.4 to be reachable from the GUI |
