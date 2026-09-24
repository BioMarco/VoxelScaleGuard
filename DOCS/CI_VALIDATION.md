# CI_VALIDATION — building and running the real renderer, before and after

**Status: done.** Both binaries were compiled from the pinned revision on a
GitHub-hosted runner, and both were run against real public catalog data with
identical inputs. This document is the record: environment, exact commits,
commands, and results.

Runs:

* **<https://github.com/BioMarco/VoxelScaleGuard/actions/runs/35374993168>** — the
  newest green run (2026-09-18, commit `953d5f6`, 25 steps, none failed). Same
  build-and-render checks, plus the two assertions added that day:
  `PATCH_FILES=3` and a `PATCH_IDENTICAL` verdict that now fails the step rather
  than printing a line. First run in which those executed.
* <https://github.com/BioMarco/VoxelScaleGuard/actions/runs/35249590299> — the run
  the evidence figures were built from; cited throughout as the reference run.
* <https://github.com/BioMarco/VoxelScaleGuard/actions/runs/35151048117> — the
  earlier green run: full build, before/after render, physical-size checks of §8,
  and the ×1000 fix recorded in §8.1.
* <https://github.com/BioMarco/VoxelScaleGuard/actions/runs/35137825520> — the
  first green run, which established §7.

Two runs on that commit were cancelled before this one, and neither indicates a
problem: the workflow sets `concurrency: … cancel-in-progress: true` for the branch,
so the push-triggered run (`35374933203`) was cancelled by the first manual dispatch,
and that dispatch (`35374813151`) was in turn cancelled by the second. Only the last
run on a branch survives under that setting. `RESULTS.md` §12.7 has the table.

It closes `RESULTS.md` §7.1–7.3. `RESULTS.md` §10 records the later unit regression
and its fix. §7.4 and §7.5 remain open.

---

## 1. Answers, up front

| Question | Answer |
|---|---|
| Original renderer compiled? | **Yes** — `build/baseline/bin/vc_render_tifxyz` |
| Patched renderer compiled? | **Yes** — but it did **not** before this session; see §5 |
| Render executed on public data? | **Yes** — `PHerc0009B` and `PHerc0172`, four runs |
| `.zattrs` unit and scale corrected? | **Yes** — `micrometer` / `[8.64, 8.64, 8.64]` vs `nanometer` / `[1, 1, 1]` |
| TIFF resolution present and right? | **Yes** — `2939.81…` px/inch where the baseline wrote none |
| Pixels unchanged (regression)? | **Yes** — decoded-pixel SHA-256 identical on both volumes |
| Error/edge cases exercised? | **Yes**, with their **physical values** checked, not just exit codes — see §8 |
| Unit coherence on the explicit-size path? | **Yes** — nm/µm/mm/m each declare a number and unit denoting the same physical size; this caught a ×1000 error introduced by the first version of the patch (§8.1) |

---

## 2. Why this runs on GitHub Actions and not locally

Established by execution on the development machine, recorded in `RESULTS.md`
§8.4:

* **No build system can execute a compiler here.** `cmake --version` and
  `ninja --version` both work, but `cmake` fails with `Accesso negato` when its
  `project()` step probes `ninja.exe --version`, and `ninja` hangs when a build
  rule spawns a process. Every upstream preset is Ninja-based
  (`CMakePresets.json:9`), so this blocks all of them, and a newer CMake does not
  help because the CMake version gate is a *separate* failure.
* **The dependency closure is absent.** The renderer's translation unit needs
  `<opencv2/imgproc.hpp>`, `<tiffio.h>` and `<boost/program_options.hpp>`; none is
  installed locally, and `vc_core` is 50 further translation units.
* **The prebuilt dependency bundles upstream CI uses are not publicly
  readable.** An anonymous `ghcr.io` token request for
  `repository:scrollprize/vc3d-deps:pull` returns **HTTP 403** for both the
  `windows:sha-d84dfa07…` and `linux:sha-0c371b1d…` tags
  (`research/recon_ghcr_bundles.mjs`). The official Windows workflow authenticates
  with the runner's `${{ github.token }}`; that does not transfer to another
  repository.

The route taken instead uses only publicly available packages: the same package
list upstream's own `scripts/install_build_deps.sh` installs from the Ubuntu
archive, applied group by group so a single unavailable package is visible rather
than fatal. No private registry, no credentials beyond the automatic
`GITHUB_TOKEN`, no paid runners.

---

## 3. Environment and identification

* Workflow: `.github/workflows/renderer-validation.yml`
* Runner: `ubuntu-24.04` (GitHub-hosted, free tier), 4 vCPU
* Trigger: `workflow_dispatch`, plus push to `ci/renderer-validation`
* villa revision: `757f70c0140a4cfbbbd44975ef09558444b96980`, fetched with
  `git fetch --depth 1`, checked out detached, HEAD asserted equal to the input,
  and the worktree asserted clean before patching
* Build configuration, identical for both binaries:
  `-DCMAKE_BUILD_TYPE=QuickBuild -DVC_QUICKBUILD_OPT_LEVEL=0 -DVC_TESTING=OFF
  -DVC_BUILD_APPS=ON -DVC_BUILD_FLATBOI=OFF -G Ninja`
* Toolchain in the run: **gcc 13.3.0**, **cmake 3.31.6**, **ninja 1.13.2**
* Target: `vc_render_tifxyz` only (not `vc_cli_all`, not `VC3D`)

---

## 4. How the two binaries are produced from one revision

1. Fetch and check out the pinned commit; assert `HEAD == VILLA_COMMIT` and that
   `git status --porcelain` is empty.
2. Configure and build → `build/baseline/bin/vc_render_tifxyz`.
3. `git apply --check` then `git apply patch/vc_render_tifxyz.patch` on that same
   detached tree. The patch is applied by `git apply`; **no villa source is edited
   by hand.** The resulting `git diff` is compared byte-for-byte with the
   committed patch: `PATCH_IDENTICAL=yes`. Additionally, `git diff --name-only` must
   list **exactly 3** files (`PATCH_FILES=3`).

   *Both of those are now asserted rather than logged, corrected 2026-09-18.* Until
   then a `PATCH_IDENTICAL=no` only printed a line, so a mismatch would have left a
   green run whose "patched" binary came from something other than the committed
   patch; and the file-count check did not exist, which is how `AGENTS.md` §4's
   one-path regeneration command could have gone unnoticed (`RESULTS.md` §12.1).
   `ci/preflight_workflow.py` now checks the committed artefact's file count too.
4. Configure and build → `build/patched/bin/vc_render_tifxyz`.

The binaries are **not** copied out of their build trees. Upstream links the `vc_*`
libraries as shared objects with an `$ORIGIN/../lib` rpath, so a copied executable
fails to load (`error while loading shared libraries: libvc_flattening.so`); every
invocation runs them in place.

---

## 5. Build results — and the defect the build found

| Step | Result |
|---|---|
| Baseline build | **OK** (62 s) |
| Patch applies to the pinned tree | **OK**, `PATCH_IDENTICAL=yes` |
| Patched build | **OK** (62 s) |
| `--help` identical between the binaries | **Yes** (`HELP_IDENTICAL=yes`) |

**The first compile of the patch failed.** On run 35134946141:

```
vc_render_tifxyz.cpp:1449:43: error: 'hasExplicitVoxelSize' was not declared in this scope
vc_render_tifxyz.cpp:1450:13: error: 'voxel_unit' was not declared in this scope
```

and the same for `explicitVoxelSize`, `base_voxel_size`, `zarr_voxel_unit` and
`render_level_voxel_size`. The new resolution block had been inserted ~60 lines
before the declarations it reads. The patch as committed could not have built.

It was fixed by moving the block to immediately after the declarations, with no
logic change. Diffstat moved from +177/−65 to +176/−63; `git apply --check
--reverse` exits 0 and a reverse-then-forward round trip reproduces the same file
byte-for-byte.

*(§8.1 later changed the same region again, so this document at that point recorded
**+218/−64**. Both figures above are the state at the time each defect was fixed and
are kept as the record. **The patch's current diffstat is three files, +358/−102** (it became +250/−65 on 2026-09-17, and grew again with the review fixes of 2026-09-23)
— `RESULTS.md` §11.5 added `core/src/Zarr.cpp` and `core/include/vc/core/util/Zarr.hpp`.
Corrected here 2026-09-18; the previous note stopped at +218/−64 and so understated
the patch's file count, which is the same staleness that produced the defect in
`RESULTS.md` §12.1.)*

The harness could not have caught this, and in fact hid it: `harness/setup.ps1`
was copying the file from `villa/`'s working tree, which is patched in place, so
the "pristine" copy was the patched file. It now takes the file from
`git show <commit>:<path>`. Two tests were added, described in `RESULTS.md` §9.2.

---

## 6. Rendering experiment

One invocation per binary per volume, identical inputs and parameters. Only the
executable differs.

Public inputs, read anonymously from `vesuvius-challenge-open-data`:

| Role | Object |
|---|---|
| Volume under test | `PHerc0009B/volumes/20250521125136-8.640um-1.2m-116keV-masked.zarr` |
| Mesh for it | `PHerc0009B/segments/20250510172639/mesh/20250510172639-on-20250521125136-8.64um.tifxyz` |
| Control volume | `PHerc0172/volumes/20241024131838-7.910um-53keV-masked.zarr` |
| Control mesh | `PHerc0172/segments/20250917143559-w062…/mesh/20250917143559-on-20241024131838-7.91um.tifxyz` |

Arguments, identical for both binaries:

```
-g 0 --scale 1 -n 1 --cache-gb 4 --crop-x 3145 --crop-y 3412
--crop-width 128 --crop-height 128
```

`--volume` names a local **cache directory**, not a Zarr store; chunks stream from
S3 and persist there, so only what the segment touches is downloaded. Both binaries
share one cache directory, and each writes to its own output paths so no run can
skip work by finding existing output.

The cache directory deliberately contains **no** `meta.json` or `metadata.json`.
That is the experiment: with no store document on the local filesystem, the shipped
reader resolves nothing while the patched renderer falls back to the volume it has
already opened. Copying the store document in would make the *baseline* resolve the
value too and would compare nothing. The consequence is that both baseline rows
below are the "unresolved" case — including PHerc0172, which the deployed reader
*does* handle when its `meta.json` is present, and which the standalone probe does
resolve as the control (`RESULTS.md` §2).

---

## 7. Measurements

### 7.1 The log line, from the real binaries

| Run | Line | Exit |
|---|---|---|
| 0009B baseline | `Voxel size: 1.0 (no metadata found; override with --voxel-size)` | 0 |
| 0009B patched | `Voxel size (remote volume metadata): 8.64 micrometer` | 0 |
| 0172 baseline | `Voxel size: 1.0 (no metadata found; override with --voxel-size)` | 0 |
| 0172 patched | `Voxel size (remote volume metadata): 7.91 micrometer` | 0 |

### 7.2 OME-Zarr `.zattrs`

Per-level scale, group `-g 0`:

| Run | unit | level 0 | level 5 |
|---|---|---|---|
| 0009B baseline | `nanometer` | `[1, 1, 1]` | `[1, 32, 32]` |
| 0009B patched | `micrometer` | `[8.64, 8.64, 8.64]` | `[8.64, 276.48, 276.48]` |
| 0172 baseline | `nanometer` | `[1, 1, 1]` | `[1, 32, 32]` |
| 0172 patched | `micrometer` | `[7.91, 7.91, 7.91]` | `[7.91, 253.12, 253.12]` |

Declared physical voxel size at level 0: baseline `0.001 µm` (wrong by ×8640 and
×7910 respectively), patched `8.64 µm` and `7.91 µm` (correct).

### 7.3 TIFF tags

| Run | XResolution | YResolution | ResolutionUnit |
|---|---|---|---|
| 0009B baseline | **absent** | absent | absent |
| 0009B patched | `2939.814697265625` | same | 2 (inch) |
| 0172 baseline | **absent** | absent | absent |
| 0172 patched | `3211.125244140625` | same | 2 (inch) |

`25400/8.64 = 2939.8148…` and `25400/7.91 = 3211.1252…`: the emitted resolution is
the correct physical scale to six significant figures. The baseline wrote no
resolution tag at all on this path.

Confirmed with a second, independent reader — `tiffinfo` from libtiff, not Pillow:

```
--- 0009B-baseline: out/0009B-baseline.tif/00.tif
--- 0009B-patched:  out/0009B-patched.tif/00.tif
  Resolution: 2939.81, 2939.81 pixels/inch
--- 0172-baseline: out/0172-baseline.tif/00.tif
--- 0172-patched:  out/0172-patched.tif/00.tif
  Resolution: 3211.13, 3211.13 pixels/inch
```

Both baseline runs print no `Resolution` line, which is what "the tag is absent"
looks like from the outside.

### 7.4 Are the pixels unchanged?

| Volume | Decoded-pixel SHA-256 | File bytes |
|---|---|---|
| 0009B | `7d92aec89d3638940fd45e62a0269ae99404391451bdc5dde267e4b2aec65a9b` on **both** sides | 14244 → 14296 |
| 0172 | `4fe7b59af6de3b665b67788cc2f99892ab827efae3a467342b3bb4e3bc8e5bfe` on **both** sides | 806 → 858 |

**Verdict: decoded pixels identical, file bytes differ** — which is the expected
outcome, because the patched TIFF carries the resolution tag the baseline omitted.

The distinction matters and is the reason the check is on decoded pixels: the user
brief for this work explicitly warns against treating two images as identical on
the basis of file size. Here file size and file digest both *differ* while the
image is unchanged, so the opposite error was the live risk. `ci/selftest_compare.py`
exercises both directions against synthetic fixtures, including the case where
pixels really do differ.

---

## 8. Error and edge cases

Driven through the patched binary's real CLI. Exit codes are recorded, and for every
successful run the emitted `.zattrs` and TIFF tags are converted to micrometres and
compared against the physical size the command line asked for
(`ci/check_physical_size.py`). That second half is not decoration: it is the check a
number/unit mismatch survives, and it is what caught the defect in §8.1.

| Case | Invocation | Exit | Physical check |
|---|---|---|---|
| Zero size | `--voxel-size 0` | **1** | no `.zattrs`; `Error: --voxel-size must be a positive finite number` |
| Negative size | `--voxel-size -3` | **1** | same error; no `.zattrs` |
| Non-finite | `--voxel-size nan` | **1** | same error; no `.zattrs` |
| Unknown unit | `--voxel-size 7.91 --voxel-unit furlong` | **1** | `Error: unsupported --voxel-unit: furlong`; no `.zattrs` |
| Nanometres | `--voxel-size 8640 --voxel-unit nanometer` | 0 | `.zattrs` 8640 nm = **8.64 µm**; TIFF = 8.64 µm |
| Micrometres | `--voxel-size 8.64 --voxel-unit micrometer` | 0 | `.zattrs` 8.64 µm; TIFF = 8.64 µm |
| Millimetres | `--voxel-size 0.00864 --voxel-unit millimeter` | 0 | `.zattrs` 0.00864 mm = **8.64 µm**; TIFF = 8.64 µm |
| Metres | `--voxel-size 0.00000864 --voxel-unit meter` | 0 | `.zattrs` 8.64e-06 m = **8.64 µm**; TIFF = 8.64 µm |
| A different size | `--voxel-size 7910 --voxel-unit nanometer` | 0 | `.zattrs` = **7.91 µm**; TIFF = 7.91 µm |
| Baseline, zero size | `--voxel-size 0` | **1** | same error as the patched binary |

All four spellings of one physical size declare the caller's own number *and* the
caller's own unit, so the pair denotes the same 8.64 µm in each case; the 7.91 µm case
shows the check is not asserting a constant.

The last row is the control for "this patch introduces no regression in input
validation": both binaries reject a non-positive `--voxel-size`, because that check
predates the patch. What the patch changes is not validation of what the user
supplies, but whether a value is found at all when the user supplies none.

### 8.1 A 1000x unit regression, found here and fixed

*The first version of the patch failed this section.* It wrote `.zattrs` with scale
`8.64` and unit `nanometer` for `--voxel-size 8640 --voxel-unit nanometer` --
declaring 8.64 nm where 8640 nm was asked for, a silent x1000 error, while the TIFF
tag was correct. Cause, fix and tests are in `RESULTS.md` §10.

This is why the physical check exists and why it converts both sides to micrometres.
An earlier version of this section recorded only exit codes and a raw `.zattrs`
summary, and read as a pass while the declared value was wrong by 1000x.

**Not exercised:** the "no usable voxel size anywhere" path, i.e. the branch that
emits the warning and writes no physical scale. With this volume the patched binary
finds the value in the volume it has already opened, which is the fix, so the
branch is unreachable without severing the volume from its own metadata. It is
logic-verified in the harness (`RESULTS.md` §3) and remains the one path this
workflow does not drive end to end. Stated here rather than omitted.

---

## 9. Problems still open

* **The GUI path.** `SegmentationCommandHandler.cpp:2076-2078` still suppresses
  `--voxel-size` for a native-resolution remote volume, so the CLI fix is not
  reachable from the route most users take. Deliberately not part of this patch
  (`RESUME.md` §6); documented as a separate follow-up in `FEASIBILITY.md` §8.
* **The "no metadata" warning branch**, as described in §8.
* **Only two volumes and one crop per volume were rendered.** The public catalog
  has 46 scrolls; this is a demonstration that the fix works, not a survey.
* No pull request has been opened against `ScrollPrize/villa`, and nothing has been
  submitted for a prize.

---

## 10. Reproducing this

```bash
# locally: syntax/behaviour pre-flight for the workflow and its reporter
python ci/preflight_workflow.py     # YAML, bash -n per step, patch round-trip
python ci/selftest_compare.py       # reporter self-test on synthetic fixtures
python ci/summarize_zattrs.py <path>  # one-line .zattrs summary

# remotely: dispatch the workflow (needs Actions: write on the repository)
#   GitHub UI -> Actions -> "Renderer validation (baseline vs patched)" -> Run workflow
```

Artefacts retained for 30 days: `ci-out/` (toolchain, dependency presence, build
logs with `/usr/bin/time -v`, the patch diff, `comparison.txt`, `edge-cases.txt`,
`tiffinfo.txt`, per-run render logs and exit codes) and `out/` (the rendered
`.zattrs` and TIFFs). Size is bounded by the 128×128 crop and single-slice render.
Downloading them needs authentication; the job log itself is readable and contains
the same comparison report.

