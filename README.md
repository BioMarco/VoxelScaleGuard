# VoxelScale Guard

A verified diagnosis and a compiled, tested fix for a **silently wrong physical
voxel size** in Vesuvius Challenge [`villa`](https://github.com/ScrollPrize/villa)'s
renderer, `vc_render_tifxyz`.

> **Status: the fix is written, compiled, run, and tested. The pull request is
> prepared but NOT open.** A fork and a branch exist; the PR has not been opened and
> nothing has been submitted for a prize, both pending the repository owner's
> decision.

**Reviewers, start here:**

| If you want | Read |
|---|---|
| The problem in 30 seconds | [The problem](#the-problem-in-one-paragraph), below |
| Proof that it is fixed | [`DOCS/CI_VALIDATION.md`](DOCS/CI_VALIDATION.md) — the run record |
| The before/after figure | [`DOCS/evidence/before-after.png`](DOCS/evidence/before-after.png) |
| The same result as terminal output | [`DOCS/evidence/terminal-before-after.png`](DOCS/evidence/terminal-before-after.png) — the run's log lines, drawn verbatim, and labelled as CI output rather than an interactive session |
| What the evidence figures are, and how they were built | [`DOCS/evidence/README.md`](DOCS/evidence/README.md) |
| What is **not** verified | [`DOCS/RESULTS.md` §7](DOCS/RESULTS.md) — kept deliberately, and mostly closed |
| The reading order for everything | [`DOCS/INDEX.md`](DOCS/INDEX.md) |
| The proposed PR text | [`DOCS/PR_DRAFT.md`](DOCS/PR_DRAFT.md) |
| The prize submission text | [`DOCS/SUBMISSION_DRAFT.md`](DOCS/SUBMISSION_DRAFT.md) |
| Licence and attribution position | [`DOCS/LICENSING_PROPOSAL.md`](DOCS/LICENSING_PROPOSAL.md) — a proposal; no licence applied |

---

## What this repository contains

1. **A diagnosis**, at file-and-line resolution, of why `vc_render_tifxyz` attached
   a physically wrong voxel size to everything it rendered — established by reading
   the pinned revision *and* by executing its real code against live catalog
   metadata.
2. **A fix**, a patch against the pinned revision. It is **three files**: the
   renderer, plus the OME-Zarr attribute writer whose contract had to change so that
   an unknown size is not published as a measurement.
3. **Evidence on real published data**: the patch compiled and run on GitHub-hosted
   runners against two public catalog volumes, with a deliberate control, comparing
   the declared metadata, the TIFF tags, and the rendered pixels.
4. **A public test harness** that reproduces the defect and pins the behaviour,
   including upstream's own test suite compiled unmodified as a control.

## The problem in one paragraph

`vc_render_tifxyz` writes the physical scale of its output into the OME-Zarr axis
metadata (`.zattrs`) and into TIFF resolution tags. To do that it needs the volume's
voxel size. It looked for that number in a **local** file, using a reader that
recognises only a top-level `voxelsize` key — while the process had *already fetched
the correct value over the network* and was holding it. For three of the four
published volumes probed here, the renderer therefore fell back to a scale of `1.0`
and declared it as **nanometres**. The declared physical voxel size was wrong by
**×2400, ×8640 or ×45532**. The rendered image was fine; every physical number
attached to it was not.

## What was found

Established by reading `main` @ `757f70c` and by executing the pinned revision's
real code against live catalog metadata. Full detail in
[`DOCS/ROOT_CAUSE_ANALYSIS.md`](DOCS/ROOT_CAUSE_ANALYSIS.md).

| # | Finding |
|---|---|
| 1 | The renderer resolves the size **before** it opens the remote volume, so it can never see the remote metadata. It grew its own filesystem-only reader as a result. |
| 2 | That reader knows only a top-level `voxelsize`. Most of the catalog publishes its resolution only as `scan.tomo.acquisition.detector.samplePixelSize` (mm). `core` already has a shared resolver for exactly this — `vc::metadata::resolveLocalStoreVoxelSize` — which `vc_grow_seg_from_segments` uses and the renderer does not. |
| 3 | The reader validates the field's *type* but not its *value*: `{"voxelsize": 0}` yields `0.0` and `{"voxelsize": -3}` yields `-3.0`, both returned as if they were measurements. |
| 4 | The placeholder `1.0` is emitted as a physical scale, with the unit defaulting to `nanometer` while the number is a micrometre quantity. The error is the product of two independent defects. |
| 5 | The GUI never passes the size for a native-resolution remote volume (`SegmentationCommandHandler.cpp:2076`), so the CLI fix would not reach most users. **Diagnosed, not fixed, and deliberately outside this patch** — see [`FEASIBILITY.md` §8](DOCS/FEASIBILITY.md). |

Two claims worth flagging because they contradict common assumptions:

* **The `vc_grow_seg_from_seed` half of issue #1403 is already fixed upstream.**
  The `std::ifstream(vol_path/"meta.json")` it quotes is not on `main`. This
  project did not fix it and does not claim it.
* **Even where the old reader succeeds, the output is ×1000 wrong**, because of the
  nanometre default. Fixing discovery alone would turn ×8640 wrong into ×1000 wrong.

## The evidence

### From real compiled binaries, on real published volumes

The patched renderer **has been compiled and run** [exec]. Both the baseline and the
patched binary were built from the same commit, in the same configuration, by
[`.github/workflows/renderer-validation.yml`](.github/workflows/renderer-validation.yml)
on a free `ubuntu-24.04` runner, and run against public catalog data with identical
arguments.

Run: **<https://github.com/BioMarco/VoxelScaleGuard/actions/runs/35249590299>**

| | baseline (`main`) | patched |
|---|---|---|
| `PHerc0009B` `.zattrs` | `nanometer` / `[1, 1, 1]` | `micrometer` / `[8.64, 8.64, 8.64]` |
| `PHerc0009B` TIFF | *no resolution tag* | `XResolution 2939.8147` px/inch |
| `PHerc0172` `.zattrs` (control) | `nanometer` / `[1, 1, 1]` | `micrometer` / `[7.91, 7.91, 7.91]` |
| `PHerc0172` TIFF (control) | *no resolution tag* | `XResolution 3211.1252` px/inch |
| **decoded pixels** | — | **identical on both volumes** |

`25400 / 8.64 = 2939.8148…` and `25400 / 7.91 = 3211.1252…`, so the emitted
resolution is the correct physical scale. The pixels are unchanged: the file *bytes*
differ only because the resolution tag is the fix, which is why the regression check
is a hash of the decoded pixel array and not of the file.

### From the pinned revision's own code, against the live catalog

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

### Tests

**27 cases / 208 assertions** added, plus upstream's own **13 cases / 54 assertions**
compiled unmodified as a control. Transcripts in
[`DOCS/RESULTS.md`](DOCS/RESULTS.md).

## The patch

`patch/vc_render_tifxyz.patch` — **3 files, +250 / −65**:

| File | Why |
|---|---|
| `volume-cartographer/apps/src/vc_render_tifxyz.cpp` | the fix |
| `volume-cartographer/core/src/Zarr.cpp` | `writeZarrAttrs()` omits the `multiscales` block when the voxel size is unknown, instead of publishing a placeholder scale |
| `volume-cartographer/core/include/vc/core/util/Zarr.hpp` | the documented contract for the above |

It applies cleanly to the pinned revision and to `main` at
`b1ef996e357de0b2f24fb30198c6d9c32611d4fb` (2026-09-18), where all three files are
byte-identical to the pinned revision [exec]. The GUI change is **not** included.

## Repository layout

```
README.md      this file
AGENTS.md      operating rules for agents working in this repository

DOCS/          all project documentation
  INDEX.md                    reading order and the evidence map
  PROJECT_STATUS.md           state, gaps, isolation rules, environment notes
  CI_VALIDATION.md            the build-and-run record: environment, commands, results
  RESULTS.md                  everything executed, and everything that was not (§7)
  ROOT_CAUSE_ANALYSIS.md      the cause, at file-and-line resolution
  ARCHITECTURE.md             the fix's design and the rejected alternatives
  FEASIBILITY.md              GO decision, resources, alternatives, out-of-scope items
  TEST_PLAN.md                what was tested, what is blocked, what would falsify this
  RESEARCH.md                 sources, exact commits, licences, assumptions that failed
  PRIZE_REQUIREMENTS.md       Progress Prize rules vs. this project
  PR_DRAFT.md                 the pull request, ready to paste; NOT open
  SUBMISSION_DRAFT.md         Progress Prize submission text; NOT sent
  PROGRESS_PRIZE_CHECKLIST.md what is prepared, and what only the author can attest
  LICENSING_PROPOSAL.md       licence/attribution inventory and the decision it needs
  evidence/
    before-after.png          the before/after figure
    terminal-before-after.png the run's log lines, drawn verbatim
    README.md                 where every value comes from, and the data's licence

patch/vc_render_tifxyz.patch    the proposed fix (three files)
harness/                        the reproducer: real upstream code + tests
ci/                             the validation workflow's helpers and self-tests
research/                       live catalog probe + the raw documents it fetched
  raw_metadata/PROVENANCE.md    URLs, hashes and terms for those documents
tools/                          small scripts (git wrapper, fork-branch staging)
villa/                          read-only clone of villa @ 757f70c (git-ignored)
```

## Development history, kept deliberately

The project's earlier state was wrong in ways that were only found by executing, and
the record is kept rather than tidied, because it is what makes the current claims
checkable. In order:

1. **The patch did not compile.** The first real compile failed on a
   use-before-declaration error: the new block read variables declared ~60 lines
   below it. The harness could not have caught this, because it never compiled the
   file — and its "pristine" copy was itself contaminated, copied from the patched
   working tree. Both are fixed, and both now have tests.
   [`RESULTS.md` §9](DOCS/RESULTS.md)
2. **A 1000× unit regression on the explicit-size path.**
   `--voxel-size 8640 --voxel-unit nanometer` declared `8.64 nanometer` instead of
   `8640 nanometer`. Found by review of the published output, not by the tooling.
   Fixed, with tests that check physical values rather than exit codes.
   [`RESULTS.md` §10](DOCS/RESULTS.md)
3. **An adversarial pre-PR review found three more issues**: the unknown-size branch
   told the operator "no physical scale will be written" while still writing
   `scale = 1.0`; the warning's own advice would have produced a 1000× error; and
   the opened volume was consulted *after* a possibly-stale local cache. All three
   fixed.
   [`RESULTS.md` §11](DOCS/RESULTS.md)

Two of the three earlier defects were invisible to reading and to the test suite of
the day. That is the reason the evidence in this repository is execution-based.

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
./test_render_voxel_size.exe              # this project's: 27/27

# 4. the before/after demonstration over the real documents in research/raw_metadata
./probe_render_voxel_size.exe
```

To re-fetch the live metadata (read-only, ~12 KB):

```powershell
node research/fetch_volume_metadata.mjs research/raw_metadata
```

### Building and running the **renderer** itself

That needs OpenCV, libtiff, Boost `program_options` and more, which are **not**
installable on the machine this was developed on, and neither CMake nor Ninja can
execute a compiler there ([`RESULTS.md` §8.4](DOCS/RESULTS.md)). It is done in CI
instead, on a free GitHub runner, from the public apt package list:

<https://github.com/BioMarco/VoxelScaleGuard/actions/workflows/renderer-validation.yml>

### Applying the patch

```bash
cd villa
git apply --check /path/to/patch/vc_render_tifxyz.patch   # exit 0
git apply         /path/to/patch/vc_render_tifxyz.patch
```

Verified to apply: `git apply --check --reverse` succeeds against the patched tree,
i.e. the patch describes it exactly, and it applies to current upstream `main`.

## What is still open

1. **The PR is not open.** A fork and branch are prepared
   ([`PR_DRAFT.md`](DOCS/PR_DRAFT.md)); publication awaits the author's decision,
   and two things must be written by the author first.
2. **The GUI path remains broken**: `SegmentationCommandHandler.cpp:2076-2078`
   suppresses `--voxel-size` for native-resolution remote volumes, so the CLI fix is
   not reachable from VC3D. Deliberate, separate follow-up.
3. **Coverage is two volumes, one crop, one slice each**, in one build
   configuration. That demonstrates the correction and the absence of a pixel
   regression; it is not a survey.
4. **No prize submission has been made.**
   [`PROGRESS_PRIZE_CHECKLIST.md`](DOCS/PROGRESS_PRIZE_CHECKLIST.md) holds what is
   prepared and what only the author can attest.
5. **A repository licence has not been chosen.** See below.

## Licences

Facts, so the author can decide; no licence has been applied to this repository's
own work yet.

* `villa`'s **root** is **MIT**, Copyright (c) 2024 Vesuvius Challenge — but that
  file does **not** govern the subtree this patch modifies.
  `villa/volume-cartographer/` is **GPL-3.0-or-later**, Copyright (C) 2023 EduceLab
  (`volume-cartographer/LICENSE` and `NOTICE`; corroborated by
  `volume-cartographer/Dockerfile:10`). All three files the patch touches live
  there, so the patch is a derivative of GPL-3.0-or-later code, not MIT code.
  *This section previously said "`villa` is MIT … the patch is a diff against
  MIT-licensed files", which was wrong; corrected 2026-09-18. The full inventory,
  including the rest of villa's per-subproject licence patchwork, is in
  [`DOCS/LICENSING_PROPOSAL.md`](DOCS/LICENSING_PROPOSAL.md) §1.*
* The harness copies eight `volume-cartographer/` translation units
  byte-for-byte. Those are GPL-3.0-or-later by provenance, but they are
  **not tracked in git** (`.gitignore:13` matches `villa/` at any depth), so a
  clone of this repository redistributes none of them; `harness/setup.ps1`
  re-creates them locally. See the proposal §2 and §5.
* `nlohmann/json` (MIT) and `doctest` (MIT) are fetched at build time, not
  vendored.
* `scrollprize.org` content is CC BY-NC 4.0 unless otherwise specified; quoted prize
  rules are attributed to that page. The four committed metadata documents under
  `research/raw_metadata/` and the rendered panels inside
  `DOCS/evidence/before-after.png` are **derived from or copies of Open Data bucket
  material under the same CC BY-NC 4.0 terms**, and that attribution is currently
  recorded only in the proposal — see its §4, which flags it as a gap to close.
* The Open Data bucket is accessed anonymously and read-only.

A proposal for licensing this repository's original scripts and documentation, with
the notices and attributions it would require, is in
[`DOCS/LICENSING_PROPOSAL.md`](DOCS/LICENSING_PROPOSAL.md). **It is a proposal
awaiting the author's decision, not an applied licence.** It also records a real
conflict with the prize rules' wording ("permissive license … to accept the prize")
that the author should raise with the organisers rather than resolve silently.

## Attribution

The approach of having the renderer consult the volume's remote voxel size is
**NicolasHuberty**'s, from PR #1417, which lapsed to an inactivity bot rather than
being rejected. The `samplePixelSize` schema handling,
`resolveLocalStoreVoxelSize`, and `Volume::voxelSize()`'s semantics are
**Bullo27**'s and the villa maintainers' (PRs #1227, #1229, #1454). The VC3D
enable-predicate diagnosis is **Bullo27**'s, from PR #1228. The issue reports are
**DarthCeltic**'s (#1403) and **Bullo27**'s (#1226). See
[`FEASIBILITY.md` §7](DOCS/FEASIBILITY.md).

## Contributing

Read [`AGENTS.md`](AGENTS.md) first. It contains the verification rules, the
attribution requirements, and the environment traps that will otherwise cost you an
hour.
