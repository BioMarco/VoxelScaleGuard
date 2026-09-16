# SUBMISSION_DRAFT

**Not submitted. Nothing has been published and no form has been filled in.**

Written early on purpose: the Progress Prize guidance favours contributions that
are *"released or open-sourced early"*. This draft is deliberately honest about
what is not yet done, because the review team will try to reproduce the work and a
submission that oversells will fail on inspection.

---

## Category

**Progress Prize.** Not the 2027 Grand Prize, First Letters, or the PHerc. Paris 4
Title Prize — this is a tooling correctness contribution, not a reading of a
scroll.

## Deadline

**Next deadline re-checked [live] 2026-09-16** against <https://scrollprize.org/prizes>:
*"The next deadline is 11:59pm Pacific, September 30th, 2026."* That is **14 days
away**, not past. An earlier revision of this draft said the date "has passed at the
time of writing"; that was a documentation error and is corrected here (see
`RESULTS.md` §8). Submissions are evaluated monthly, so a missed month costs a
month rather than the contribution — but the immediate deadline is live.

---

## Title

```
Silently wrong physical scale in vc_render_tifxyz: ×2400 to ×45532 errors on real Open Data volumes
```

## Short summary

`vc_render_tifxyz` attaches a physical scale to everything it renders. On current
`villa` it obtains the voxel size from a local file, using a reader that recognises
only a top-level `voxelsize` key — even though the process has already fetched the
correct value over the network and is holding it. For most of the published
catalog the fallback is a scale of `1.0` declared as **nanometres**, which makes
the declared physical voxel size wrong by **×2400, ×8640 or ×45532**, depending on
the volume. The rendered image is correct; every physical measurement attached to
it is not.

Measured against four real Open Data volumes, the shipped reader resolves **one**.
That one is the legacy-shaped volume which the repository's only live-S3 test
happens to pin — which is why the defect has survived.

## The contribution

1. **A verified diagnosis**, at file-and-line resolution, of five interacting
   defects, including the finding that the error is the *product* of two
   independent bugs (an unresolved number, and a unit that does not match it).
2. **A contained fix** — one file, +177/−65 — that routes resolution through an
   abstraction `core` already provides and that other tools already use, and that
   resolves the value *after* the volume is open so the already-fetched remote
   value is reused. No public interface change, no new dependency, no new
   metadata convention, and no change to the rendered pixels.
3. **Evidence on real data**: the before/after over published volumes, with a
   deliberate control (the one volume that already worked), plus 16 test cases and
   upstream's own suite passing unmodified as the sanity check.
4. **Reproduction of two review concerns** from the earlier lapsed attempt at this
   fix (PR #1417), as executable tests rather than assertions — one confirmed and
   shown reachable, one confirmed and shown worse than described.

## Rationale against the published criteria

> *"Resolve outstanding bugs in tools that people are using, and that you are using
> yourself, evidenced by before/after screenshots, logs, etc."*

Before/after logs are in `RESULTS.md` §2, produced by running the pinned
revision's own code against live catalog metadata. The tool is on the documented
path from CT scan to readable text, and its output scale is what a scale bar — a
Grand Prize requirement — is drawn from.

> *"Improve results quantitatively and/or qualitatively on real data."*

Quantitative: the declared physical voxel size moves from a wrong value to the
correct one on 3 of 4 real volumes probed, with the 4th as a control. The
correctness of the recovered value is independently corroborated by the volume
names themselves (`…-8.640um-…` ↔ `samplePixelSize 0.00864 mm`), which does not
depend on trusting the code.

> *"Are well documented."*

Nine documents plus a runnable harness, including the parts that did **not** work
out.

> *"Demonstrate significant advantages over existing solutions."*

Three prior attempts at parts of this exist and all lapsed unfixed (#1417, #1228,
#1313); a fourth (#1541) lapsed too. The concrete advantage over #1417 is a
comparison, not a claim: it re-fetched the metadata, so it could disagree with the
volume being streamed and had to defend against a URL-fragment hazard its own
review raised. Reading `Volume::voxelSize()` from the volume already open removes
the need for that defence and costs zero extra requests. Full table in
`FEASIBILITY.md` §7.

> *"Accept standard community formats… Maintain consistent output formats…
> Designed for modular integration."*

The fix concerns OME-Zarr `.zattrs` and TIFF resolution tags. No format changes,
no interface changes, and it reuses `vc::metadata::resolveLocalStoreVoxelSize` from
`core`, which is the shared resolver the rest of the tree already agrees on.

## What is verified, and what is not

Stated plainly, because this is what the review team will check first.

**Verified**

* The defect is present at `main` @ `757f70c0140a4cfbbbd44975ef09558444b96980`.
* The pre-patch reader resolves 1 of 4 real published volumes; the failure and the
  resulting ×8640/×45532/×2400 declared-scale errors are demonstrated by execution.
* The pre-patch reader returns `0.0` and `-3.0` as if they were measurements.
* The URL-fragment hazard from #1417's review is real in `joinRemoteUrlPath`, and
  reachable via the GUI's `remoteVolumeLocator()` for rebased volumes.
* The fixed decision procedure behaves as specified, across 16 cases.
* The patch applies to the pinned revision exactly (`git apply --check --reverse`
  succeeds on the patched tree).

**Not verified — the honest gaps**

* **The patched binary has never been compiled or run.** Two independent causes,
  both measured on 2026-09-16 and recorded in `RESULTS.md` §8.4: the compile-time
  closure (OpenCV, libtiff, Boost `program_options`, curl, blosc, zstd, lz4,
  `vc_delta3d`) is not installed, and neither CMake nor Ninja can execute a
  compiler in this environment — CMake's child-process probe of Ninja returns
  `Accesso negato`, and Ninja hangs when a build rule spawns a process. A newer
  CMake (upstream needs ≥ 3.28; 3.24 is present) would close only the version half.
  The patch is **logic-verified, not binary-verified**.
* **No render was produced**, so there is no `.zattrs` file, no TIFF tag dump, and
  no before/after image pair. The physical-scale consequence is derived from
  reading `writeZarrAttrs` and `Tiff.cpp`, plus the live documents.
* **The GUI path stays broken** without the separate
  `SegmentationCommandHandler` change, so the fix is not yet reachable from the
  route most users take.
* **The `before` transcript is available but has not been captured.** A prebuilt
  Windows package built from exactly the pinned commit
  (`VC3D-757f70c-2026-09-15-win64.zip`, 148.4 MB, `latest` release) was
  re-confirmed to exist [live] — but it has not been downloaded, at the standing
  instruction to obtain authorisation first. It yields the *before* side only; the
  *after* side still requires a build.

Because of the first two, **this is not ready to submit as a working fix.** It is
a verified diagnosis with a logic-verified patch, and the remaining work is
enumerated with what it needs.

## Suggested next steps, in order

1. **Capture the `before` transcript** from the prebuilt Windows package built from
   the pinned commit (148.4 MB, `latest` release; existence re-confirmed [live]
   2026-09-16). This needs no build and no authorisation beyond the download. It
   answers a question currently only [read]: that the `vc_*` CLI tools really do
   ship in the bundle, and what the shipped renderer actually prints.
2. Authorise a build of the patched `vc_render_tifxyz`. On the current machine this
   is not a matter of choosing a preset: `RESULTS.md` §8.4 records that neither
   CMake nor Ninja can execute a compiler here, and that the dependency closure is
   absent. A build therefore needs a different environment — a machine with a
   working CMake ≥ 3.28 + Ninja + Docker/WSL, or a full `cl.exe` manifest — and the
   decision is the user's, not the agent's.
3. Render a small segment against
   `PHerc0009B/volumes/20250521125136-8.640um-1.2m-116keV-masked.zarr` and check
   `.zattrs` (`unit: micrometer`, `scale: [1, 8.64, 8.64]` at `-g 0 --scale 1`) and
   the TIFF `XResolution` (≈2939.8 px/inch). Capture the before/after logs and the
   images the rubric asks for.
4. Confirm the pixels are unchanged on a legacy volume (`PHerc0172`) as the
   regression check.
5. Take the VC3D predicate change as a reviewed follow-up.
6. Open the PR, then submit.

## Attribution

The "consult the volume's remote voxel size" approach is **NicolasHuberty**'s from
PR #1417, which lapsed to an inactivity bot rather than being rejected. The
`samplePixelSize` schema handling and `resolveLocalStoreVoxelSize` are
**Bullo27**'s and the villa maintainers'. The VC3D enable-predicate diagnosis is
**Bullo27**'s from PR #1228. This work consolidates those into one path and
verifies what the earlier attempts left open.

## Assets to attach

* `RESULTS.md` §2 — the before/after transcript on four real volumes.
* `harness/` — the reproducer, buildable in minutes with a local MSVC toolchain.
* `patch/vc_render_tifxyz.patch` — the fix, one file, reviewable in a sitting.
* *(pending)* rendered `.zattrs`, TIFF tag dump, and before/after images.
