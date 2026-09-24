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

> **Status update, 2026-09-16 (second session).** §7.1, §7.2 and §7.3 are now
> **CLOSED** by execution — see §9. The entries below are kept verbatim as the
> record of what was open and why, because that record is what made the gap
> checkable; §7.1 in particular is now known to have understated the risk (the
> patch did not merely risk a warning, it did not compile).
> §7.4 and §7.5 remain open.

This is the most important section of this document.

### 7.1 The patched `vc_render_tifxyz` was never compiled or run — **CLOSED 2026-09-16**

*Read the text below as the state before 2026-09-16. It is kept verbatim because
it is what made the gap checkable, and because it turned out to be too optimistic:
the compile it called unverified did not merely warn, it failed.*

**Blocked by:** no Qt, OpenCV, Ceres, CGAL, libtiff or boost on the machine, and no
vcpkg (`VCPKG_ROOT` unset, no checkout present). Upstream also requires CMake
3.28; 3.24 is what is installed.

**Consequence (as then stated):** the patch is **logic-verified, not
binary-verified.** Its C++ has been read line by line but not type-checked by a
compiler. Specifically unverified: that the new static helpers compile in that
translation unit, and that the reordering does not produce an unused-variable or
shadowing diagnostic under `-Werror`.

**What actually happened:** the new block was inserted before the declarations it
reads, so the file did not compile at all. See §9.1. The two risks named above were
the right ones; the outcome was worse than "a diagnostic".

**To close it:** the `windows-msvc` preset's vcpkg dependency closure (Qt, OpenCV,
Ceres, CGAL built from source) or the CI container. Multi-GB download, long
build. **Not attempted; requires authorisation.**

> **Update, 2026-09-16 — the reason above was incomplete; see §8.4.** The closure
> being absent is one cause. A second, independent cause is that neither CMake nor
> Ninja can execute a compiler in this environment, so a CMake-driven build is not
> reachable here even once the dependencies are present. §8.4 also records that the
> target's *link* closure excludes Ceres, CGAL and Qt — those are configure-time
> requirements of the project as a whole, not of this binary.

### 7.2 No `.zattrs` and no TIFF were produced — **CLOSED 2026-09-16**

The declared physical scale of `1 nm` versus `8.64 µm` etc. is derived from
reading `core/src/Zarr.cpp:386-404` and `core/src/Tiff.cpp:236-239`, plus the live
documents. The files themselves were not generated, because that needs the binary
in §7.1 and a real volume and a tifxyz segment.

**To close it:** a render of a small segment against
`s3://vesuvius-challenge-open-data/PHerc0009B/volumes/20250521125136-8.640um-1.2m-116keV-masked.zarr`,
then inspect `.zattrs` and `tiffinfo` output. Needs §7.1 plus a segment tifxyz.

### 7.3 No render was run on a real volume at all — **CLOSED 2026-09-16**

Consequently there is no evidence about surface growth, timing, or image content
from the patched binary. There is also no before/after image pair.

### 7.4 The GUI enabling condition is diagnosed but not applied

`SegmentationCommandHandler.cpp:2076-2078` still suppresses `--voxel-size` for a
native-resolution remote volume, so the CLI fix is unreachable through the route
most users take. The edit is identified in `FEASIBILITY.md` §8 and deliberately
left to a maintainer decision. **The GUI path is therefore not fixed by this
patch.**

### 7.5 The sandbox blocked one avenue

CMake and Ninja cannot execute a compiler here, and native commands cannot have
their output redirected or piped
(`StandardOutputEncoding is only supported when standard output is redirected`).
Commands in this document are recorded in the form that works. The harness
invokes `cl.exe` directly as a result.

> **Update, 2026-09-16 — this entry was accurate but under-specified; see §8.4.**
> Both tools *do* run and report their versions. What fails is CMake's child-process
> probe of Ninja (`Accesso negato`) and Ninja's execution of a build rule (it
> hangs). The distinction matters because it rules out "install a newer CMake" as a
> fix on its own. §8.4 also records that Node's `spawnSync` fails with `EPERM` for
> piped stdio regardless of the program — the same trap in a second guise.

---

## 8. Session of 2026-09-16 — inherited-state re-verification, and the build attempt

Added by the continuation session. This section records three things: the
re-verification of the inherited state, a documentation error that was corrected,
and the **negative** result of the build reconnaissance. §7 is retained as the
authoritative list of what is unverified; two of its entries (§7.1 and §7.5) carry
an update pointer to §8.4 with no claim withdrawn.

### 8.1 Inherited-state checks — all four pass [exec]

Run from the workspace root, exactly as `RESUME.md` §1 prescribes.

| Check | Command | Result |
|---|---|---|
| Repository clean | `pwsh -File tools\git.ps1 status --short` | no output — clean |
| Working tree vs. remote | `pwsh -File tools\git.ps1 rev-parse HEAD origin/main` | both `f2945b97e7e2b2d9e9a91deeb20c998c36c72643` |
| Control suite | `test_upstream_voxel_size_metadata.exe` | **13 cases / 54 assertions, 0 failed** |
| Project suite | `test_render_voxel_size.exe` | **16 cases / 82 assertions, 0 failed** |
| Probe | `probe_render_voxel_size.exe` | `documents examined: 4, divergences: 3` |
| Patch round-trip | `git -c safe.directory='*' -C villa apply --check --reverse ../patch/vc_render_tifxyz.patch` | exit **0** |

Notes on the two checks where the inherited document was slightly out of date —
both are benign and neither changes a finding:

* `RESUME.md` §1 predicted the log head `9cfcaf9, 6de80df, 4a77205`. The actual head
  is `f2945b9`, which is the commit that **added `DOCS/RESUME.md` itself**, on top of
  the predicted `9cfcaf9`. The tree is clean and in sync with `origin/main`; the
  prediction was simply written before its own commit landed.
* `RESUME.md` §1 says the probe should show "3 divergences out of 4 volumes" and the
  binary prints exactly that.

The patch diffstat was re-derived from the committed artefact rather than quoted:
**+177 / −65 across one file**
(`volume-cartographer/apps/src/vc_render_tifxyz.cpp`), which matches `RESUME.md` §2
and `PROJECT_STATUS.md`.

**Corrected 2026-09-18: this measurement is a point-in-time record, not the current
state.** The patch became **three files, +250/−65** on 2026-09-17 (§11.5). The
paragraph above is left as written because it is what was measured on 2026-09-16,
and because the three documents it says "match" were still propagating the one-file
figure on 2026-09-18 — including `AGENTS.md` §4, where it was load-bearing (§12.1).
Where a number and the committed artefact disagree, the artefact wins:
`git -C villa apply --numstat ../patch/vc_render_tifxyz.patch`.

### 8.2 A documentation error: the Progress Prize deadline had not passed

Four documents (`README.md`, `DOCS/PRIZE_REQUIREMENTS.md` §1.1 and §4.1,
`DOCS/SUBMISSION_DRAFT.md`, `DOCS/RESUME.md` §9) claimed that the stated Progress
Prize deadline of **11:59pm Pacific, September 30th, 2026** "has passed".

**That was wrong.** Re-fetched [live] on 2026-09-16 from
<https://scrollprize.org/prizes>, the page still states verbatim:

> "Submissions are evaluated monthly, and multiple submissions/awards per month are
> permitted. The next deadline is 11:59pm Pacific, September 30th, 2026!"

The date is **14 days in the future**, not past. The page had not changed; the
reading of it was wrong, and the error had propagated to four files. All four are
corrected in place and the correction is recorded here rather than deleted
(per `AGENTS.md` §11). The surrounding factual content of §1.1 — the Progress Prize
structure, the amounts, the evaluation cadence — was checked against the same fetch
and stands.

Also re-verified in the same fetch: the **2027 Grand Prize** and **First Letters**
deadlines are **June 25th, 2027** [live]. They are irrelevant to this project, which
is a tooling contribution and not a reading of a scroll, but they were previously
not stated at all.

### 8.3 Attribution audit on the "segmentation loss" issue

`README.md` and `DOCS/SUBMISSION_DRAFT.md` were checked for any claim that this
project fixed the `vc_grow_seg_from_seed` segmentation-loss half of issue #1403.

**Result: no such claim exists, and no edit was needed.** Every mention is already
explicit that the fix is upstream's and not this project's:

* `README.md`: *"The `vc_grow_seg_from_seed` half of issue #1403 is already fixed
  upstream. … This project did not fix it and does not claim it."*
* `DOCS/PROJECT_STATUS.md` finding 6, `DOCS/ROOT_CAUSE_ANALYSIS.md` §1 and §2,
  the bottom-line table in §9 below, `DOCS/PRIZE_REQUIREMENTS.md` §3.
* The PR draft and `DOCS/RESEARCH.md` §4.1 both record it as a **negative** result,
  which is the convention `AGENTS.md` §11 asks for.

The attribution table required by `AGENTS.md` §5 (NicolasHuberty #1417; Bullo27
#1227/#1228 and the maintainers; DarthCeltic #1403) is present and correct in
`README.md` and `DOCS/SUBMISSION_DRAFT.md`.

### 8.4 The build reconnaissance — negative result, and exactly what blocks it

This session's goal was to close the main gap in §7: compile the patched
`vc_render_tifxyz` and demonstrate before/after on a real volume. **The compile
could not be achieved, and it is not a matter of effort.** The evidence:

**Blocker A — CMake cannot launch its own subprocesses here, and Ninja hangs when
a build rule does.**

An earlier draft of this section claimed that *no* CMake version could be driven
from this sandbox, on the strength of a probe that had itself been defeated by the
harness's output-redirection bug (`AGENTS.md` §7.1). That claim was too strong and
is corrected here; the honest picture is narrower and still blocking.

| Attempt | Result |
|---|---|
| `cmake --version` | works — `cmake version 3.24.202208181-MSVC_2`, exit 0 |
| `ninja --version` | works — `1.11.0`, exit 0 |
| `cmake -S <dir> -B <dir>\build -G Ninja …` on a project declaring `cmake_minimum_required(VERSION 3.28)` | fails **on the version check**, as documented: `CMake Error at CMakeLists.txt:1 (cmake_minimum_required): CMake 3.28 or higher is required.  You are running version 3.24.202208181-MSVC_2` |
| the same on a project declaring `VERSION 3.20`, i.e. version-independent | **fails deeper**, exit 1: `CMake Error at CMakeLists.txt:2 (project): Running '<…>\ninja.exe' '--version' failed with: Accesso negato`, then `CMake Error: CMAKE_CXX_COMPILER not set, after EnableLanguage` |
| `ninja -C <dir>` on a hand-written `build.ninja` whose only rule runs `cmd /c copy` | **hangs**: exit status never arrives; killed at a 30 s bound (earlier hand runs: 120 s and 45 s). The rule's output file **is** produced, so the rule body runs and the hang is in Ninja's process handling. `ninja --version` alone, which executes no rule, returns normally |

The first four data rows come from `node research/recon_compiler_spawn.mjs`, added
this session. The Ninja row is that script (`ninja --version` returns, a rule
hangs) plus two manual runs at longer bounds, 120 s and 45 s, which behaved the
same way.

So both tools start and report their versions, but neither can *execute a compiler
or a command* in this sandbox: CMake's probe of Ninja is denied, and Ninja's own
rule execution hangs. This is the behaviour `AGENTS.md` §7.2 records, confirmed
again, and it is what forces `harness/build.ps1` to drive `cl.exe` in a loop
directly — which does work, and worked again this session.

**One methodological caveat, because it already produced a wrong answer here.**
Every command in this probe captures output through **files**, never pipes. This
sandbox denies piped stdio between processes: Node's `spawnSync` with piped stdio
fails with `EPERM` for *every* program, including `cl.exe` and `git.exe`, and
PowerShell fails with `StandardOutputEncoding is only supported when standard output
is redirected` (`AGENTS.md` §7.1). A probe that pipes therefore reports "blocked"
for a reason unrelated to the tool it is measuring. An earlier draft of this section
drew a stronger conclusion than the evidence supported from exactly that mistake;
it is corrected here.

Two consequences worth separating, because they have different fixes:

* The **CMake version gap is real and is a download**. Upstream requires
  `cmake_minimum_required(VERSION 3.28 FATAL_ERROR)`
  ([read] `volume-cartographer/CMakeLists.txt:1`) and every preset is
  `"generator": "Ninja"` ([read] `volume-cartographer/CMakePresets.json:9`). A
  portable CMake ≥ 3.28 would close *this* half.
* It would **not** close the other half. A newer CMake still has to launch Ninja
  and `cl.exe`, and that is what is denied. So a CMake-driven build is not
  reachable here even after that download.

**A related correction.** The wider claim first drawn here from that mismeasured
probe — that this sandbox "refuses to launch executables from Program Files" — is
**false**, and the harness contradicts it: `harness/build.ps1` invokes `cl.exe` from
`C:\Program Files\Microsoft Visual Studio\2022\Community\…` on every build,
including this session's. Two places still carry that wording as an inherited
comment rather than as a verified finding, and both are recorded here rather than
quietly edited, per `AGENTS.md` §11:

* `.gitignore:7-9` justifies ignoring `harness/tools/ninja.exe` with "this sandbox
  refuses to launch executables from Program Files". The file is ignored, but that
  reason is wrong — and nothing in the repository references
  `harness/tools/ninja.exe`, so the ignore entry appears vestigial.
* `harness/build.ps1:9-13` explains that "CMake cannot launch its own subprocesses
  (\"Accesso negato\" for both ninja.exe and, from inside CMake, any probe)". The
  file's *conclusion* — invoke `cl.exe` directly — is right and was re-confirmed
  this session; only the words "and ninja.exe" are imprecise, since Ninja itself
  runs and instead hangs when executing a rule.

Neither file was changed beyond the `.gitignore` addition below, because both are
still correct in effect and `harness/build.ps1` is on the verified-harness path.
`scratch/` was added to `.gitignore` this session to keep throwaway probe trees and
any downloaded package out of the repository.

**Blocker B — the dependency closure is not present, and it is the binding
constraint.**

`cl.exe` *can* be driven directly from PowerShell — that is what `harness/build.ps1`
does. The compile-time closure of the real translation unit, however, is absent.
From the `#include` directives of `vc_render_tifxyz.cpp` and its direct
dependencies [read]:

| Needed at compile time | Needed by | Present locally |
|---|---|---|
| `<opencv2/imgproc.hpp>`, `<opencv2/core/mat.hpp>` | `vc_render_tifxyz.cpp:25` and `:24` @ `757f70c`; `core/include/vc/core/util/Tiff.hpp:3`; `core/include/vc/core/types/Volume.hpp:15` | **No** |
| `<tiffio.h>` | `vc_render_tifxyz.cpp:45` @ `757f70c`; `core/include/vc/core/util/Tiff.hpp:8` | **No** |
| `<boost/program_options.hpp>` | `vc_render_tifxyz.cpp:32` @ `757f70c` | **No** |
| `<omp.h>` | `vc_render_tifxyz.cpp:46` @ `757f70c` | yes (MSVC ships it) |

*Line numbers for `vc_render_tifxyz.cpp` are the **pre-patch** ones, i.e. the pinned
revision's, consistent with `AGENTS.md` §11. In the patched working tree they are
17/16, 35, 23 and 36 respectively — the patch adds 8 lines above them.*

Beyond the translation unit, `core/CMakeLists.txt` builds `vc_core` from **73**
sources including `Volume.cpp`, `Tiff.cpp`, `Zarr.cpp`, `S3AuthFallback.cpp`,
`HttpFetch.cpp`, `render/ZarrChunkFetcher.cpp` [read], and `vc_flattening` links
`vc_core` and `OpenABF` ([read] `core/CMakeLists.txt:152`). None of these artefacts
exists locally, so merely parsing the file would not produce a runnable renderer
even if the headers were stubbed.

The closure was audited this session, and one result **narrows** the problem in a
way that matters for choosing a route [read]. The full audit — every `find_package`,
`FetchContent`, `add_subdirectory` and `target_link_libraries` traced for this
target — is preserved at `research/vc_render_tifxyz_build_analysis.md`:

* `apps/CMakeLists.txt:17-18` links `vc_core vc_flattening Boost::program_options
  TIFF::TIFF`.
* **Ceres, CGAL and Qt are *not* in the target's link closure at all.** Ceres is
  linked only by `vc_inpaint`/`vc_lasagna`/`vc_atlas`/`vc_tracer`
  (`core/CMakeLists.txt:155,189,196,216`), CGAL only by `vc_add_ignore_label`
  (`apps/CMakeLists.txt:102`), Qt only by VC3D
  (`apps/VC3D/CMakeLists.txt:365-384`). The CLI reaches no Qt header.
* But they are **hard configure-time requirements anyway**:
  `find_package(Ceres REQUIRED)` is unconditional
  (`volume-cartographer/CMakeLists.txt:529`), CGAL and Qt are gated on
  `VC_BUILD_APPS` (`:665-667`, `:517-518`), and that option is ON by default
  (`:158`) with `apps/CMakeLists.txt:8` unconditionally adding the Qt GUI. **There
  is no switch that builds the CLI tools without the GUI.** So the CMake project
  cannot be configured with a reduced closure; the earlier framing of this gap as
  "the `windows-msvc` vcpkg closure (Qt, OpenCV, Ceres, CGAL)" is right about the
  *configure* cost, and the target's own *link* closure is smaller.
* What a hand-rolled `cl.exe` build would actually need: compile `vc_core` (its
  `add_library` block at `core/CMakeLists.txt:3-54` lists **50** source files) plus
  `utils`, `vc_flattening`, the vendored `c3d`, and the renderer itself; then link
  OpenCV (core, imgproc, imgcodecs, calib3d, video), libtiff, Boost
  `program_options`, libcurl, zlib, blosc, zstd, lz4 and a `vc_delta3d` codec.
  Eigen and nlohmann-json are header-only. Two network fetches are unavoidable even
  then: `vc-delta3d` v0.1.0 (`CMakeLists.txt:686-697`, genuinely included via
  `core/include/vc/core/util/CacheCompression.hpp:3`) and libigl (header-only,
  pulled only because `VC_BUILD_APPS` is ON). **None of it is present.**

**Corollary worth stating: the patch's own new code needs none of it.** The added
functions use only `<optional>`, `<string>`, `<filesystem>`, `<cmath>` and the
in-tree `vc::metadata::resolveLocalStoreVoxelSize`
([read] `patch/vc_render_tifxyz.patch`, added lines 21–139; declaration at
`core/include/vc/core/util/VoxelSizeMetadata.hpp:42`). This is consistent
with the fix's design — no new dependency — but it does **not** make the file
compilable, because the *existing* translation unit already required OpenCV, libtiff
and Boost.

**Blocker C — the sanctioned prebuilt dependency bundle is not fetchable
anonymously.** MSYS2 is not installed (`C:\msys64` absent) [exec], `VCPKG_ROOT` is
unset and no vcpkg checkout exists [exec], and there is no WSL distribution
(`wsl --status` exits 50; `wsl --list --verbose` exits 1 with usage text) [exec].

The route upstream CI uses is `ci-windows-mingw` driven by `cmake --preset` and
`ninja` ([read] `.github/workflows/vc3d-windows.yml:87,92`) plus
`oras pull ghcr.io/scrollprize/vc3d-deps/windows:sha-d84dfa07…` (`:73`). Two
independent obstacles: it lands on Blocker A, and the bundle itself is **not
publicly readable**. An anonymous `ghcr.io/token` request for
`repository:scrollprize/vc3d-deps:pull` returns **HTTP 403** [live], so its size and
contents cannot even be measured from here, let alone pulled. (The runner supplies
`${{ github.token }}` at `:72`, which is how CI authenticates.)

By contrast the same probe shows `ghcr.io/scrollprize/villa/volume-cartographer:edge`
**is** anonymously readable: `linux/amd64`, 41 layers, **2636 MB compressed**
[live]. That image is a built runtime, but the `edge` tag is built from current
`main` — not from the pinned commit — so it is neither the `before` nor the `after`
side of this patch, and running a Linux binary needs Docker or WSL, which are
absent. The probe is reproducible as `node research/recon_ghcr_bundles.mjs`.

**A route that was considered and rejected on evidence.** One idea was to stub the
missing third-party headers (`opencv2/...`, `tiffio.h`, `boost/program_options.hpp`)
so that the *patched regions* could at least be syntax-checked by the compiler. It
was tested and does not work: MSVC's preprocessor copies macro definitions out of
included headers without parsing the header body, so a deliberately malformed header

```cpp
// header.hpp
#pragma once
#define BROKEN 1 +
```

compiles cleanly when the macro is never used, and the syntax error only surfaces
when it is. (`cl /c b.cpp` where `b.cpp` merely includes `header.hpp` → exit 0.)
A stub-based "compile" would therefore validate an arbitrary subset of the
translation unit and could report success on code that does not compile. That is
precisely the kind of claim `AGENTS.md` §2 forbids, so the route was abandoned
rather than reported as a partial success.

### 8.5 The "before" side: package re-confirmed, not yet downloaded

The prebuilt Windows package that `RESUME.md` §5 and `PROJECT_STATUS.md` finding 10
describe was re-verified against the live GitHub releases API [live] — metadata
only, nothing downloaded:

```
RELEASE latest | published 2026-09-15T17:56:17Z | name "VC3D Latest"
   ASSET VC3D-757f70c-2026-09-15-win64.zip    155637402 bytes (148.4 MB)
   ASSET VC3D-757f70c-2026-09-15-win64.exe    110333029 bytes (105.2 MB)
   ASSET VC3D-757f70c-2026-09-15-linux-x86_64.AppImage  124905976 bytes (119.1 MB)
   ASSET VC3D-757f70c-2026-09-15-macos-arm64.zip        117046516 bytes (111.6 MB)
   body mentions commit: 757f70c0140a4cfbbbd44975ef09558444b96980
```

So the asset exists, is 148.4 MB, and the release body names the pinned commit
`757f70c…` exactly. The command is now reproducible as
`node research/recon_release_assets.mjs` (public REST API, anonymous, read-only,
metadata only — it downloads no asset).

**This was not downloaded, at the user's standing instruction to ask first.** It
also does not, by itself, close §7.1: it yields the **before** transcript from a real
shipped binary, which is genuinely useful, but no `after`, because the after side
requires the patched binary to exist. For completeness: the Linux `AppImage` is
119.1 MB and would be the more convenient before-side artefact *if* a Linux
environment existed, which it does not (§8.4 Blocker C).

### 8.6 §7 items closed, and items still open

Nothing in §7 is closed by this session. Stated explicitly:

| §7 item | Status after 2026-09-16 |
|---|---|
| 7.1 patched binary never compiled or run | **Still open.** Two independent causes: the dependency closure is absent (Blocker B), and neither CMake nor Ninja can execute a command here (Blocker A). A newer CMake fixes only the version half |
| 7.2 no `.zattrs`, no TIFF tag dump | **Still open.** Depends on 7.1 |
| 7.3 no render on a real volume | **Still open** |
| 7.4 GUI predicate diagnosed but not applied | **Still open by decision** — `RESUME.md` §6, unchanged |
| 7.5 sandbox blocked an avenue | **Still open, and now precisely characterised** (§8.4): CMake's child-process probe and Ninja's rule execution are denied; driving `cl.exe` directly is not |

The one thing this session did close is a *documentation* gap: the deadline claim in
§8.2 was wrong and is now correct.

---

## 9. Session of 2026-09-16 (second) — the renderer compiled, run, and compared

**This section closes §7.1, §7.2 and §7.3.** It is the first execution-based
verification of the fix, and it began by disproving the patch.

Everything below was produced by GitHub Actions run
[35137825520](https://github.com/BioMarco/VoxelScaleGuard/actions/runs/35137825520)
on `ubuntu-24.04`, building
`ScrollPrize/villa` @ `757f70c0140a4cfbbbd44975ef09558444b96980`. The workflow is
`.github/workflows/renderer-validation.yml`; the full record is in
`DOCS/CI_VALIDATION.md`.

### 9.1 The patch did not compile — and that is the headline

The patch as committed in `f2945b9` **could never have built**. The first real
compile produced:

```
vc_render_tifxyz.cpp:1449:43: error: 'hasExplicitVoxelSize' was not declared in this scope
vc_render_tifxyz.cpp:1450:13: error: 'voxel_unit' was not declared in this scope
```

and the same for `explicitVoxelSize`, `base_voxel_size`, `zarr_voxel_unit` and
`render_level_voxel_size`. The new resolution block had been inserted about sixty
lines **before** the declarations it reads.

This is precisely the risk §7.1 recorded as unverified — *"that the new static
helpers compile in that translation unit, and that the reordering does not produce
an unused-variable or shadowing diagnostic"*. It was not a diagnostic; it was a
hard error. §7 was right to keep it open, and wrong to frame it as a warning risk.

**Fixed** by moving the block to immediately after the declarations block. No logic
changed. Diffstat went from +177/−65 to **+176/−63**; `git apply --check --reverse`
exits 0, and a reverse-then-forward round trip reproduces the same file
byte-for-byte.

### 9.2 The harness was blind to it, and was itself contaminated

Two harness defects were found while fixing this:

1. **The "pristine" copy was patched.** `harness/setup.ps1` copies upstream files
   from `villa/`, which is deliberately modified in place to hold the patch. So
   `harness/src/villa/apps/src/vc_render_tifxyz.cpp` — labelled pristine, and used
   as the "before" side — actually contained the patch. It now comes from
   `git show <commit>:<path>`, i.e. the committed blob. Verified: the copy's SHA-256
   equals the pinned blob's, and it contains `readVolumeVoxelSize` and not
   `resolveRenderVoxelSize`.
2. **Nothing compiled the patched file**, because its closure (OpenCV, libtiff,
   Boost) is absent locally. The class of defect is now checked without that
   closure: a test asserts the pristine copy is unpatched and the working tree is
   patched, and that every name the resolution block reads is declared *above* its
   call site. A fixture reproducing the pre-fix ordering proves the check can tell
   the two apart. It cannot prove the file compiles; only the build can.

Harness after the fix [exec]: upstream control **13 cases / 54 assertions** pass
unmodified; this project **27 cases / 208 assertions** pass; probe unchanged at
4 documents, 3 divergences.

### 9.3 Both binaries built from one commit

| Step | Result |
|---|---|
| villa checked out at the pinned commit, worktree asserted clean | [exec] |
| Baseline `vc_render_tifxyz` built (`QuickBuild`, `-O0`, gcc 13.3.0, cmake 3.31.6, ninja 1.13.2) | [exec] |
| Patch applied with `git apply` to that same tree | [exec] |
| Applied diff byte-identical to `patch/vc_render_tifxyz.patch` (`PATCH_IDENTICAL=yes`) | [exec] |
| Patched `vc_render_tifxyz` built | [exec] |
| `--help` identical between the two binaries | [exec] |

The patch is the only difference between the two binaries, and the workflow checks
that rather than assuming it.

### 9.4 Before and after on a real published volume

One invocation per binary, identical inputs and parameters, shared cache
directory. Public inputs: `PHerc0009B/volumes/20250521125136-8.640um-1.2m-116keV-masked.zarr`
with its own mesh `20250510172639-on-20250521125136-8.64um.tifxyz`, and the legacy
`PHerc0172` volume as the control. 128×128 crop, one slice, remote streaming.

**The log line, from the real binaries** [exec]:

> The patched log format below is from the *first* patched build. §10 changed the
> message so that it states the size exactly as `.zattrs` declares it, which makes
> the log checkable against the output. The values are the same; the wording of the
> patched line now reads `Voxel size (<source>): <number> <unit>`, so the metadata
> rows read `Voxel size (remote volume metadata): 8.64 micrometer` before and after.

| Run | Log line | Exit |
|---|---|---|
| 0009B baseline | `Voxel size: 1.0 (no metadata found; override with --voxel-size)` | 0 |
| 0009B patched | `Voxel size (remote volume metadata): 8.64 micrometer` | 0 |
| 0172 baseline | `Voxel size: 1.0 (no metadata found; override with --voxel-size)` | 0 |
| 0172 patched | `Voxel size (remote volume metadata): 7.91 micrometer` | 0 |

**The `.zattrs` each binary wrote** [exec], scale at dataset `0` (group `-g 0`):

| Run | unit | scale | declared physical voxel size |
|---|---|---|---|
| 0009B baseline | `nanometer` | `[1, 1, 1]` | 0.001 µm — wrong by **×8640** |
| 0009B patched | `micrometer` | `[8.64, 8.64, 8.64]` | 8.64 µm — correct |
| 0172 baseline | `nanometer` | `[1, 1, 1]` | 0.001 µm — wrong by **×7910** |
| 0172 patched | `micrometer` | `[7.91, 7.91, 7.91]` | 7.91 µm — correct |

Higher pyramid levels scale correctly too: `0009B-patched` level 5 is
`[8.64, 276.48, 276.48]` µm, i.e. 8.64 × 32.

Both rows of the baseline are the *unresolved* case, not the "legacy volume already
works" case: with no store document on the local filesystem, the shipped reader
finds nothing on either volume. That is the defect, reproduced from the shipped
behaviour.

**The TIFF tags** [exec]:

| Run | XResolution | ResolutionUnit |
|---|---|---|
| 0009B baseline | **absent** | absent |
| 0009B patched | **2939.814697265625** | 2 (inch) |
| 0172 baseline | **absent** | absent |
| 0172 patched | **3211.125244140625** | 2 (inch) |

`25400 / 8.64 = 2939.8148…` and `25400 / 7.91 = 3211.1252…`, so the emitted
resolution is the correct physical scale to six significant figures. The baseline
wrote no resolution tag at all — it silently omitted the physical scale rather
than stating a wrong one, on this path.

### 9.5 The regression check: the rendered pixels are unchanged

| Volume | Decoded pixel SHA-256 | Verdict |
|---|---|---|
| 0009B | `7d92aec8…` on both sides | **identical** |
| 0172 | `4fe7b59a…` on both sides | **identical** |

The **file bytes differ** (14244 vs 14296 for 0009B), because the patched TIFF
carries the resolution tag the baseline omitted. That is why the verdict is a hash
of the *decoded pixel array*: comparing file sizes or file digests would have
reported this fix as a regression. `ci/compare_render_outputs.py` reports both and
labels which one is the check; `ci/selftest_compare.py` exercises that distinction
against synthetic fixtures.

No error or warning line was emitted by any of the four runs.

### 9.6 What this does and does not establish

**Established [exec]:** the patched renderer compiles; it runs on a real published
volume from the public catalog; it resolves the correct voxel size where the
shipped binary resolved nothing; the declared OME-Zarr physical scale and the TIFF
resolution tag become correct; the rendered pixels are byte-identical.

**Not established:** anything about GUI reachability (§7.4 stands — the patch does
not touch `SegmentationCommandHandler`), and correctness on volumes whose metadata
is genuinely absent, which this volume cannot exercise because the patched binary
finds the value in the volume it opened. Error handling of unusable *inputs* is
exercised separately (§9.7).

### 9.7 Error and edge cases

*Filled in from the same workflow's edge-case step; see `DOCS/CI_VALIDATION.md` §8
for the exact invocations and outputs.*

### 9.8 The "before" side from the shipped binary is now redundant

`RESUME.md` §5 proposed downloading the 148 MB prebuilt Windows package to obtain
the "before" transcript. That is no longer necessary for this purpose: the
baseline **is** the shipped behaviour, built from the same commit, and its transcript
above comes from a real run rather than a downloaded binary. The package remains
useful only as an independent cross-check that the shipped artefact behaves as the
source says.

---

## 10. Session of 2026-09-16 (third) — a 1000x unit regression the patch itself introduced

**Reported by the user after reviewing the published run, reproduced here, and
fixed.** Session §9 found that the patch did not compile. This one found that, once
compiling, it wrote a *wrong physical number* on the explicit-size path — the same
class of defect the project exists to correct.

### 10.1 The defect

`--voxel-size 8640 --voxel-unit nanometer` wrote:

| | value |
|---|---|
| `.zattrs` scale at level 0 | `8.64` |
| `.zattrs` axis unit | `nanometer` |
| **what that denotes** | **8.64 nm** |
| what the caller asked for | **8640 nm** |

A silent ×1000 error. The TIFF tag was correct at the same time, so the two output
formats disagreed with each other as well as with the request [exec].

**Cause.** `explicitMicrometerPerVoxel()` converts a CLI pair to micrometres;
`base_voxel_size` then carried the converted number while `zarr_voxel_unit` kept the
caller's unit label. `writeZarrAttrs` writes the number and the unit independently —
`Zarr.cpp:386-404` builds `scale` and `axes[*].unit` from separate arguments — and
nothing checks that they agree.

**It is a regression introduced by this patch, not a pre-existing bug.** The
pre-patch renderer wrote the caller's raw number under the caller's unit
(`8640 nanometer`) and used the converted value only for the TIFF tag, so both
outputs denoted 8640 nm. `--voxel-unit` has therefore always meant *"the unit of the
number I am giving you"*, and the patch broke that meaning. Confirmed by reading the
pre-patch source, `harness/src/villa/apps/src/vc_render_tifxyz.cpp:1397-1418` [read].

### 10.2 The fix

`zarrVoxelUnit()`/`zarrVoxelValue()` replace `kiloMicrometerUnit()` and compute the
number and the unit **together**, so they cannot disagree:

* explicit `--voxel-size` — the caller's own number and unit are preserved, exactly
  as before the patch. **No option semantics changed.**
* store metadata / the open volume — the resolved value is micrometres, so both are
  micrometres. This is the original fix and it is untouched.

`base_voxel_size` stays in micrometres for the TIFF resolution, so the TIFF tag is
unchanged. `render_level_voxel_size` is now derived from the declared pair, which is
what makes every pyramid level's scale agree with its unit.

### 10.3 Tests that check physical values, not exit codes

The previous tests could not have caught this: they checked exit codes, tier
priority and the resolved micrometre value — all of which were correct. The number
*and* its unit were each fine; only the pair was wrong.

Five cases, 91 assertions added. The load-bearing one is the invariant
`zarrScaleValue(...) × micrometersPerUnit(zarrUnit(...)) == micrometerPerVoxel`,
checked for nanometer, micrometer, millimeter and meter in long and short spellings,
plus:

* four spellings of one physical size resolving and declaring identically;
* the TIFF tag and `.zattrs` independently recovering the same physical size;
* metadata-sourced sizes declared in micrometres whatever `--voxel-unit` says;
* an unusable size declaring nothing.

**Verified that the tests catch the defect** [exec]: reintroducing it in the model
fails 3 cases / 21 assertions, reporting values such as `0.00864` where `8.64` was
expected for the millimetre case — the ×1000 made visible.

### 10.4 CI now checks the physical size end to end

`ci/check_physical_size.py` converts `.zattrs` and the TIFF tags to micrometres and
compares **both** against what the command line asked for, failing the job if either
disagrees or if they disagree with each other. It is driven from the edge-case step
for nanometer, micrometer, millimeter and meter, and for a second physical size
(7.91 µm) so it does not merely assert a constant. `ci/selftest_physical_size.py`
covers the checker itself — including that it **rejects** `8.64` labelled
`nanometer` — and runs inside `ci/preflight_workflow.py` before any push.

### 10.5 What this says about the project's process

Two sessions, two defects, both found only by executing:

1. the patch did not compile (§9.1);
2. once it compiled, it wrote a wrong physical scale on one path (§10.1).

Both were invisible to reading, to the harness as it then stood, and to a
"logic-verified" label. The second was found by a human reviewing the published
output, not by the tooling. The lesson worth keeping: a number and its unit are a
single physical quantity and have to be tested as one.

---

## 11. Session of 2026-09-17 — pre-PR adversarial review, and three more fixes

Ahead of proposing the patch upstream, it was reviewed adversarially against the
pristine revision and the writer it calls. The review found three should-fix issues.
**All three were reproduced before being changed**, and all three are fixed.

### 11.1 The unresolved branch published a fabricated measurement

`writeZarrAttrs()` writes the per-axis `scale` unconditionally and makes only the
axis `unit` conditional (`core/src/Zarr.cpp:374` vs `:391-394`). So when no voxel
size could be resolved, the render still emitted
`coordinateTransformations.scale = [1, 2, 4, …]` — the same numbers `main` emits —
while the new warning told the operator *"no physical scale will be written"*. The
message was false, and a placeholder measurement survived.

**Fixed by making "unknown" mean unknown.** A non-positive `baseVoxelSize` now makes
`writeZarrAttrs()` declare no physical size, and the renderer passes 0 in that case.
The TIFF is untouched: `tifDpi` stays 0, which already means "do not set the
resolution tags".

> **This fix was itself wrong, and §16 records the correction.** As first written it
> omitted the entire `multiscales` block. Reviewer finding 1 on
> [PR #1831](https://github.com/ScrollPrize/villa/pull/1831) showed that this costs
> the image's discovery metadata, so the block is now kept and only the *physical
> claim* is removed. The table below is what that revision produced; §16 has the
> current behaviour. Both rows are kept, because the change from "no multiscales" to
> "unitless multiscales" is the substance of the correction.

Verified end to end [exec], run 35249590299, by rendering a purely LOCAL store that
has no `metadata.json` and no remote marker, so nothing can resolve a size:

| | `.zattrs` keys | TIFF |
|---|---|---|
| baseline | `… multiscales …` with `nanometer` / `[1, 1, 1]` | no resolution tag |
| patched (first revision, now superseded) | `['canvas_size', 'chunk_size', 'note_axes_order', 'num_slices', 'slice_step', 'source_group', 'source_zarr']` — **no multiscales** | no resolution tag |

The baseline row is the fabricated measurement: it declares 1 nm. The patched row
declares nothing — but also loses the level list, which is why it was wrong.
`check_physical_size.py --expect-unknown` now asserts the opposite of what the
superseded row shows: the block must be **present** and unitless.

Note on getting there: the **first two attempts at that check passed vacuously** and
were caught by reading the logs. An empty cache directory made the binary exit at
`Error opening local zarr: … cannot detect version` before resolving anything; then
a cache carrying `.remote_source.json` streamed the real store, whose own document
states 8.64 µm, so the branch was never entered. The step now asserts the warning
text is present and that no cached remote source was used, so it cannot pass
without exercising the branch.

### 11.2 The warning's own advice produced a 1000x error

It said *"Pass `--voxel-size` (um)"*, but `--voxel-unit` defaults to `nanometer`, so
following it literally yields a self-consistent 7.91 **nm**. The message now names
the unit: `--voxel-size <value> --voxel-unit micrometer`. The default itself is
deliberately unchanged — reversing it is a documented separate decision
(`RESUME.md` §6).

### 11.3 The opened volume was consulted *after* the local document

`resolveRenderVoxelSize()` checked the local store document before
`remoteVolume->voxelSize()`, while its own comment justified the opposite on the
grounds that the open volume "cannot disagree with the volume actually being
rendered". `--volume` is frequently a chunk cache for a remote source, and such
cache directories carry the store's root metadata
(`docs/remote_file_cache.md:197-201`), so a **stale local mirror could win** over the
document `Volume` construction had just fetched.

**Fixed by reordering**: explicit CLI → the open volume → the local document, when no
volume is open. Precedence tests updated, with a new case asserting that a local
document does not pre-empt the open volume.

### 11.4 Also from the review

* the unreachable re-check inside the CLI tier is gone, with a comment stating it is
  **not** a fallback for an unusable `--voxel-size` (the caller hard-errors first);
* `--voxel-unit`'s help text said "Physical unit for OME-Zarr axes", which stopped
  being true once sizes are declared in the unit their source implies. It now
  describes what the flag does. The default is unchanged;
* the pure-whitespace hunk and a comment falsely claiming an "upstream logging test"
  asserts the log format were removed (`main`'s logging test asserts only error
  diagnostics: `--segmentation required`, `Error opening local zarr:` and two
  option-validation messages).

### 11.5 Scope, and the state after the review

The patch is now **three files**: the renderer, plus `core/src/Zarr.cpp` and
`core/include/vc/core/util/Zarr.hpp` for the omit-when-unknown contract. All three
are byte-identical between the pinned revision and upstream `main` at `2dcfaf6`
[exec], and the patch applies to `main` cleanly.

| Check | Result |
|---|---|
| Patch reverse-applies against the pinned revision | exit 0 |
| Patch applies to upstream `main` (`2dcfaf6`) | exit 0 |
| Harness: upstream control | 13 cases / 54 assertions pass, unmodified |
| Harness: this project | **27 cases / 208 assertions** pass |
| Probe over the real documents | 4 documents, 3 divergences — unchanged |
| Reporter self-tests | `compare_render_outputs` 4/4, `check_physical_size` 11/11 |
| CI end to end | run 35249590299 — success, 25/25 steps |

---

## 12. Session of 2026-09-18 — the licence inventory, and four documentation defects it found

No code changed in this session. It was the documentation and evidence pass before a
submission, and it produced **one substantive correction and three procedural ones**,
all found by executing checks rather than by reading prose.

### 12.1 The patch-regeneration command in `AGENTS.md` §4 was wrong, and would have corrupted the artefact [exec]

`AGENTS.md` §4 documented:

```
git -c safe.directory='*' diff --output=../patch/vc_render_tifxyz.patch \
    -- volume-cartographer/apps/src/vc_render_tifxyz.cpp
```

That pathspec names **one** file. The patch has covered **three** since §11.5. Run
from inside `villa/`, exactly as documented, it wrote a **18,480-character** patch
whose only `diff --git` header is the renderer's — against the committed artefact's
**20,559 characters** (blob `c4c1a99`, 20,564 bytes) and three headers. It then
*reverse-applied cleanly*, because it correctly described the two thirds it
mentioned. So the documented verification would have reported success while having
replaced the committed patch with a 2/3 patch.

| Invocation | Characters | `diff --git` headers |
|---|---|---|
| As documented (one path) | 18,480 | 1 |
| All three paths | 20,559 | 3 — byte-identical to the row below (`git hash-object` both `c4c1a99`) |
| Committed `patch/vc_render_tifxyz.patch` | 20,559 | 3 |

Fixed by naming all three paths, with the correction and its date recorded inline.
The workflow now also asserts `PATCH_FILES == 3`
(`.github/workflows/renderer-validation.yml`, "Apply patch to the pinned revision"),
so this cannot regress silently even if the comment drifts again. While adding that,
a second gap in the same step was closed: `PATCH_IDENTICAL=no` was only **printed**,
never asserted, so a mismatch between the applied diff and the committed patch would
have left a green run whose "patched" binary was built from a different change. Both
are now `exit 1`, and `ci/preflight_workflow.py` checks the committed artefact's file
count on every run.

An earlier probe of this same command appeared to produce **0** files: that run
passed `--output=../patch/...` to `git -C villa`, which resolves the output path
against the *current* directory, not against `-C`'s. The documented form is
`cd villa` first, which is what was reproduced above. Both facts are worth keeping
because they are two different ways the same check can lie.

### 12.2 "`villa` is MIT" was false for every file this project touches [read]

The licence inventory read the checkouts and found:

| Path | Licence |
|---|---|
| `villa/LICENSE` (root) | MIT, Copyright (c) 2024 Vesuvius Challenge |
| `villa/volume-cartographer/LICENSE` | **GNU GPL version 3**, Copyright (C) 2023 EduceLab |
| `villa/volume-cartographer/NOTICE` | GPL-3.0-or-later |
| `villa/volume-cartographer/Dockerfile:10` | `LABEL org.opencontainers.image.licenses="GPL-3.0"` |

All three patched files, all eight files `harness/setup.ps1` copies, and the three
subprojects `harness/src/vsguard/` links against are inside that subtree. `README.md`,
`AGENTS.md`, `DOCS/PRIZE_REQUIREMENTS.md`, `DOCS/RESEARCH.md` and
`DOCS/PROGRESS_PRIZE_CHECKLIST.md` all asserted or relied on "villa is MIT", and
`PRIZE_REQUIREMENTS.md` used it to argue the prize's licence condition was satisfied.
Each is corrected where it stood, with the correction dated; the full inventory is
`LICENSING_PROPOSAL.md`. **This does not affect the patch or the PR** — upstream *is*
the GPL project, so contributing to it is contributing under its own terms.

One thing the same inventory established in the project's favour, **by accident
rather than design**: `harness/setup.ps1` re-creates the eight upstream files, and
they are **not tracked in git** — `.gitignore`'s `villa/` pattern (no leading
slash) ignores a `villa` directory at any depth. So a clone redistributes no GPL
source. Recorded so it can be made deliberate; it has since been, by a comment above
the pattern naming this reason.

### 12.3 The prize rules were misread in two places [live]

The prizes page and the submission form were both re-fetched on 2026-09-18.

* The deadline *"11:59pm Pacific, September 30th, 2026"* is still current, and the
  Grand Prize deadline is June 25th 2027. Confirmed, not assumed.
* The sentence *"To qualify, you must have registered on the Vesuvius Challenge
  Discord at the time of the submission"* sits under the **2027 Grand Prize**'s
  Additional terms, not under the Progress Prizes. `PROGRESS_PRIZE_CHECKLIST.md` had
  presented it as a Progress Prize requirement.
* The Terms say *"you have to make it open source under a permissive license to
  accept the prize"* — a condition of **accepting** an award, not of submitting. The
  GPL question in §12.2 therefore does not block an entry.
* The Progress Prizes form has **six required fields, one optional field and a
  consent checkbox**, and **no upload**. The four-part question in its long field is
  now answered in `SUBMISSION_DRAFT.md` section by section, and mapped in
  `PROGRESS_PRIZE_CHECKLIST.md` §4.1.

### 12.4 Third-party data is redistributed without its required attribution [read]

Four Open Data bucket metadata documents are committed verbatim under
`research/raw_metadata/`, and `DOCS/evidence/before-after.png` embeds renders of two
bucket volumes. The published datasets are **CC BY-NC 4.0** unless otherwise noted,
which requires attribution on redistribution. No file in the repository stated this.
`research/raw_metadata/PROVENANCE.md` now records each document's URL, byte count and
SHA-256 and the terms; `DOCS/evidence/README.md` states the same beside the figures;
`LICENSING_PROPOSAL.md` §4 lists it as an open action item. **It is still not in a
`NOTICE`/`LICENSE`**, because the repository has no licence file yet by decision.

### 12.5 One code change, and it was forced by a new file

Adding `research/raw_metadata/PROVENANCE.md` (§12.4) changed the probe's output: it
iterated the whole directory, so the new Markdown file became a fifth "document" and
printed `SKIP: not valid JSON`, and the summary read **"documents examined: 5"**
where every document in the set says four. `harness/tools/probe_render_voxel_size.cpp`
now filters to `*.json` regular files and reports **4 documents, 3 divergences**
again — re-built and re-run, exit 0 [exec].

That is the whole of this session's code change. Nothing in the patch, the workflow's
build steps, or the test suites was touched.

### 12.6 The assertions were executed, not just written [exec]

Run **35374993168** (2026-09-18, `workflow_dispatch`, commit `953d5f6`,
conclusion **success**, 25 steps, none failed) is the first run on the strengthened
step. Its log contains, verbatim:

```
PATCH_FILES=3
PATCH_IDENTICAL=yes
physical-size failures: 0
      DECODED PIXELS IDENTICAL: YES   <-- the regression check   (×2)
```

with no `FATAL` line anywhere, and the render lines reading

```
Voxel size: 1.0 (no metadata found; override with --voxel-size)      # baseline
Voxel size (remote volume metadata): 8.64 micrometer                 # patched
Voxel size: 1.0 (no metadata found; override with --voxel-size)      # baseline
Voxel size (remote volume metadata): 7.91 micrometer                 # patched
```

The unit-variant cases all resolved to the requested physical size
(`8640 nanometer`, `8.64 micrometer`, `0.00864 millimeter`, `8.64e-06 meter`,
`7910 nanometer`), and `physical-size failures: 0`.

### 12.7 Two runs were cancelled first, and the reason is in the workflow [live]

The push *did* trigger a run — `35374933203`, on `953d5f6` — and it was **cancelled**,
as was the first manual dispatch I made (`35374813151`, cancelled 90 seconds in,
during the apt step). Neither was a failure of the code. The workflow declares:

```yaml
concurrency:
  group: renderer-validation-${{ github.ref }}
  cancel-in-progress: true
```

so any new run on the same branch cancels the one in flight. The sequence was:

| Run | Trigger | Outcome |
|---|---|---|
| `35374933203` | push of `953d5f6` | cancelled — by my first manual dispatch, which started moments later |
| `35374813151` | first manual dispatch | cancelled — by my second dispatch, because `cancel-in-progress` does not exempt manual runs |
| `35374993168` | second manual dispatch | **success**, 25/25 steps |

Two lessons worth keeping. First, `cancel-in-progress: true` means "dispatch a run"
is not a way to get a *second* observation of the same branch — it silences the one
already running. Second, an initial reading of this session's evidence said flatly
that "the push did not start a run"; the API says it did, and the difference matters
because it changes the diagnosis from *GitHub not triggering the workflow* (a real
defect, worth investigating) to *my own dispatch cancelling it* (expected behaviour).
The first reading was wrong and is recorded here rather than deleted.

The earlier evidence run `35249590299` is unchanged and remains what the figures
were built from.

### 12.8 What this session did not do

It did not open a PR, did not submit anything, did not add a `LICENSE`, and did not
change the patch. Two workflow assertions were **strengthened**, not relaxed:
`PATCH_FILES == 3`, and `PATCH_IDENTICAL=no` now fails the step instead of printing
a line and continuing (§12.1).

---

## 14. Session of 2026-09-18 (second) — the licence and attribution applied

Separate section because it is a different kind of work from §12: §12 found the
defects, this session **acted** on the two that needed a decision. No code changed
again — the only tracked source file touched is the probe tool in §12.5, from the
previous session — and no build or render was re-run, because nothing here can
affect either.

### 14.1 What was applied

| Artefact | What it establishes |
|---|---|
| `LICENSE` | MIT, **Copyright (c) 2026 Marco Pontesilli**, for this repository's original work, **with an explicit clause listing what it does not cover**. That clause is the point: a bare MIT file at the root of a mixed repository is exactly the ambiguity the inventory was written to remove |
| `LICENSE-GPL-3.0.txt` | The GNU GPL v3 text, copied byte-for-byte from `villa/volume-cartographer/LICENSE` — 35,832 bytes, SHA-256 `95dd6ceb0e40950eb88fef3a6eb017802f13c6e87fefc72500ebcededd24c760`, **verified identical to the source**, not retyped [exec] |
| `NOTICE.md` | The authoritative path-by-path map: §1 MIT, §2 GPL-3.0-or-later with the verbatim upstream programme notice and external notices (nlohmann/json, OpenABF, bvh, mpl-colormaps), §2.2 the statement of modification with dates, §2.3 the preserved upstream notices, §2.4 what is **not** determined, §3 the third-party data, §4 what is not redistributed |
| `DATA_ATTRIBUTION.md` | The data's terms, the per-volume dataset assignment, **both** required citations, and an itemised description of the transformations applied to the figure |
| `patch/README.md` | The patch's own GPL notice, its modification dates, and its "proposed, not applied, not merged" status |
| GPL headers on four files | `harness/src/vsguard/upstream_read_volume_voxel_size.hpp`, `render_voxel_size_resolution.hpp`, `render_voxel_size_resolution.cpp`, `harness/tests/test_render_voxel_size.cpp` |
| `DOCS/PROGRESS_PRIZE_QUESTION.md` | The organisers' question, in English, ready to send. **Not sent** |

**The conservative treatment was chosen, not a technical separation.** The proposal
offered the option of splitting the GPL-linked harness files apart so that only the
genuinely upstream code stayed GPL. That was rejected: the `vsguard` resolver exists
precisely to call `vc::metadata::voxelSizeFromStoreMetadata`, so separating it would
mean either duplicating the resolver — the exact failure mode that caused this bug in
the first place — or building a second target, for no benefit to a reader. All four
files are therefore marked GPL-3.0-or-later, which **grants recipients more
permission than may strictly be required** and cannot be wrong in the direction that
matters. `NOTICE.md` §2.4 says that this is a deliberate over-inclusion and not a
finding that the strict reading is correct.

### 14.2 The one thing deliberately *not* done

Adding the notice as a comment **inside** `patch/vc_render_tifxyz.patch` was
considered and rejected. The CI step compares the applied diff byte-for-byte against
that artefact (`PATCH_IDENTICAL`), so anything prepended would either break the check
or make it compare something other than the bytes `git apply` consumes. The notice
lives in `patch/README.md` instead, and the patch is untouched — verified: no diff
against §12's state, and the file still reverse-applies.

### 14.3 Checks actually run this session [exec]

| Check | Result |
|---|---|
| `LICENSE-GPL-3.0.txt` identical to the upstream source | yes — SHA-256 match, 35,832 bytes |
| Patch unchanged from the previous commit | `git diff` empty for `patch/`; worktree blob `c4c1a99` = HEAD blob |
| Patch still reverse-applies to the pinned revision | exit 0 |
| Patch still touches exactly 3 files | `git apply --numstat` → 3 paths |
| `ci/preflight_workflow.py` | **PRE-FLIGHT OK**, including the 3-file assertion |
| Relative links in every tracked `*.md` resolve | all resolve (two known non-links: a code fragment, and a quotation of upstream's own relative link, annotated as such) |
| No licence applied indiscriminately to third-party material | audited: `LICENSE` names the exclusions; every `MIT` mention in `NOTICE.md` and `DATA_ATTRIBUTION.md` is an exclusion, a grant for this project's own work, an upstream dependency's licence, or part of a quoted licence text — none applies MIT to the patch or the data |
| GPL header present on all four derived files | audited: `GPL-3.0-or-later`, `NOT MIT` and a `NOTICE.md` reference in the first 25 lines of each |
| No generated upstream files tracked | `git ls-files` shows no `harness/src/villa/**`, no `harness/third_party/**`, no `villa/**` |
| **Harness rebuilt and re-run** | `cl.exe` build exit 0; upstream control **13/13**, project suite **27/27 (208 assertions)**, probe **4 documents / 3 divergences** |
| GitHub's licence detection | **not observable, and the negative result recorded below** |
| Figure generators | **not modified** this session, so their self-tests were not re-run; they were last run green in §12 |

**Rebuilt even though the header edits were comment-only, and that was the right
call**: the four files are compiled translation units, so "it was only a comment" is
a claim about the compiler that is cheaper to check than to assert. The rebuild is
what makes it a measurement. **No render and no full CI run**: the changes cannot
affect either, and §9's results stand from run `35249590299` / `35374993168`.

#### The GitHub licence-detection probe — a negative result, and what it did and did not establish

`https://api.github.com/repos/BioMarco/VoxelScaleGuard` reports
`"license": null`, and `/license` returns **404**. That is worth explaining rather
than leaving as a mystery, so it was probed: a throwaway branch
`probe/licence-detection` was pushed carrying a **pure, canonical MIT** `LICENSE`
(no exclusions clause), and `/license?ref=probe/licence-detection` also returned
**404**.

**What that establishes:** the detection API is not available for a non-default
branch, so *no experiment of this kind can distinguish the two cases here*. It does
**not** establish that the trailing exclusions clause defeats detection, and it does
**not** establish that it does not.

**What it does not change:** this repository's `main` is still the old
`f2945b9` — the licence work lives on `ci/renderer-validation` — so GitHub's
detection result *could not* have changed yet regardless of the file's content. No
conclusion is drawn, and none should be.

**The probe was fully reverted**: the remote branch deleted, the local branch
deleted, `LICENSE` restored and verified byte-identical to HEAD
(`git diff --stat -- LICENSE` empty), working tree clean. The only surviving trace
is this paragraph. The file kept is the one **with** the exclusions clause, because
legibility to a human arriving at the repository matters more than a badge whose
behaviour cannot be tested from here.

### 14.4 What remains open after this session

1. **Whether the organisers' wording accepts the split.** Asked, not sent:
   `PROGRESS_PRIZE_QUESTION.md`. Nothing here asserts either answer.
2. **The two legal uncertainties** in `NOTICE.md` §2.4 — the aggregate question and
   the derivative-or-combined question. Documented precisely; **not** resolved, and
   not resolvable by adding a notice.
3. **One dataset assignment is a derivation, not a quotation** — `PHerc0172` is
   placed in EduceLab-Scrolls from the shape and naming of its own metadata
   document, because no per-volume dataset index was found. `DATA_ATTRIBUTION.md`
   §2 says so and gives the reasoning, so a reader can disagree with it.

### 14.5 The prize page was re-read, and reported rather than interpreted [live]

`https://scrollprize.org/prizes`, re-read 2026-09-18. **Nothing relevant changed**:
the Progress Prize deadline is still *"11:59pm Pacific, September 30th, 2026"*, the
award is still *"Best Submission of the Month: $20,000"*, the three milestone
deadlines are still June 25th 2027, the Terms still say *"permissive license"*, the
Grand Prize conditions still say *"open source license (e.g. MIT)"*, Discord
registration is still a Grand Prize requirement rather than a Progress Prize one, and
the submission form is still the same per-month form. The table is in
`PROGRESS_PRIZE_QUESTION.md` §5. **The discrepancy is therefore still live and was
not resolved by re-reading** — which is the reason the question exists.

---

## 16. Session of 2026-09-23 — review of PR #1831, and the correction it forced

The PR opened on 2026-09-19. **hendrikschilling** reviewed it and raised two
findings. Both were correct, and the first invalidates something this project had
documented as a deliberate improvement (§11.1 and `PR_DRAFT.md`). That is recorded
here rather than quietly amended.

The PR itself is unchanged in scope: the same three files, the same one commit
before this round, no new PR, no rebase.

### 16.1 Finding 1 — the unknown-size branch removed the OME-Zarr pyramid description

Reviewer text, in substance: *unknown physical size removes the entire OME-Zarr
pyramid description; `Zarr.cpp` around line 375 omits the whole `multiscales` block,
losing dataset discovery, axes and relative pyramid scaling; OME readers may no
longer recognise the output as a multiscale image.*

**The reviewer was right, and the mistake was ours in a specific way.** §11.1 fixed a
real problem — a fabricated `scale = 1.0` with a `nanometer` label — by deleting the
containing block. That "fixed" the false measurement by also deleting the axes, the
ordered level list and every level's `coordinateTransformations`. The `multiscales`
attribute is how a reader discovers an image as multiscale at all; without it the
output is an anonymous group of arrays. Trading a wrong number for an unrecognisable
dataset is not a fix, and the block contains far more than the scale.

*Why it was not caught here:* every check this project had asked "does anything
claim a physical size it should not?" — which is precisely the question the deletion
satisfied. **No check asked "is the structure still complete?"**, and the reviewer's
question was the one missing. `check_physical_size.py --expect-unknown` in fact
*asserted* the block was absent, so the project's own tooling would have rejected the
correct behaviour. That assertion is now inverted (§16.4).

**The fix.** `buildMultiscales()` — split out of `writeZarrAttrs()` so it is a pure
function of `(baseVoxelSize, voxelUnit, sliceStep, pixelsPerVoxel)` — now always
produces a descriptor, and separates the two things a `scale` can mean:

| `baseVoxelSize` | `axes[*].unit` | each level's `scale` |
|---|---|---|
| `> 0` (known) | `voxelUnit` | physical units per axis: Z = `baseVoxelSize * sliceStep`, Y/X = `baseVoxelSize / pixelsPerVoxel * 2^level` |
| `<= 0` or non-finite (unknown) | **absent** | **relative** factor between that level and level 0: Z = 1, Y/X = `2^level` |

Level 0 is the identity, so there is no absolute length anywhere in the document; a
reader can still place the levels relative to each other and cannot mistake the
numbers for micrometres. This is what OME-NGFF 0.4 requires of a scale when a
physical size is unavailable:

> *"If scaling information is not available or applicable for one of the axes, the
> value MUST express the scaling factor between the current resolution and the first
> resolution for the given axis, defaulting to 1.0 if there is no downsampling along
> the axis."* — <https://ngff.openmicroscopy.org/0.4/#multiscale-md> [read]

The relative factors are not invented: `createPyramidDatasets()` keeps Z fixed and
halves only Y/X at each level, so Y/X double and Z stays at its level-0 spacing. The
descriptor also carries `metadata.physical_size = "unknown"`, so a reader does not
have to infer the omission from a missing key. The TIFF is unchanged — `tifDpi`
stays 0, so no resolution tag is written, which is a separate output format with its
own rule.

Resulting descriptor for an unknown size, from the real function [exec] — see §16.3
for how that was produced on a machine that cannot build volume-cartographer:

```json
{
  "axes": [ {"name":"z","type":"space"}, {"name":"y","type":"space"}, {"name":"x","type":"space"} ],
  "datasets": [
    {"path":"0","coordinateTransformations":[{"type":"scale","scale":[1.0,1.0,1.0]},{"type":"translation","translation":[0.0,0.0,0.0]}]},
    {"path":"1","coordinateTransformations":[{"type":"scale","scale":[1.0,2.0,2.0]}, …]},
    … through level 5, Y/X = 2^level, Z always 1.0 …
  ],
  "metadata": { "downsampling_method": "mean", "physical_size": "unknown" },
  "name": "render",
  "version": "0.4"
}
```

**No rendered pixel is affected by any of this**, and neither is the known-size path:
the branch is only reached when no size could be resolved anywhere.

### 16.2 Finding 2 — switching resolvers lost previously supported local metadata

Reviewer text, in substance: *`resolveLocalStoreVoxelSize()` stops at an existing
`meta.json` even when that file contains no usable voxel size, instead of continuing
to `metadata.json`; and it does not preserve the previously supported
`metadata.json → scan.voxelsize` case. Some existing local inputs lose TIFF
resolution and, because of finding 1, OME multiscale metadata.*

**Also right, and it was a regression this project introduced.** Both halves are in
the code as it was:

* `resolveLocalStoreVoxelSize()` returned `voxelSizeFromStoreMetadata(parse(file))`
  from inside the loop — `return` of a `std::optional`, so a file that parsed but
  yielded nothing *ended the search*. A `meta.json` holding only dimensions, which
  many published stores have, hid a perfectly good `metadata.json` beside it. An
  unparseable file ended the search too, via the `catch` block.
* The renderer's own pre-patch reader had three candidates
  (`vc_render_tifxyz.cpp:999-1001` @ `757f70c`):

  ```cpp
  if (auto v = tryFile(volPath / "meta.json", nullptr))      return v;
  if (auto v = tryFile(volPath / "metadata.json", "scan"))   return v;   // dropped
  if (auto v = tryFile(volPath / "metadata.json", nullptr))  return v;
  ```

  The shared resolver covers the first and third. `metadata.json → scan.voxelsize`
  was dropped when this patch replaced the private reader with the shared one.

**The fix, in the shared resolver rather than in the renderer** — as the reviewer
preferred, and for the reason this project already committed to in §11.3:
`Volume::NewFromUrl()` and `vc_grow_seg_from_segments()` call the same resolver, so a
renderer-only workaround would let the two disagree about what a store says. The
narrower alternative (patching around it in `vc_render_tifxyz.cpp`) was rejected for
that reason, not for effort.

* `resolveLocalStoreVoxelSize()` now falls through: a candidate that yields nothing —
  because it parsed but carried no recognised schema, or because it would not parse —
  advances to the next. A missing file was always skipped.
* `legacyScanVoxelSize()` restores `metadata.json → scan.voxelsize`, matched at that
  **exact path**, only when `scan` holds no `tomo` acquisition record. Order is the
  pre-patch order: top-level `voxelsize`, then the acquisition record, then
  `scan.voxelsize`, then the `source` walk. The `tomo` guard keeps the modern shape on
  its own path and stops the fallback shadowing it.
* Exact-path matching is deliberate, and tested against four near-misses:
  `metadata.scan.voxelsize`, `source.scan.voxelsize`, `properties.voxelsize` and
  `scan.properties.voxelsize` all still resolve nothing.

**Units for the restored field, determined from the history rather than assumed.**
`readVolumeVoxelSize()` returned this number with **no conversion of any kind**, and
its caller treated every number it returned as micrometres — the only input that ever
received a unit conversion was an explicit `--voxel-size`, guarded by the
`voxelSizeFromCli` flag (`vc_render_tifxyz.cpp:1397-1412` @ `757f70c`). Since the
same reader produced both `meta.json`'s top-level `voxelsize` (micrometres: the
published 7.91 matches the volume's own `-7.910um-` name) and `scan.voxelsize`, and a
single code path cannot have meant two units, `scan.voxelsize` is micrometres. Had it
been anything else, the legacy volume would have produced a wrong TIFF resolution on
exactly the path the repository's live-S3 test exercises.

### 16.3 What was actually executed, and what was not

This machine cannot build volume-cartographer: no OpenCV, no Qt, and none of the
dependency closure (`README` environment notes). Three things were therefore run
differently, and the distinction matters.

**Executed [exec]:**

| Check | Command / artefact | Result |
|---|---|---|
| The fork's resolver suite, compiled for real | `scratch/run_resolver_test.ps1` — MSVC 14.34.31933 on the fork's `VoxelSizeMetadata.cpp` + `Json.cpp` + `test_voxel_size_metadata.cpp` | **17 cases / 87 assertions pass** |
| Negative control: revert the fall-through | same, with the old `return` restored | **1 case fails** (`an unusable meta.json falls through`) |
| Negative control: remove the legacy schema | same, with the `legacyScanVoxelSize()` call removed | **2 cases fail** (legacy schema, and the resolver/reader agreement case) |
| The real `buildMultiscales()`, executed | `scratch/build_zattrs_driver.ps1` — compiles the fork's `Zarr.cpp` unmodified against the real `utils::Json`, with a `cv::Mat`/`cv::Size` stand-in and empty `VcDataset` bodies supplied by `scratch/` | **all structural checks pass**; output quoted in §16.1 |
| Negative control: the reviewed behaviour | same driver, `buildMultiscales()` returning an empty object when the size is unknown | **structure reported MISSING**, non-zero exit |
| Harness, rebuilt | `harness/build.ps1` | build OK |
| Harness, upstream control | `test_upstream_voxel_size_metadata.exe` | **17 cases / 87 assertions pass** (was 13/54; the PR's tests travel with it) |
| Harness, project suite | `test_render_voxel_size.exe` | **35 cases / 242 assertions pass** (was 27/208; +8 review cases) |
| Harness, probe | `probe_render_voxel_size.exe` | 4 documents, 3 divergences — unchanged |
| Physical-size checker self-test | `ci/selftest_physical_size.py` | **13/13 pass** (was 11/11; the unknown-size cases were rewritten) |
| Workflow pre-flight | `ci/preflight_workflow.py` | **PRE-FLIGHT OK**, including `bash -n` on the edited step |
| Patch regeneration and integrity | the corrected `AGENTS.md` §4 command | 3 files, +358/−102, reverse-applies exit 0 |
| Mergeability against current upstream `main` | `git merge-tree` in a throwaway worktree | **no conflicts**; `main` at `59b454a8` |

Two things the harness caught, both worth recording because they are the guards
working:

1. **`test_render_voxel_size.cpp` failed** when the harness's *pristine* renderer
   copy was refreshed from the PR branch: it asserted the pristine copy contains
   `readVolumeVoxelSize` and not `resolveRenderVoxelSize`, and the PR branch's copy
   has the opposite. That is the anti-contamination check doing its job — the
   pristine copy must come from the pinned commit, never from a branch that carries
   the change. `harness/setup.ps1` was corrected to take the three files the *patch*
   modifies from `git show` at the pinned commit, and the three files the *PR* changes
   from the PR branch, with the PR revision printed and recorded in
   `harness/PR_REVISION.txt`.
2. **`selftest_physical_size.py` case 7 failed** once the checker was fixed, because
   the self-test still passed `multiscales=False` as the *expected-accept* case. The
   expectation, not the code, was stale.

**Not executed, and why:**

* **No full compile of volume-cartographer, and no renderer run.** Not possible here,
  and not required by the changes: the renderer diff is comments only, and the two
  `core` files' behaviour is covered by the two harness suites, which do compile and
  run. `Zarr.cpp` compiles as part of the scratch driver, so its syntax is checked;
  the parts of it that need OpenCV are not exercised.
* **No GitHub Actions run for this revision.** CI triggers on push, so it cannot be
  claimed before the push happens. Everything above is local. When the push lands,
  run `renderer-validation` on `ci/renderer-validation` is the remote check, and its
  unknown-size step now asserts the structure is present and unitless rather than
  absent.
* **No render on `PHerc0009B` / `PHerc0172`.** Neither finding can change those
  outputs: PHerc0009B resolves `8.64 µm` and PHerc0172 `7.91 µm` through paths that
  are untouched, and the unknown-size branch is not reached. The existing PHerc0009B
  and PHerc0172 evidence therefore stands **unchanged and is not superseded** — no
  new figure was generated, and none is needed.

### 16.4 What this invalidated in the project's own documentation

Corrected, each with the correction dated:

* `PR_DRAFT.md` §"The size is unknown" — rewritten: it described omitting the block
  as the fix. It now describes what the block is for and what the corrected branch
  writes, with the reviewer's finding attributed.
* `PR_DRAFT.md` §Behaviour of each source — the "nothing usable" row said "no
  `multiscales` block"; it now says no physical size, block retained.
* `PR_DRAFT.md` §Evidence — the unknown-size bullet said the render "declares **no**
  `multiscales` block". Corrected, and the old wording is quoted so the change is
  visible.
* `RESULTS.md` §11.1 — annotated: the fix it records was itself wrong, and §16 is the
  correction. The old table is kept because the change is the point.
* `ci/check_physical_size.py --expect-unknown` — **inverted**. It asserted the block
  was absent; it now asserts the block is present, unitless, has all three axes and
  all six levels, and that each level's relative scale is exactly `[1, 2^l, 2^l]`.
  This is the check that would have caught the defect, and did not exist.
* `ci/selftest_physical_size.py` — the unknown-size cases rewritten; a
  `no_multiscales` fixture is now an expected **reject**, and three further reject
  cases were added (a unit with no measurement, wrong relative scales, a surviving
  resolution tag).
* `.github/workflows/renderer-validation.yml` — the unknown-size step's comment, and
  a new plainly-worded summary that prints the axes, their units, the level count and
  every level's scale, plus an explicit guard against the block going missing again.
* `patch/README.md`, `README.md`, `ARCHITECTURE.md`, `INDEX.md`, `PROJECT_STATUS.md`,
  `SUBMISSION_DRAFT.md`, `RESUME.md`, `CI_VALIDATION.md` — the patch diffstat
  (+250/−65 → +358/−102) and the per-file counts, which the review changed.
* `DOCS/PROGRESS_PRIZE_CHECKLIST.md` — the test counts.

**Nothing about `PHerc0009B` or `PHerc0172` was invalidated**, and no prior evidence
was replaced.

**Supersession, stated explicitly:** for the *unknown-size branch*, any statement
made before 2026-09-23 is superseded by this section; for everything else, run
`35249590299` and its figures remain current.

---

## 17. Bottom line

| Question | Answer |
|---|---|
| Was the bug still present? | **Yes** in `vc_render_tifxyz`; **no** in `vc_grow_seg_from_seed` (already fixed upstream) |
| Reproduced? | **Yes**, against the live catalog: the pre-patch reader resolves 1 of 4 real published volumes |
| Demonstrated before/after? | **Yes**, twice over: on the resolution logic against live metadata, **and now from two real compiled binaries on a real published volume** (§9) |
| Tested on real data? | **Yes** — real published metadata documents, **and a real render** of `PHerc0009B` and `PHerc0172` through the real binaries (§9.4) |
| Regression tests? | 35 cases / 242 assertions; upstream's 17 cases / 87 assertions pass unmodified as a control. (Was 27/208 and 13/54 before the review round added 8 harness cases and 4 resolver cases — §16.3) |
| Binary-verified? | **Yes** since 2026-09-16: both binaries compile in CI and were run on public data (§9.3–9.4) |
| Does the patch compile? | **Yes** — but it did **not** before this session. The committed patch could never have built (§9.1). That is the single most important result in this document |
| Are the rendered pixels unchanged? | **Yes**, decoded-pixel hashes identical on both volumes (§9.5) |
| Can the patched binary be built on this machine? | **No** locally (§8.4), which is why the build runs on GitHub Actions (§9) |
| Did the review find real defects? | **Yes, two, both now fixed.** The unknown-size branch removed the OME-Zarr `multiscales` block (the image's discovery metadata), and the shared resolver stopped at the first existing metadata file and had dropped the legacy `scan.voxelsize` schema. §16 |
| Is the repository's licensing settled? | **Yes as an arrangement, no as a question.** The split is applied (`LICENSE`, `LICENSE-GPL-3.0.txt`, `NOTICE.md`, `DATA_ATTRIBUTION.md`) and two legal uncertainties are documented in `NOTICE.md` §2.4. Whether the organisers accept it is asked and unanswered (§14.4) |
| Is the third-party attribution done? | **Yes** — `DATA_ATTRIBUTION.md`, reachable from the README, the figures' own README and the copied documents' provenance file (§14.1) |
| Ready to submit as-is? | **The technical evidence is in place and the PR is open** (#1831). Remaining: the author's decision to send the organisers' question and to fill in the form |
| Progress Prize deadline | **11:59pm Pacific, 30 September 2026** — re-verified [live] 2026-09-18. Earlier revisions wrongly said it had passed; see §8.2 |
