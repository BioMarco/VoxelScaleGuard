# SUBMISSION_DRAFT

**Not submitted. No form has been filled in, and none will be without the author's
authorisation.**

Text for a Vesuvius Challenge **Progress Prize** entry. Written to be readable by
someone who knows the scrolls but not this codebase. Every claim is either measured
in a linked run or marked as a limit.

---

## Title

```
A render declares the wrong physical voxel size, silently — fixed and verified on
two public volumes
```

## Short description (for a form's summary field)

`vc_render_tifxyz`, the tool that turns a surface mesh into a flattened image of a
scroll, attaches a physical size to everything it produces: the units and scale in
the OME-Zarr metadata, and the resolution tag in the TIFF. On the published catalog
it read that number from the wrong place, so for most volumes it declared **1
nanometre per voxel** where the store says 8.64 µm — the physical measurements were
wrong by ×8640, ×2400 or ×45532, while the images themselves were correct. This
contribution corrects it and demonstrates before/after on two real public volumes,
with the rendered pixels verified identical so that the change affects only what was
wrong.

---

## Answers to the submission form's four questions

The Progress Prizes form's single long field asks four numbered questions
(`PRIZE_REQUIREMENTS.md` §2.4, field 6). This is the prepared answer, in the form's
own order. It is written to be pasted and is self-contained: it does not assume the
reviewer has opened anything yet.

> **1. Which scroll data did you work on for this submission?**
>
> Published Vesuvius Challenge Open Data volumes from the public catalog, accessed
> anonymously and read-only: `PHerc0009B` (8.640 µm, 2025-05-21 scan) as the volume
> under test, and `PHerc0172` (7.910 µm, 2024-10-24 scan) as the control, because it
> is the one legacy-shaped catalog entry that the buggy code path did handle. Two
> further volumes, `PHercParis4` at 45.532 µm and 2.400 µm, were probed against their
> own published metadata documents but not rendered. The metadata documents I used
> are archived with hashes in `research/raw_metadata/`.
>
> **2. How does it substantially increase the probability of yourself or someone else
> reading those scrolls or others?**
>
> It removes a silent, several-thousand-fold error from the physical scale that
> `vc_render_tifxyz` attaches to every image it produces on the documented path from
> a CT scan to a readable surface. Nothing downstream of a render can currently trust
> a distance measured in it: a scale bar, a pixel-to-micrometre conversion, a check
> that a mesh matches the volume it was grown on, or a resolution-dependent
> comparison between two scans of the same scroll are all wrong by ×2400 to ×45532,
> with no error and no missing field to notice. This is a prerequisite for anyone
> doing measurement-driven work on these images, so that measurements taken from a
> render agree with the volume they came from.
>
> **3. What does it enable that was not possible before?**
>
> (a) Renders whose declared physical size is correct, so measurements taken from
> them mean something, and so the OME-Zarr axis metadata and the TIFF resolution tag
> agree with each other instead of describing two different voxels. (b) Trustworthy
> scale bars and cross-scan geometric comparison, which is what surface-fitting and
> registration work needs. (c) A defined behaviour when no voxel size can be found
> anywhere: the metadata block is now omitted rather than filled with a fabricated
> `1.0`, so "unknown" is representable instead of indistinguishable from "1
> nanometre". (d) For the community, a regression test and a reproducible CI check,
> so this cannot silently come back.
>
> **4. What evidence have you provided for this?**
>
> A public GitHub Actions run that builds the baseline and the patched renderer from
> the same commit in the same configuration, applies the patch with `git apply` and
> verifies the applied diff is byte-identical to the committed patch, then renders
> with identical arguments on the two volumes above. Evidence attached to this
> submission: a before/after figure showing the console output, the `.zattrs` scale
> and unit, the TIFF resolution tag and a **blank** pixel-difference panel — the
> decoded pixels are verified identical, digests compared, so the change provably
> touches only what was wrong; a second figure carrying the run's log lines verbatim;
> the run's own comparison report; a 27-case unit suite (208 assertions) including
> tests that fail if either the original defect or a ×1000 unit regression is
> reintroduced; and the full record of what was and was not verified, including two
> earlier attempts of my own that were wrong. All links are in the repository below.

---

## The full text

### What this is

A fix for a measurement bug in `vc_render_tifxyz`, one of the tools on the documented
path from a CT scan to a readable surface. It is proposed upstream as a pull request
and verified end to end on real published data. The work is a correction and a
verification, not a new method.

### The problem

When `vc_render_tifxyz` renders a surface it also records **how big a voxel is**, in
two places: the axis metadata of the OME-Zarr output (`.zattrs`), which states both a
scale and a unit, and the resolution tag of the TIFF. Anything downstream that wants
a real-world distance — a scale bar, a pixel-to-micrometre conversion, a check that a
mesh matches the volume it was grown on — reads those numbers.

The renderer looked for the voxel size in a **local file** next to the volume, using
a reader that recognised a single field name, and it did so *before* opening the
volume. Almost none of the published catalog stores its resolution that way; they
record it in a scan-acquisition record instead. So the lookup failed, and the render
fell back to a placeholder of `1.0` — declared in **nanometres**, because that is
`--voxel-unit`'s default, even though the number is a micrometre quantity.

The result, on four volumes probed against their own published metadata:

| Published volume | What the store says | What the render declared | Error |
|---|---|---|---|
| `PHerc0009B/…-8.640um-…` | 8.64 µm | 1 nm | **×8640** |
| `PHercParis4/…-45.532um-…` | 45.532 µm | 1 nm | **×45532** |
| `PHercParis4/…-2.400um-…` | 2.4 µm | 1 nm | **×2400** |
| `PHerc0172/…-7.910um-…` | 7.91 µm | 7.91 nm | ×1000 (unit only) |

The fourth row is why this survived: it is the one catalog shape the old reader
handled, and it is the volume the repository's only live-S3 test pins. Note that its
number was right and its **unit** was wrong — so even the working case was ×1000 off.

### Why a wrong declared size matters

The pixels are not the problem; every physical statement about them is.

* **Any measurement taken from a render is wrong by a factor of thousands.** A
  distance measured in the flattened image, or a scale bar drawn on it, is wrong by
  the same factor.
* **It fails silently and in the confident direction.** The output declares
  `nanometer` and a number; nothing is missing, nothing errors, and a consumer that
  trusts the metadata cannot tell it is wrong. A missing unit can be noticed; a wrong
  one cannot.
* **It is inconsistent between output formats and between volumes.** The same tool
  writes a self-consistent pair on one volume and a mismatched one on another, so any
  downstream comparison across volumes inherits the error.
* **It propagates.** A flattened image whose declared scale is wrong is a bad input
  to anything that resamples, tiles, registers or composites it, and to any
  mesh/volume frame check.

This is a measurement-plumbing bug. It does not make ink detection better or worse;
it makes the numbers attached to the data trustworthy or not.

### How the fix works

**Take the size from the volume that is already open.** By the point the renderer
needs the number it has already opened the source volume, and opening a volume
already fetches and normalises that same store document through the shared resolver
the rest of the codebase uses (`vc::metadata::resolveLocalStoreVoxelSize`,
`Volume::voxelSize()`). So the fix resolves the size *after* the volume is open and
reads it from there, falling back to the shared local resolver for runs with no
volume open, and to the command line if the user supplied one.

Three consequences, and they are the reason for this shape rather than another
metadata fetch:

* **no extra network request** — the document has already been fetched;
* **it cannot disagree with the volume being rendered**, because it is the same
  object the renderer is streaming from;
* an earlier attempt (PR #1417) re-fetched the document and had to defend against a
  URL-fragment hazard in the path builder; reading the open volume constructs no URL
  at all, so that hazard is unreachable by construction rather than guarded.

The patch also fixes a second, independent defect it exposed: the OME-Zarr writer
took the scale and the unit as two separate arguments and never checked that they
agreed, so a converted micrometre number could be written under a caller's
"nanometer" label — another silent ×1000. The number and its unit are now computed
together, and when no size is available at all, **nothing is declared**: the writer
omits the scale block rather than publishing a placeholder as if it were measured.

### Results: before and after, on two public volumes

Both binaries were built from the same commit in the same configuration and run with
identical arguments on public Open Data. Run:
<https://github.com/BioMarco/VoxelScaleGuard/actions/runs/35249590299>

**`PHerc0009B` — 8.64 µm, the volume under test**

| | baseline (`main`) | patched |
|---|---|---|
| console | `Voxel size: 1.0 (no metadata found…)` | `Voxel size (remote volume metadata): 8.64 micrometer` |
| `.zattrs` unit | `nanometer` | `micrometer` |
| `.zattrs` scale | `[1, 1, 1]` | `[8.64, 8.64, 8.64]` |
| TIFF `XResolution` | *absent* | `2939.8147` px/inch |
| **decoded pixels** | — | **identical** |

**`PHerc0172` — 7.91 µm, the control** (the legacy-shaped volume the old reader did
handle): `.zattrs` goes from `nanometer`/`[1,1,1]` to `micrometer`/`[7.91,…]`, the
TIFF gains `3211.1252` px/inch, and again the decoded pixels are identical.

**Metadata and TIFF now agree.** `25400 / 8.64 = 2939.8148…` and
`25400 / 7.91 = 3211.1252…`, so the resolution tag and the OME-Zarr scale describe the
same physical size rather than two different ones. That agreement is checked
directly: the CI converts both outputs to micrometres and compares each against the
size the command line asked for, across nanometre, micrometre, millimetre and metre
spellings.

**The pixels are identical, and that is the claim.** A difference panel in the
evidence figure is blank, and the figure says so in the panel. File sizes do differ
(14244 → 14296 bytes) because the resolution tag *is* the fix; that is exactly why
the regression check hashes the **decoded pixel array** and not the file — comparing
files would have reported this fix as a regression. The figure:
<https://github.com/BioMarco/VoxelScaleGuard/blob/ci/renderer-validation/DOCS/evidence/before-after.png>

### Advantages over the earlier attempts, and credit for them

Three earlier attempts at parts of this exist and all lapsed to process rather than
to rejection, and their authors did the substantive thinking:

* **NicolasHuberty, PR #1417** — having the renderer consult the volume's remote
  voxel size. That is the approach here. It was closed by an inactivity bot, not
  rejected.
* **Bullo27 and the villa maintainers, PRs #1227 / #1229 / #1454** — the scan-record
  schema handling, and the shared resolver this fix reuses.
* **Bullo27, PR #1228** — the diagnosis of the GUI-side enable predicate.
* **DarthCeltic (#1403)** and **Bullo27 (#1226)** — the issue reports.

What is new here is the verification and the correction: the defect reproduced on real
catalog data, a patch that compiles and runs, before/after on two public volumes with
pixel equality, and a test suite. Concretely, compared with #1417's design, this one
needs no second fetch and so cannot disagree with the volume being streamed, and it
makes the fragment hazard raised in that PR's review unreachable rather than guarded.
The `vc_grow_seg_from_seed` half of #1403 is **already fixed upstream** and is not
claimed here.

### How to reproduce

Everything is public and needs no paid service.

* Evidence repository: <https://github.com/BioMarco/VoxelScaleGuard>
* The patch: `patch/vc_render_tifxyz.patch` (3 files, +250/−65), applies to
  `villa` `main`.
* The workflow that builds both binaries from one commit, applies the patch by
  `git apply`, checks the applied diff is byte-identical to the committed patch, and
  runs both on public volumes with identical arguments:
  `.github/workflows/renderer-validation.yml`, dispatched from the Actions tab. It
  installs dependencies from the public apt list on a free runner; no private
  registry, and no credentials beyond the automatic token.
* The harness, buildable in minutes with a local MSVC toolchain, which compiles the
  pinned revision's real translation units and runs the test suite, with upstream's
  own suite unmodified as a control.

### Limits, stated plainly

* **Two volumes, one crop, one slice each, one build configuration.** This
  demonstrates the correction and the absence of a pixel regression. It is not a
  survey, and no claim is made about volumes not rendered here.
* **The GUI path is not fixed by this patch.** VC3D suppresses `--voxel-size` for a
  native-resolution remote volume, so the CLI fix is not yet reachable from the route
  most users take. That is deliberately left as a separate change because it alters
  GUI behaviour.
* **No claim about ink detection.** This fix corrects declared physical metadata. It
  does not improve, and is not claimed to improve, the readability of any scroll or
  the output of any ink model.
* **The patch is not merged.** It is prepared as a pull request and has not been
  opened, let alone accepted. No claim is made that anyone is using it.
* **The development history contains two real defects** that only execution found:
  the first version of the patch did not compile, and the second wrote a wrong unit
  on one command-line path. Both are recorded, with the tests that now pin them.
* **The patched build was produced by CI, not on the author's machine**, which cannot
  build the application at all. The workflow is the reproduction path.

### Assets

* before/after figure (metadata, both rendered images, empty difference panel):
  `DOCS/evidence/before-after.png`
* terminal evidence, copied verbatim from the run's logs:
  `DOCS/evidence/terminal-before-after.png`
* the run's full record: `DOCS/CI_VALIDATION.md`
* everything executed, and everything not: `DOCS/RESULTS.md`
* the file-by-file cause: `DOCS/ROOT_CAUSE_ANALYSIS.md`

### Licence note for the submission

`villa`'s root is MIT; the `volume-cartographer` subproject that this patch modifies
is licensed **GPL-3.0-or-later** (Copyright (C) 2023 EduceLab — see
`volume-cartographer/NOTICE` and its `LICENSE`). So the patch itself is a derivative
of GPL-3.0-or-later code and cannot be relicensed MIT; this repository's own work —
the investigation, harness, CI, evidence and documentation — can be licensed
permissively without difficulty.

The form's Terms say *"you have to make it open source under a permissive license to
accept the prize"*, and the Grand Prize section says *"open source license (e.g.
MIT)"*. Note that this is a condition of **accepting a prize**, not of submitting.
Whether a contribution to a GPL-3.0-or-later subproject satisfies the "permissive"
wording is a question for the organisers, raised here rather than assumed. Full
inventory, including the third-party Open Data attribution that still needs adding:
`LICENSING_PROPOSAL.md`.
