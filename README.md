# VoxelScale Guard

A verified diagnosis and a contained fix for a **silently wrong physical voxel
size** in Vesuvius Challenge [`villa`](https://github.com/ScrollPrize/villa)'s
renderer, `vc_render_tifxyz`.

> **Status: the diagnosis is verified; the patch is logic-verified but has never
> been compiled or run.** The application cannot be built on the machine used
> here (no Qt/OpenCV/Ceres/CGAL/vcpkg). Read
> [`RESULTS.md` §7](DOCS/RESULTS.md) before relying on anything. No pull request
> has been opened against `villa`, and nothing has been submitted for a prize.

**Start here:** [`DOCS/INDEX.md`](DOCS/INDEX.md) — reading order and the evidence map.
**Contributing:** [`AGENTS.md`](AGENTS.md) — verification rules, attribution, and the
environment traps.

---

## The problem in one paragraph

`vc_render_tifxyz` writes the physical scale of its output into the OME-Zarr axis
metadata (`.zattrs`) and into TIFF resolution tags. To do that it needs the
volume's voxel size. On a current `villa` it looks for that number in a **local**
file, using a reader that recognises only a top-level `voxelsize` key — while the
process has *already fetched the correct value over the network* and is holding
it. For three of the four published volumes probed here, the result is that the
renderer falls back to a scale of `1.0` and declares it as **nanometres**. The
declared physical voxel size is then wrong by **×2400, ×8640 or ×45532**. The
rendered image is fine; every physical number attached to it is not.

## What was found

Established by reading `main` @ `757f70c` and by executing the pinned revision's
real code against live catalog metadata. Full detail in
[`ROOT_CAUSE_ANALYSIS.md`](DOCS/ROOT_CAUSE_ANALYSIS.md).

| # | Finding |
|---|---|
| 1 | The renderer resolves the size **before** it opens the remote volume, so it can never see the remote metadata. It grew its own filesystem-only reader as a result. |
| 2 | That reader knows only a top-level `voxelsize`. Most of the catalog publishes its resolution only as `scan.tomo.acquisition.detector.samplePixelSize` (mm). `core` already has a shared resolver for exactly this — `vc::metadata::resolveLocalStoreVoxelSize` — which `vc_grow_seg_from_segments` uses and the renderer does not. |
| 3 | The reader validates the field's *type* but not its *value*: `{"voxelsize": 0}` yields `0.0` and `{"voxelsize": -3}` yields `-3.0`, both returned as if they were measurements. |
| 4 | The placeholder `1.0` is emitted as a physical scale, with the unit defaulting to `nanometer` while the number is a micrometre quantity. The error is the product of two independent defects. |
| 5 | The GUI never passes the size for a native-resolution remote volume (`SegmentationCommandHandler.cpp:2076`), so the CLI fix would not reach most users. **Diagnosed, not fixed** — see [`FEASIBILITY.md` §8](DOCS/FEASIBILITY.md). |

Two claims worth flagging because they contradict common assumptions:

* **The `vc_grow_seg_from_seed` half of issue #1403 is already fixed upstream.**
  The `std::ifstream(vol_path/"meta.json")` it quotes is not on `main`. This
  project did not fix it and does not claim it.
* **Even where the old reader succeeds, the output is ×1000 wrong**, because of the
  nanometre default. Fixing discovery alone would turn ×8640 wrong into ×1000
  wrong.

## The evidence

Against four real published volumes, running the pinned revision's reader and the
fixed chain side by side:

| Published store | Old reader | Store actually says | Declared scale error |
|---|---|---|---|
| `PHerc0009B/…-8.640um-…` | *not found* | 8.64 µm | **×8640** |
| `PHercParis4/…-45.532um-…` | *not found* | 45.532 µm | **×45532** |
| `PHercParis4/…-2.400um-…` | *not found* | 2.4 µm | **×2400** |
| `PHerc0172/…-7.910um-…` | 7.91 µm | 7.91 µm | ×1000 (unit only) — the **control** |

The last row is the reason the bug survived: it is the only catalog entry the old
reader handles, and it is the volume `core/test/test_volume_live_s3.cpp` pins.

Tests: **16 cases / 82 assertions** added, plus upstream's own **13 cases / 54
assertions** compiled unmodified as a control. Transcripts in
[`RESULTS.md`](DOCS/RESULTS.md).

## Repository layout

```
README.md      this file
AGENTS.md      operating rules for agents working in this repository

DOCS/          all project documentation
  INDEX.md               reading order and the evidence map
  PROJECT_STATUS.md      state, gaps, isolation rules, environment notes, activity log
  RESEARCH.md            sources, exact commits, licences, and brief assumptions that failed
  PRIZE_REQUIREMENTS.md  Progress Prize rules vs. what this project considers useful
  ROOT_CAUSE_ANALYSIS.md the cause, at file-and-line resolution
  FEASIBILITY.md         GO decision, resources, alternatives, out-of-scope items
  ARCHITECTURE.md        the fix's design and the rejected alternatives
  TEST_PLAN.md           what was tested, what is blocked, what would falsify this
  RESULTS.md             everything actually executed, and everything that was not
  PR_DRAFT.md            pull request draft (not submitted)
  SUBMISSION_DRAFT.md    Progress Prize draft (not submitted)

patch/vc_render_tifxyz.patch    the proposed fix (177 insertions, 65 deletions)
harness/                        the reproducer: real upstream code + tests
research/                       live catalog probe + the raw documents it fetched
villa/                          read-only clone of villa @ 757f70c (git-ignored, local only)
```

Start at **[`DOCS/INDEX.md`](DOCS/INDEX.md)** for the reading order.

**Read [`DOCS/RESULTS.md` §7](DOCS/RESULTS.md) before trusting anything here.** It is
the list of what has *not* been verified, and it is the most important section of
the document set.

Contributors: read [`AGENTS.md`](AGENTS.md) first. It contains the verification
rules, the attribution requirements, and the environment traps that will otherwise
cost you an hour.

## Building and running the harness

No installs. Uses the MSVC toolchain already on the machine (found: MSVC
14.34.31933, Windows SDK 10.0.22000.0).

```powershell
# 1. copy the pinned upstream sources + fetch two header-only libs (~1.2 MB)
pwsh -File harness/setup.ps1
node harness/fetch_deps.mjs

# 2. build (invokes cl.exe directly; CMake cannot spawn subprocesses here)
pwsh -File harness/build.ps1 -Configuration Release

# 3. tests
cd harness/build/Release
./test_upstream_voxel_size_metadata.exe   # upstream's suite, unmodified: 13/13
./test_render_voxel_size.exe              # this project's: 16/16

# 4. the before/after demonstration (uses the documents in research/raw_metadata)
./probe_render_voxel_size.exe
```

To re-fetch the live metadata (read-only, ~12 KB):

```powershell
node research/fetch_volume_metadata.mjs research/raw_metadata
```

### Applying the patch

```bash
cd villa
git apply /path/to/patch/vc_render_tifxyz.patch
```

Verified to apply: `git apply --check --reverse` succeeds on the patched tree,
i.e. the patch describes it exactly.

## Preconditions for anything beyond this

1. Build and run the patched `vc_render_tifxyz`. **Measured on 2026-09-16 as
   blocked on two independent things** (`DOCS/RESULTS.md` §8.4): the compile-time
   closure (OpenCV, libtiff, Boost `program_options`, curl, blosc, …) is absent, and
   neither CMake nor Ninja can execute a compiler in this environment. Upstream's
   CMake project also cannot be configured with a reduced closure —
   `find_package(Ceres REQUIRED)` is unconditional. **Not attempted; the required
   authorisation has not been given.**
2. Produce a real `.zattrs` and TIFF from a real volume and a tifxyz segment, and
   check the tags. Needs (1).
3. Decide the VC3D GUI predicate change (`DOCS/FEASIBILITY.md` §8).
4. ~~Re-check the Progress Prize deadline.~~ **Re-checked 2026-09-16 [live]:** the
   next deadline is 11:59pm Pacific, **September 30th, 2026** — 14 days out, not
   past. An earlier revision of this file wrongly said it had passed; see
   [`RESULTS.md` §8.2](DOCS/RESULTS.md).

## Licences

* `villa` is **MIT**, Copyright (c) 2024 Vesuvius Challenge. The patch is a
  derivative work and carries the same terms.
* The harness copies `villa` translation units byte-for-byte; they remain MIT.
* `nlohmann/json` (MIT) and `doctest` (MIT) are fetched at build time, not
  vendored.
* `scrollprize.org` content is CC BY-NC 4.0 unless otherwise specified; quoted
  prize rules are attributed to that page.
* The Open Data bucket is accessed anonymously and read-only.

## Attribution

The approach of having the renderer consult the volume's remote voxel size is
**NicolasHuberty**'s, from PR #1417, which lapsed to an inactivity bot rather than
being rejected. The `samplePixelSize` schema handling,
`resolveLocalStoreVoxelSize`, and `Volume::voxelSize()`'s semantics are
**Bullo27**'s and the villa maintainers'. The VC3D enable-predicate diagnosis is
**Bullo27**'s, from PR #1228. See [`FEASIBILITY.md` §7](DOCS/FEASIBILITY.md).
