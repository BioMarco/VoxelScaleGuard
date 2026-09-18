# PROJECT_STATUS

**Project:** VoxelScale Guard
**Repository:** <https://github.com/BioMarco/VoxelScaleGuard> (public)
**Workspace:** `C:\Users\marco\Documents\DeepSeek\VoxelScaleGuard`
**Status:** diagnosis verified and demonstrated on real data; patch written and
logic-verified; **not compiled or run** — see §6
**Upstream revision analysed:** `ScrollPrize/villa` @
`757f70c0140a4cfbbbd44975ef09558444b96980` (`main`)
**Last synced:** `main` @ `6de80df` (initial commit `4a77205`)

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
  nothing about `villa` left the machine.
* No pull request was opened, and nothing was published — **with one authorised
  exception:** this project's own repository,
  <https://github.com/BioMarco/VoxelScaleGuard>, was created and synced at the
  user's explicit request. That is this project's own artefact, not upstream's.
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
| 10 | A **prebuilt VC3D Windows package built from the pinned commit** (`VC3D-757f70c-2026-09-15-win64.zip`, 148 MB) is published on the `latest` release, and it bundles the `vc_*` CLI tools | GitHub releases API; `vc3d-windows.yml` smoke-tests `$bin\vc_tifxyz_trim.exe` in the same directory as `VC3D.exe` |
| 11 | That package makes the **before** side of the demonstration obtainable from the real shipped binary, with no build — but not the **after** side, which still needs the `ci-windows-mingw` build (MSYS2 + the prebuilt `vc3d-deps` archive, not a from-source vcpkg closure) | **not yet attempted**; procedure in `DOCS/RESUME.md` §5 |

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

**Patch** — `patch/vc_render_tifxyz.patch`, **3 files, +250/−65** after the review fixes
described in `RESULTS.md` §9. Applies exactly to the pinned revision
(`git apply --check --reverse` succeeds on the patched tree).

**Harness** — `harness/`, which compiles the pinned revision's real
`VoxelSizeMetadata.cpp` / `RemoteUrl.cpp` / `Json.cpp` byte-for-byte and runs:
upstream's own suite unmodified (**13 cases / 54 assertions**, as a control) plus
this project's tests (**27 cases / 208 assertions**), plus a before/after probe.
The suite also asserts that the pristine `vc_render_tifxyz.cpp` copy really is
unpatched, and that the patch declares every name its new block reads before using
it — the defect the first real compile exposed (§9).

**Documents** — all ten required, listed in §7.

## 6. What is NOT done, and why

This is the most important section. Three of the four gaps it listed are now
closed, and closing them did not go the way this project expected: **the patch did
not compile.** See `RESULTS.md` §9.

| Gap | Status |
|---|---|
| **The patched binary was never compiled or run** | **CLOSED 2026-09-16.** Both binaries compile and run in CI on GitHub-hosted runners, from the pinned revision. `RESULTS.md` §9, `CI_VALIDATION.md`. The local machine still cannot build it (`RESULTS.md` §8.4), which is why a workflow does |
| **No `.zattrs` or TIFF was produced** | **CLOSED.** Real `.zattrs` with `micrometer` / `[8.64, 8.64, 8.64]`, real TIFF `XResolution = 2939.8147`, from a real render. `CI_VALIDATION.md` §7 |
| **No render was run on real data** | **CLOSED.** Four runs over `PHerc0009B` and `PHerc0172`; decoded pixels byte-identical. `CI_VALIDATION.md` §6–7 |
| **The patch as first committed did not compile** | **FOUND AND FIXED.** The new resolution block used variables declared ~60 lines below it. This is the most valuable single result in the project: a small, reviewed, "logic-verified" change that could never have built. The harness now tests for the class of defect (`RESULTS.md` §9.2) |
| **The `harness` "pristine" copy was actually patched** | **FOUND AND FIXED.** `setup.ps1` copied from the patched working tree, so the before/after comparison compared the patch with itself. It now uses `git show <commit>:<path>` |
| **The GUI path remains broken** | **Open by decision.** `SegmentationCommandHandler.cpp:2076` is deliberately a separate commit, because it changes GUI behaviour and its predicate has a lapsed history (#1228). Exact edit in `FEASIBILITY.md` §8 |
| **The "no usable voxel size" branch** | **CLOSED 2026-09-17.** It is driven end to end against a local-only store with no metadata document, asserting on the emitted files that nothing is declared. This row said "unit-tested only" until 2026-09-18; that was stale. Getting the check to exercise the branch took two attempts that passed vacuously — `RESULTS.md` §11.1 |
| **Coverage is two volumes, one crop, one slice** | **Open.** Enough to demonstrate the correction and the absence of a pixel regression; not a survey |
| **The `vc_zarr_to_tiff` schema gap** | **Open.** Same class of defect (local-only, top-level key only), but no remote path and not needed for the reported problem. Documented as a candidate, not developed |
| **The live-S3 test still pins the legacy volume** | **Open.** Changing an existing live test's fixture is a maintainer decision. Proposed in `PR_DRAFT.md` |
| **No repository licence is applied, and the licence reasoning was wrong** | **CLOSED 2026-09-18.** The wrong reasoning was found and corrected in five documents, and the licence is now applied: `LICENSE` (MIT, original work only), `LICENSE-GPL-3.0.txt` (verbatim upstream GPL text), `NOTICE.md` (the authoritative path-by-path map), `DATA_ATTRIBUTION.md` (third-party data), and GPL headers on the four derived files. Two residual uncertainties are recorded in `NOTICE.md` §2.4 rather than treated as settled. `RESULTS.md` §12.2 and §14 |
| **Third-party Open Data attribution was missing from the repository's own licence material** | **CLOSED 2026-09-18.** `DATA_ATTRIBUTION.md` names the rights holders, gives source links and **both** dataset citations, itemises the transformations applied to the figure, and is linked from `README.md`, `NOTICE.md`, `DOCS/evidence/README.md` and `research/raw_metadata/PROVENANCE.md`. `RESULTS.md` §14.1 |
| **The documented patch-regeneration command was wrong** | **FOUND AND FIXED 2026-09-18.** `AGENTS.md` §4 named one path where the patch covers three, so following it produced an 18,480-byte patch that reverse-applied cleanly while having dropped both Zarr hunks. Fixed, and CI now asserts the touched-file count. `RESULTS.md` §12.1 |
| **No submission and no PR** | By instruction. The evidence needed for one now exists; the paperwork is the remaining work |

**The patch is binary-verified as of 2026-09-16.** It compiles, runs on real
published volumes, corrects the declared physical scale in both output formats, and
leaves the rendered pixels byte-identical. What it is *not* yet is reachable from
the GUI, or verified beyond two volumes.

## 7. Deliverables

Documentation lives in `DOCS/`; `README.md` and `AGENTS.md` are the only
Markdown files at the root. Reading order and an evidence map: `DOCS/INDEX.md`.

| File | Contents |
|---|---|
| `DOCS/INDEX.md` | reading order and the evidence map |
| `DOCS/RESUME.md` | handoff for a new session: state verification, next step, decisions, traps |
| `DOCS/PROJECT_STATUS.md` | this file |
| `DOCS/RESEARCH.md` | sources, exact commits, licences, and three brief assumptions that failed |
| `DOCS/PRIZE_REQUIREMENTS.md` | Progress Prize rules vs. what this project considers useful |
| `DOCS/ROOT_CAUSE_ANALYSIS.md` | the cause at file-and-line resolution |
| `DOCS/FEASIBILITY.md` | **GO**, with scope, resources, alternatives, and out-of-scope items |
| `DOCS/ARCHITECTURE.md` | the fix's design, ordering argument, and rejected alternatives |
| `DOCS/TEST_PLAN.md` | executed vs. blocked, and what would falsify each claim |
| `DOCS/RESULTS.md` | everything executed, with exit codes; and §7, what was not |
| `DOCS/PR_DRAFT.md` | pull request draft, **not submitted** |
| `DOCS/SUBMISSION_DRAFT.md` | Progress Prize draft, including the form's four questions answered in the form's own order; **not submitted** |
| `DOCS/PROGRESS_PRIZE_CHECKLIST.md` | the submission, field by field, and what only the author can attest |
| `DOCS/LICENSING_PROPOSAL.md` | How the licence position was reached; approved and applied 2026-09-18; `NOTICE.md` governs |
| `DOCS/PROGRESS_PRIZE_QUESTION.md` | The prepared question for the organisers. **Not sent** |
| `README.md` | landing page: description, layout, build and run, and the licence summary |
| `AGENTS.md` | operating rules, verification requirements, attribution, environment traps |
| `LICENSE` | MIT — this project's original work only, with what it does not cover |
| `LICENSE-GPL-3.0.txt` | the GPL v3 text, copied byte-for-byte from upstream |
| `NOTICE.md` | the authoritative path-by-path licence map |
| `DATA_ATTRIBUTION.md` | the third-party data, its terms and its required citations |
| `patch/README.md` | the patch's GPL notice, modification dates, and its not-applied status |
| `patch/vc_render_tifxyz.patch` | the fix |
| `harness/` | the reproducer and its tests |
| `research/` | the live catalog probe, the raw documents it fetched, and the probe summary |
| `research/raw_metadata/PROVENANCE.md` | URL, hash and terms for each fetched document |
| `tools/git.ps1` | git wrapper carrying the transient `safe.directory` flag |

## 8. Environment notes worth keeping

* **MSVC works**: `cl.exe` 14.34.31933 with Windows SDK **10.0.22000.0** (the
  10.0.22621.0 paths in a naive setup do not exist on this machine).
* **CMake and Ninja cannot be used here**: CMake fails to launch its own
  subprocesses in this sandbox. `harness/build.ps1` invokes `cl.exe` directly.
  `harness/CMakeLists.txt` is kept for normal environments.
* **Native commands cannot have output redirected or piped**: `cmd > file`,
  `cmd | ...`, `cmd 2>&1` fail with `StandardOutputEncoding is only supported when
  standard output is redirected`. Commands are recorded in the form that works.
  `git diff --output=…` is used instead of a shell redirect, and
  `research/fetch_volume_metadata.mjs` writes its own `--out` file for the same
  reason.
* **Git ownership**: both this working copy and the `villa` clone are owned by
  `BUILTIN/Administrators`. `~/.gitconfig` is outside the sandbox, so the
  transient `-c safe.directory='*'` flag is required and is wrapped by
  `tools/git.ps1`.
* **`git push` needs a stdio pipe** for the HTTPS transport, which the sandbox
  denies. Every push in this project therefore required a one-shot escalation;
  the transfer itself is a normal authenticated push through Git Credential
  Manager.
* **`Invoke-WebRequest` fails TLS** in this environment; Node's `fetch` works,
  which is why `research/fetch_volume_metadata.mjs` and `harness/fetch_deps.mjs`
  are Node scripts.
* **Command execution is intermittent**: several external processes returned empty
  output with no exit code mid-session and later recovered. No work depends on a
  single such invocation.
* **`.gitattributes` pins `* -text`.** Line-ending normalisation would corrupt
  `patch/vc_render_tifxyz.patch` (git apply is EOL-sensitive) and would break the
  byte-for-byte copy guarantee in `harness/setup.ps1`.

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
| All ten required documents written |
| `AGENTS.md` added: verification rules, attribution, scope boundaries, environment traps |
| Documentation reorganised into `DOCS/`; `DOCS/INDEX.md` added; links and code comments updated |
| Tests re-run after the reorganisation: 13/13 and 27/27 still pass (count grew again on 2026-09-16; see `RESULTS.md` §9) |
| `.gitattributes` (`* -text`) added so line-ending normalisation cannot corrupt the patch or the byte-for-byte copies |
| Repository created and pushed to <https://github.com/BioMarco/VoxelScaleGuard> at the user's request (`4a77205`) |
| Zero-byte `metadata_probe.json` (an artefact of a failed shell redirect) replaced by a real 3812-byte summary; the probe script now writes its own `--out` file (`6de80df`) |
| `DOCS/RESUME.md` written: state-verification commands, the next step with acceptance criteria, the decisions already made and why, the environment traps, and a ready-to-paste continuation prompt |
| Reconnaissance for the handoff: found that the published prebuilt Windows package is built from the **pinned commit** and bundles the `vc_*` CLI tools, which makes the *before* half of the demonstration obtainable without a build |
| Remaining: build the patched binary and produce artifact-level evidence — **blocked, needs authorisation**. Procedure in `DOCS/RESUME.md` §5 |
| Build-and-run moved to GitHub Actions; run 35249590299 builds baseline and patched from one commit and renders on two public volumes (`RESULTS.md` §9) |
| Patched pixels verified byte-identical; the regression check hashes decoded pixels, not files (`RESULTS.md` §9.5) |
| ×1000 unit regression found in the patch's own explicit-size path, fixed and pinned by tests (`RESULTS.md` §10) |
| Pre-PR adversarial review: fabricated `scale` on the unknown-size branch, misleading warning text, and the local/open-volume tier order — all fixed (`RESULTS.md` §11) |
| Terminal-evidence figure generated from verbatim CI logs, with a self-test asserting every displayed line (`DOCS/evidence/README.md`) |
| Licence and attribution inventory: `volume-cartographer/` is GPL-3.0-or-later, not MIT; five documents corrected, `DOCS/LICENSING_PROPOSAL.md` written, third-party data provenance recorded (`RESULTS.md` §12) |
| `AGENTS.md` §4's patch-regeneration command corrected (it named one path where the patch covers three) and CI given a touched-file-count assertion (`RESULTS.md` §12.1) |
| `PATCH_IDENTICAL=no` promoted from a printed line to a step failure, in the same pass (`RESULTS.md` §12.1) |
| Probe tool narrowed to `*.json` so the new `PROVENANCE.md` is not counted as a probed document; rebuilt and re-run, still 4 documents / 3 divergences (`RESULTS.md` §12.5) |
| `.gitignore`'s accidental-but-correct exclusion of `harness/src/villa/**` documented as deliberate (`DOCS/LICENSING_PROPOSAL.md` §5) |
| Run 35374993168 (commit `953d5f6`, 25/25 steps) exercised the two new workflow assertions in CI: `PATCH_FILES=3`, `PATCH_IDENTICAL=yes`, `physical-size failures: 0`, decoded pixels identical (`RESULTS.md` §12.6) |
| Two earlier runs on the same commit were cancelled by the workflow's own `cancel-in-progress: true`, not by any failure; recorded so the next session does not misread a cancelled run as a broken build (`RESULTS.md` §12.7) |
| Prize rules and submission form re-read; the form's fields and Terms transcribed, and the four form questions answered in `DOCS/SUBMISSION_DRAFT.md` (`RESULTS.md` §12.3) |
| Still not done, by instruction: no PR opened, no submission sent, no `LICENSE` applied, no merge to `main` |
| Licence and third-party attribution **applied** (author-approved): `LICENSE` (MIT, original work only, with what it excludes), `LICENSE-GPL-3.0.txt` (verbatim upstream GPL text, hash-verified), `NOTICE.md` (authoritative map, upstream notices preserved, modification dates, two uncertainties recorded), `DATA_ATTRIBUTION.md` (rights holders, both dataset citations, transformations), `patch/README.md`, and GPL headers on the four derived files (`RESULTS.md` §14) |
| Organisers' question drafted in English and **not sent**: `DOCS/PROGRESS_PRIZE_QUESTION.md` |
| Prize page re-read 2026-09-18: deadline, awards and Terms all unchanged; the "permissive license" wording discrepancy is still live and remains unanswered (`RESULTS.md` §14.5) |
| Still not done, by instruction: no PR opened, no submission sent, no message to the organisers, no merge to `main` |
