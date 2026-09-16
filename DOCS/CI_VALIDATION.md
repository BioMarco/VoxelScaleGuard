# CI_VALIDATION — building and running the real renderer, before and after

**Status: this document records a workflow that has been written, committed and
dispatched, and is currently being iterated on. Read the "Current state" table in
§1 before quoting anything below it as a result.**

This is the file that closes `RESULTS.md` §7.1–7.3, or records why it could not be
closed. It exists because those sections required something the development
machine cannot provide: an execution of the patched binary.

---

## 1. Current state

| Question | Answer |
|---|---|
| Original renderer compiled? | *see §5 — filled in from the run* |
| Patched renderer compiled? | *see §5* |
| Render executed on public data? | *see §6* |
| `.zattrs` unit and scale corrected? | *see §7* |
| TIFF resolution tag present and right? | *see §7* |
| Pixels unchanged (regression)? | *see §7* |
| Error/edge cases exercised? | *see §8* |

Nothing in this file is a projection. Each row is filled from a captured command's
output, and the run URL is given so it can be checked.

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

The route taken instead uses only publicly available packages: the same apt
dependency list upstream's own `scripts/install_build_deps.sh` installs from the
Ubuntu archive. No private registry, no credentials beyond the automatic
`GITHUB_TOKEN`, no paid runners.

---

## 3. Environment and identification

* Workflow: `.github/workflows/renderer-validation.yml`
* Runner: `ubuntu-24.04` (GitHub-hosted, free tier)
* Trigger: `workflow_dispatch`, plus push to `ci/renderer-validation`
* villa revision: `757f70c0140a4cfbbbd44975ef09558444b96980`, checked out detached
  by the workflow, and asserted by the workflow itself
* Build configuration, identical for both binaries:
  `-DCMAKE_BUILD_TYPE=QuickBuild -DVC_QUICKBUILD_OPT_LEVEL=0 -DVC_TESTING=OFF
  -DVC_BUILD_APPS=ON -DVC_BUILD_FLATBOI=OFF -G Ninja`
* Target: `vc_render_tifxyz` only (not `vc_cli_all`, not `VC3D`)

Toolchain and package versions are captured by the run itself into
`ci-out/toolchain.txt` and `ci-out/verify-deps.txt`.

---

## 4. How the two binaries are produced from one revision

The requirement is that the original and the modified renderer derive from the
same commit, with the patch as the only difference.

1. `git clone` of `ScrollPrize/villa`, `git checkout --detach <pinned>`, then
   `test "$(cat ci-out/villa-HEAD.txt)" = "${VILLA_COMMIT}"`.
2. Configure and build → `ci-out/vc_render_tifxyz.baseline`.
3. `git apply --check` then `git apply patch/vc_render_tifxyz.patch` on that same
   detached tree. The patch is applied by `git apply`; **no villa source is edited
   by hand**, so the applied change is exactly the committed artefact. The
   workflow additionally compares the resulting `git diff` against
   `patch/vc_render_tifxyz.patch` byte-for-byte and records
   `PATCH_IDENTICAL=yes|no`.
4. Configure and build → `ci-out/vc_render_tifxyz.patched`.

Both binaries are archived as artefacts so their identity can be re-checked
without rerunning the build.

---

## 5. Build results

*Filled in from the run. Build logs: `ci-out/build-baseline.log`,
`ci-out/build-patched.log` (they contain the full `cmake --build` output and
`/usr/bin/time -v` resource figures).*

---

## 6. Rendering experiment

One invocation per binary per volume, with identical inputs and parameters. The
only difference is the executable.

Public inputs (all read anonymously from `vesuvius-challenge-open-data`):

| Role | Object |
|---|---|
| Volume under test | `PHerc0009B/volumes/20250521125136-8.640um-1.2m-116keV-masked.zarr` |
| Mesh for it | `PHerc0009B/segments/20250510172639/mesh/20250510172639-on-20250521125136-8.64um.tifxyz` |
| Control volume | `PHerc0172/volumes/20241024131838-7.910um-53keV-masked.zarr` |
| Control mesh | `PHerc0172/segments/20250917143559-w062…/mesh/20250917143559-on-20241024131838-7.91um.tifxyz` |

The PHerc0172 pair is the control required by `RESULTS.md` §5: it is the one
legacy-shaped volume the shipped reader already resolved, and the volume the
repository's only live-S3 test pins.

`--volume` names a local **cache directory**, not a Zarr store; chunks are fetched
from S3 and persisted there, so only what the segment touches is downloaded. Both
binaries share one cache directory, and each run writes to its own output paths so
no run can skip work by finding existing output.

The cache directory deliberately contains **no** `meta.json` or `metadata.json`.
That is the experiment: with no store document on the local filesystem, the
shipped reader resolves nothing (the defect) while the patched renderer falls back
to the volume it has already opened (the fix). Copying the store document into the
cache would make the *baseline* resolve the value too, and would compare nothing.

---

## 7. Measurements

*Filled in from `ci-out/comparison.txt` and `ci-out/tiffinfo.txt`.*

The three questions kept separate on purpose:

1. **Did both run and produce output?** exit codes and artefact lists.
2. **Did the declared physical scale change, and is the patched value right?**
   the `Voxel size` log line, every emitted `.zattrs`, TIFF `XResolution`.
3. **Are the rendered pixels unchanged?** a SHA-256 of the **decoded pixel
   arrays**, not of the files.

On (3): the patch deliberately changes the TIFF resolution tag, so the *file*
bytes differ even when every pixel is identical. Comparing file digests — or file
sizes, which is the mistake `RESULTS.md` §5 warns about — would report a
regression that is really the fix. `ci/compare_render_outputs.py` therefore
reports decoded-pixel digests as the verdict and file-byte digests only as
context, and `ci/selftest_compare.py` exercises that distinction against synthetic
fixtures so the reporter itself cannot silently invert the answer.

---

## 8. Error and edge cases

Tracked separately from the before/after experiment, because a metadata-correctness
fix must not be able to *invent* a physical scale. The intended behaviours, all of
which are logic-verified in the harness and are the subject of `RESULTS.md` §7.4's
follow-up:

| Case | Intended behaviour |
|---|---|
| No usable metadata anywhere | no physical scale written; warning on stderr; no silent `1.0` |
| `{"voxelsize": 0}` | treated as unusable, not as a measurement |
| `{"voxelsize": -3}` | treated as unusable, not as a measurement |
| Non-numeric / non-finite | resolves nothing |
| `--voxel-size` given explicitly | wins, and its unit is converted to µm |
| Unknown `--voxel-unit` | hard error, exit non-zero |
| Different-resolution volume | uses that volume's own value |
| Remote path | value comes from the opened volume, no extra request |

*Filled in from the run where the workflow exercises them; otherwise recorded as
still open.*

---

## 9. Problems still open

*Filled in from the run.*

Known and independent of the build:

* The GUI predicate (`SegmentationCommandHandler.cpp:2076-2078`) is deliberately
  **not** part of this patch, so the CLI fix stays unreachable from the route most
  users take. Documented as a separate follow-up per `RESUME.md` §6.
* No pull request has been opened against `ScrollPrize/villa` and nothing has been
  submitted for a prize.

---

## 10. Reproducing this

```bash
# locally: syntax/behaviour pre-flight for the workflow and its reporter
python ci/preflight_workflow.py     # YAML, bash -n per step, patch round-trip
python ci/selftest_compare.py       # reporter self-test on synthetic fixtures

# remotely: dispatch the workflow (needs Actions: write on the repository)
#   GitHub UI -> Actions -> "Renderer validation (baseline vs patched)" -> Run workflow
```

Artefacts retained for 30 days: `ci-out/` (logs, toolchain, patch diff, comparison
report) and `out/` (the rendered `.zarr` attributes and TIFFs). Sizes are bounded
by the 128×128 crop and the single-slice render.
