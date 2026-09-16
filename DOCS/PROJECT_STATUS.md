# PROJECT_STATUS

**Project:** VoxelScale Guard
**Workspace:** `C:\Users\marco\Documents\DeepSeek\VoxelScaleGuard`
**Status:** diagnosis verified and demonstrated on real data; patch written and
logic-verified; **not compiled or run** — see §6
**Upstream revision analysed:** `ScrollPrize/villa` @
`757f70c0140a4cfbbbd44975ef09558444b96980` (`main`)

---

## 1. What this project is

VoxelScale Guard investigates and fixes a defect in Vesuvius Challenge `villa`
tooling: `vc_render_tifxyz` attaches a **physically wrong voxel size** to its
output, silently. Measured on real published volumes, the declared physical scale
is wrong by ×2400, ×8640 or ×45532, depending on the volume. The rendered pixels
are correct; the OME-Zarr axis metadata and TIFF resolution tags are not.

## 2. Status of the previous project (FitPatch Loop)

**FitPatch Loop is suspended by decision of the user.** Its processing was
interrupted by the user.

This project has not accessed, read, modified, deleted, renamed or moved any file
belonging to FitPatch Loop, and has not performed any cleanup on it. No downloads,
processes or subagents belonging to FitPatch Loop have been resumed.

Nothing in this document should be read as a claim that FitPatch Loop's files were
verified, frozen, backed up, or are otherwise in a known-good state. Their state is
simply **not known to this project and not touched by it**. That is a statement
about what this project did, not an assurance about their integrity.

No second project has been created inside the old `Vesuvius` folder.

## 3. Isolation rules observed

* Development happened exclusively under
  `C:\Users\marco\Documents\DeepSeek\VoxelScaleGuard`.
* The previous project's folder `..\Vesuvius` was not touched.
* The **remote** `villa` repository was not modified, pushed to, or opened as a
  pull request. A local clone was made and modified in place to hold the patch;
  nothing left the machine.
* No pull request was opened. Nothing was published.
* Nothing was installed. No download exceeded 2 GB (actual: ~1.2 MB of
  header-only libraries plus ~12 KB of volume metadata).
* No paid service was used. No GPU compute was used.

## 4. Findings

Verified by reading the pinned revision and by **executing** its real code against
live catalog metadata. Details in `ROOT_CAUSE_ANALYSIS.md`; transcripts in
`RESULTS.md`.

| # | Finding | How established |
|---|---|---|
| 1 | The renderer resolves the voxel size **before** opening the remote volume, so it can never see remote metadata; it grew a private filesystem-only reader as a result | read `vc_render_tifxyz.cpp:986` vs `:1315` |
| 2 | That reader knows only a top-level `voxelsize`; most of the catalog publishes the value only as `scan.tomo.acquisition.detector.samplePixelSize` (mm). `core` already has a shared resolver, used by `vc_grow_seg_from_segments` and not by the renderer | read; **executed**: resolves 1 of 4 real volumes |
| 3 | The reader validates the field's *type* but not its *value*: `{"voxelsize":0}` → `0.0`, `{"voxelsize":-3}` → `-3.0`, both returned as measurements | **executed** |
| 4 | The `1.0` placeholder is emitted as a physical scale with the unit defaulting to `nanometer` while the number is micrometres. The error is the product of two independent defects | **executed on real data**: ×8640 / ×45532 / ×2400 |
| 5 | The GUI never passes the size for a native-resolution remote volume, so the CLI fix would not reach most users | read `SegmentationCommandHandler.cpp:2076` |
| 6 | The `vc_grow_seg_from_seed` half of issue #1403 is **already fixed upstream** — not by us | read `vc_grow_seg_from_segments.cpp:1165`; the issue's own follow-up comment concedes the reporter was on a different branch |
| 7 | A prior fix for this exact defect exists as PR #1417, **closed unmerged by a 14-day inactivity bot**, not rejected | GitHub API |
| 8 | The URL-fragment concern raised in #1417's review is **real** in `joinRemoteUrlPath` and reachable via the GUI's `remoteLocator()` for rebased volumes | **executed** + read |
| 9 | A C++ toolchain was already present: MSVC 14.34.31933, Windows SDK 10.0.22000.0, plus CMake/Ninja bundled with VS 2022 | discovered and used |

## 5. Completed work

**Reconnaissance** — scrollprize.org prizes and data; `villa` at a pinned commit;
issues #1403 and #1226 in full with comments; PRs #1417, #1228, #1229, #1313,
#1541, #1797 including review comments. Sources, licences and revisions in
`RESEARCH.md`.

**Live catalog probe** — four published volumes; raw documents saved under
`research/raw_metadata/`.

**Interrogation of the two review concerns on #1417** — both reproduced as
executable tests rather than accepted or dismissed. One confirmed and shown
reachable; one confirmed and shown *worse* than the review described.

**Patch** — `patch/vc_render_tifxyz.patch`, one file, +177/−65. Applies exactly to
the pinned revision (`git apply --check --reverse` succeeds on the patched tree).

**Harness** — `harness/`, which compiles the pinned revision's real
`VoxelSizeMetadata.cpp` / `RemoteUrl.cpp` / `Json.cpp` byte-for-byte and runs:
upstream's own suite unmodified (**13 cases / 54 assertions**, as a control) plus
this project's tests (**16 cases / 82 assertions**), plus a before/after probe.

**Documents** — all ten required, listed in §7.

## 6. What is NOT done, and why

This is the most important section. The brief asked not to declare the project
finished because code compiles; it does not even compile yet.

| Gap | Cause | What it needs |
|---|---|---|
| **The patched binary was never compiled or run** | No Qt, OpenCV, Ceres, CGAL or vcpkg on the machine; `windows-msvc` builds that closure from source. Upstream also wants CMake ≥ 3.28, and 3.24 is installed | The vcpkg dependency closure, or the CI container. Multi-GB, long build. **Requires authorisation; not attempted** |
| **No `.zattrs` or TIFF was produced** | Needs the binary above plus a real volume and a tifxyz segment | (as above) |
| **No render was run on real data** | (as above) | (as above) |
| **The GUI path remains broken** | `SegmentationCommandHandler.cpp:2076` change deliberately left as a separate commit, because it changes GUI behaviour and its predicate has a lapsed history (#1228) | A maintainer decision; the exact edit is in `FEASIBILITY.md` §8 |
| **The `vc_zarr_to_tiff` schema gap** | Same class of defect (local-only, top-level key only), but no remote path and not needed for the reported problem | Documented as a candidate, not developed |
| **The live-S3 test still pins the legacy volume** | Changing an existing live test's fixture is a maintainer decision | Proposed in `PR_DRAFT.md` |
| **No submission made** | By instruction; and the artifact-level evidence is missing | Steps above |

**Therefore the patch must be described as "logic-verified, not binary-verified",
and the project is not ready to be presented as a working fix**, let alone as a
Progress Prize submission.

## 7. Deliverables

| File | Contents |
|---|---|
| `PROJECT_STATUS.md` | this file |
| `RESEARCH.md` | sources, exact commits, licences, and three brief assumptions that failed |
| `PRIZE_REQUIREMENTS.md` | Progress Prize rules vs. what this project considers useful |
| `ROOT_CAUSE_ANALYSIS.md` | the cause at file-and-line resolution |
| `FEASIBILITY.md` | **GO**, with scope, resources, alternatives, and out-of-scope items |
| `ARCHITECTURE.md` | the fix's design, ordering argument, and rejected alternatives |
| `TEST_PLAN.md` | executed vs. blocked, and what would falsify each claim |
| `RESULTS.md` | everything executed, with exit codes; and §7, what was not |
| `README.md` | description, layout, build and run |
| `PR_DRAFT.md` | pull request draft, **not submitted** |
| `SUBMISSION_DRAFT.md` | Progress Prize draft, **not submitted** |
| `patch/vc_render_tifxyz.patch` | the fix |
| `harness/` | the reproducer and its tests |
| `research/` | the live catalog probe and the raw documents it fetched |

## 8. Environment notes worth keeping

* **MSVC works**: `cl.exe` 14.34.31933 with Windows SDK **10.0.22000.0** (the
  10.0.22621.0 paths in a naive setup do not exist on this machine).
* **CMake and Ninja cannot be used here**: CMake fails to launch its own
  subprocesses in this sandbox. `harness/build.ps1` invokes `cl.exe` directly.
  `harness/CMakeLists.txt` is kept for normal environments.
* **Native commands cannot have output redirected or piped**: `cmd > file`,
  `cmd | ...`, `cmd 2>&1` fail with `StandardOutputEncoding is only supported when
  standard output is redirected`. Commands are recorded in the form that works;
  `git diff --output=…` is used instead of a shell redirect.
* **Git ownership**: the clone is owned by `BUILTIN/Administrators` because of the
  elevated clone. `~/.gitconfig` is outside the sandbox, so git is invoked as
  `git -c safe.directory='*' …`.
* **`Invoke-WebRequest` fails TLS** in this environment; Node's `fetch` works,
  which is why `research/fetch_volume_metadata.mjs` and `harness/fetch_deps.mjs`
  are Node scripts.
* **Command execution is intermittent**: several external processes returned empty
  output with no exit code mid-session and later recovered. No work depends on a
  single such invocation.

## 9. Activity log

| Activity |
|---|
| Project opened; workspace confirmed; FitPatch Loop declared out of scope and left untouched |
| `villa` cloned locally at `757f70c0140a4cfbbbd44975ef09558444b96980` |
| Issues #1403, #1226 and PRs #1417/#1228/#1229/#1313/#1541/#1797 read in full, including review comments |
| Live metadata probe of 4 published volumes; raw documents saved |
| Toolchain discovered (MSVC + SDK 10.0.22000.0); MSVC smoke test compiled and run |
| Harness set up with pinned byte-for-byte copies; built with `cl.exe` |
| Upstream's 13-case suite compiled and run unmodified: 13/13 pass (harness control) |
| Defect reproduced against the real documents: 1 of 4 volumes resolved |
| Both #1417 review concerns reproduced as executable tests |
| Two test assertions corrected when execution contradicted them (`0` and `-3` are *returned*, not merely mis-signalled) |
| Patch applied to `villa`; diff verified to round-trip via `git apply --check --reverse` |
| `RESEARCH.md`, `PRIZE_REQUIREMENTS.md`, `ROOT_CAUSE_ANALYSIS.md`, `FEASIBILITY.md`, `ARCHITECTURE.md`, `TEST_PLAN.md`, `RESULTS.md`, `README.md`, `PR_DRAFT.md`, `SUBMISSION_DRAFT.md` written |
| Remaining: build the patched binary and produce artifact-level evidence — **blocked, needs authorisation** |
