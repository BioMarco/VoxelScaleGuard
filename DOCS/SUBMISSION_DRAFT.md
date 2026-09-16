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
2. **A contained fix** — one file, +176/−63 — that routes resolution through an
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
* The fixed decision procedure behaves as specified, across 19 cases.
* The patch applies to the pinned revision exactly (`git apply --check --reverse`
  succeeds on the patched tree).
* **The patched binary compiles and runs.** Both the baseline and the patched
  renderer were built in CI from the pinned revision and run against real published
  volumes: the declared physical scale becomes `micrometer` / `[8.64, 8.64, 8.64]`
  where the shipped binary declared `nanometer` / `[1, 1, 1]`, the TIFF gains
  `XResolution = 2939.8147` px/inch, and the decoded pixels are **byte-identical**.
  See `CI_VALIDATION.md`.
* Unusable inputs are refused rather than guessed: `--voxel-size 0`, `-3`, `nan`,
  and an unknown `--voxel-unit` each exit non-zero and write no physical scale.

**Honest gaps, stated rather than omitted**

* **The patch as first committed did not compile.** The new resolution block used
  variables declared ~60 lines below it. This is recorded here because it is the
  most instructive result in the project: the change was small, reviewed, and
  "logic-verified", and it still could not build. It is fixed, and the harness now
  tests for that class of defect — but a reader should know the first compile is
  what found it, not a human.
* **The GUI path stays broken** without the separate `SegmentationCommandHandler`
  change, so the fix is not yet reachable from the route most users take.
* **Coverage is two volumes and one crop each**, one slice. That is enough to
  demonstrate the correction and the absence of a pixel regression; it is not a
  survey of the catalog.
* **The "no usable voxel size anywhere" branch** — warning emitted, no physical
  scale written — is covered by unit tests, not end to end. Driving it would mean
  severing a volume from its own metadata; with a real volume the patched binary
  finds the value, which is the fix working.

This is now a working, binary-verified fix for the renderer, with the GUI
follow-up and broader coverage outstanding.

## Suggested next steps, in order

1. ~~Authorise a build of the patched `vc_render_tifxyz`.~~ **Done 2026-09-16** on
   GitHub-hosted runners; both binaries built from the pinned revision. The local
   machine remains unable to build it (`RESULTS.md` §8.4), which is why the
   workflow exists.
2. ~~Render a small segment against PHerc0009B and check `.zattrs` and the TIFF
   `XResolution`.~~ **Done**: `micrometer` / `[8.64, 8.64, 8.64]` and
   `XResolution = 2939.8147`. See `CI_VALIDATION.md` §7.
3. ~~Confirm the pixels are unchanged on a legacy volume (`PHerc0172`).~~ **Done**:
   decoded-pixel hashes identical on both volumes.
4. Optionally add a rendered image pair to the assets below, for the rubric's
   "before/after screenshots" — the tags and logs are already captured.
5. Take the VC3D predicate change (`SegmentationCommandHandler.cpp:2076-2078`) as a
   reviewed follow-up. It is the remaining gap for real users.
6. **Decide whether to open the PR.** Not done; see the note at the top of this
   file.

## Attribution

The "consult the volume's remote voxel size" approach is **NicolasHuberty**'s from
PR #1417, which lapsed to an inactivity bot rather than being rejected. The
`samplePixelSize` schema handling and `resolveLocalStoreVoxelSize` are
**Bullo27**'s and the villa maintainers'. The VC3D enable-predicate diagnosis is
**Bullo27**'s from PR #1228. This work consolidates those into one path and
verifies what the earlier attempts left open.

## Assets to attach

* `CI_VALIDATION.md` — the build and the before/after run, with the run URL, the
  exact revision, the build configuration and the raw comparison report.
* `RESULTS.md` §2 — the before/after transcript on four real volumes' metadata.
* `RESULTS.md` §9 — including §9.1, that the patch did not compile as first
  committed and why that is worth reporting.
* `harness/` — the reproducer, buildable in minutes with a local MSVC toolchain.
* `patch/vc_render_tifxyz.patch` — the fix, one file, reviewable in a sitting.
* *(optional)* rendered before/after images; the `.zattrs` and TIFF tag dumps are
  already captured in `CI_VALIDATION.md` §7.
