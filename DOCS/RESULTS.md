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

> **Update, 2026-09-16 — the reason above was incomplete; see §8.4.** The closure
> being absent is one cause. A second, independent cause is that neither CMake nor
> Ninja can execute a compiler in this environment, so a CMake-driven build is not
> reachable here even once the dependencies are present. §8.4 also records that the
> target's *link* closure excludes Ceres, CGAL and Qt — those are configure-time
> requirements of the project as a whole, not of this binary.

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

## 9. Bottom line

| Question | Answer |
|---|---|
| Was the bug still present? | **Yes** in `vc_render_tifxyz`; **no** in `vc_grow_seg_from_seed` (already fixed upstream) |
| Reproduced? | **Yes**, against the live catalog: the pre-patch reader resolves 1 of 4 real published volumes |
| Demonstrated before/after? | **Yes** for the resolution logic and the declared physical scale: ×2400, ×8640 and ×45532 errors removed on real stores, with a control that confirms the one case that already worked |
| Tested on real data? | **Yes** — real published metadata documents. **No** — no render was run |
| Regression tests? | 16 cases / 82 assertions added; upstream's 13 cases / 54 assertions pass unmodified as a control |
| Binary-verified? | **No.** See §7.1, and §8.4 for why a build is not reachable here |
| Can the patched binary be built on this machine? | **No.** Two independent causes: the OpenCV/libtiff/Boost/curl/blosc closure is absent, and neither CMake nor Ninja can execute a compiler here. See §8.4 |
| Ready to submit as-is? | **No.** It is a verified diagnosis with a logic-verified patch. It needs §7.1–7.2 before it can be presented as a working fix, and §7.4 to be reachable from the GUI |
| Progress Prize deadline | **11:59pm Pacific, 30 September 2026** — re-verified [live] 2026-09-16. Earlier revisions wrongly said it had passed; see §8.2 |
