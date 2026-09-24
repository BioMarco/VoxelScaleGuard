# PR #1831 — reply to review

**Draft. NOT POSTED.** hendrikschilling reviewed the PR on 2026-09-23 and raised two
findings; both were correct and both are now fixed. The reply below is ready to
paste, but posting it is the author's decision.

---

## The draft reply

```text
Thanks for the review — both findings were right, and the first one was a real
defect in what I sent.

1. Unknown physical size removed the whole multiscales block.

You're right that this costs far more than the scale it was meant to fix: axes,
the ordered level list and every level's coordinateTransformations all live in
that block, and a reader that can't find it doesn't see a multiscale image at
all. I was fixing a fabricated `scale = 1.0` with a unit, and I deleted the
container to do it.

What it does now:

  - the multiscales block is always written;
  - with an unknown size, no axis carries a `unit` — nothing claims a physical
    measurement;
  - each level's `scale` is the relative factor between that level and level 0
    (level 0 = [1, 1, 1], Y/X = 2^level, Z = 1), i.e. the "scaling factor
    between the current resolution and the first resolution" that OME-NGFF 0.4
    asks for when a physical scale isn't available;
  - `multiscales[0].metadata.physical_size = "unknown"` says so in the document
    rather than leaving it to be inferred from a missing key.

The TIFF is unchanged: tifDpi stays 0, so no resolution tag.

I split the construction into a `buildMultiscales()` function so it's a pure
function of (baseVoxelSize, voxelUnit, sliceStep, pixelsPerVoxel) and can be
tested directly; the known-size branch is byte-for-byte the same arithmetic as
before. New tests in core/test/test_zarr.cpp cover the block being present, no
units, the relative scales per level, and the known-size path being unchanged.

2. Switching resolvers lost previously supported local metadata.

Also right, and I'd call it a regression I introduced. Two separate things:

  - `resolveLocalStoreVoxelSize()` returned the first existing file's answer
    including "nothing", so a meta.json holding only dimensions hid a usable
    metadata.json next to it. It now falls through to the next candidate when a
    file parses but yields no usable size, and also when a file won't parse.
  - the renderer's old reader had a `metadata.json -> scan.voxelsize` candidate
    that the shared resolver didn't cover. That's restored.

I did it in the shared resolver rather than in vc_render_tifxyz, since
Volume::NewFromUrl and vc_grow_seg_from_segments use the same resolver and a
renderer-only workaround would let them disagree about what a store says.

On units for scan.voxelsize, I didn't want to guess: the old readVolumeVoxelSize()
returned it with no conversion at all, and its caller treated every value it
returned as micrometres — the only input that ever got a unit conversion was an
explicit --voxel-size, behind the voxelSizeFromCli flag. The same reader produced
both meta.json's top-level `voxelsize` (micrometres) and this field, so it's
micrometres; and if it weren't, the legacy volume would have had a wrong TIFF
resolution on exactly the path the live-S3 test covers. I've written that
reasoning into the function so it's checkable rather than asserted.

The path match is exact — `scan.voxelsize`, nothing deeper. There are tests for five
near-misses (metadata.scan.voxelsize, source.scan.voxelsize, properties.voxelsize,
scan.properties.voxelsize, and `scan` as an array) which must all still resolve
nothing.

One thing worth stating explicitly, because I got it wrong on the first pass and
your "preserve previously supported local metadata" is what made me check: the old
reader had three candidates and no acquisition-record branch at all, so a document
carrying both `scan.voxelsize` and `scan.tomo...samplePixelSize` returned
`scan.voxelsize`. My first version added a `tomo` guard that let the newer field win
instead. That's a reinterpretation, not a preservation. The guard is gone and the
order is now top-level `voxelsize` > `scan.voxelsize` > acquisition record >
`source`, which is the historical order for every shape the old reader could read.
The regression test uses 7.91 for the legacy field and 8.64 for the acquisition
record so it can't pass by coincidence.

Tests: 5 new resolver cases plus 10 in my own harness, including negative controls
that fail if the fall-through, the legacy schema, or the precedence is changed back.
I also inverted my CI check that had asserted the old behaviour — it required the
multiscales block to be absent for an unknown size, which is the defect you found;
it now asserts the block is present, unitless, and correctly relatively scaled. That
check passes in the current CI run against the real patched binary.

One thing I should flag rather than leave you to find: that check passing was
itself the reason this got through. Every assertion I had asked "does anything
claim a physical size it shouldn't?" — none asked "is the structure still
complete?". Thanks for asking the second question.
```

---

## What was checked before writing this

Every claim in the draft maps to something executed; `RESULTS.md` §16.3 has the
commands and outcomes. Specifically:

* the `multiscales` descriptor quoted above is the output of the real
  `buildMultiscales()`, run locally;
* the relative factors match `createPyramidDatasets()`, which halves only Y/X;
* the OME-NGFF 0.4 sentence is quoted from the published spec, not paraphrased;
* the unit reasoning cites `vc_render_tifxyz.cpp:1397-1412` at `757f70c`;
* the test counts are the ones actually run (`37/249` project, `18/90` control,
  `13/13` physical-size self-test);
* the resolver precedence claim is the historical one, because the reply says the
  legacy field is preserved "exactly" — see the precedence section below.

## What CI has and has not established

**What has run and passed:** [run 36028271482](https://github.com/BioMarco/VoxelScaleGuard/actions/runs/36028271482)
completed **successfully**, 25/25 steps, none failed. That run:

* built **both** the baseline and the patched renderer from the same pinned
  revision, applied `patch/vc_render_tifxyz.patch`, and confirmed the applied diff
  is byte-identical to the committed artefact (`PATCH_FILES=3`,
  `PATCH_IDENTICAL=yes`);
* rendered `PHerc0009B` and `PHerc0172` with identical arguments, resolving
  **8.64 µm** and **7.91 µm** respectively;
* reported `DECODED PIXELS IDENTICAL: YES` for both volumes;
* drove the **unknown-size branch through the real patched CLI**, printing the
  `multiscales` structure — axes `['z','y','x']`, all three units `None`, six
  levels, relative scales `[1, 2^l, 2^l]`;
* reported `physical-size failures: 0` across the nm/µm/mm/m unit cases.

**What has *not* run:** the VoxelScaleGuard workflow builds `villa` at the *pinned*
commit and applies `patch/`. It does **not** build the 7-file PR branch itself, and
it does not compile the resolver at all — the resolver is part of the PR, not of the
patch. So:

* the **patch's** behaviour is CI-verified end to end (run `36028271482`, on
  `c8ba551`);
* the **resolver** changes, including the precedence correction, are verified
  locally: the fork's own unit suite compiled and run here (18 cases / 90
  assertions), the harness suites (37 / 249 and 18 / 90), and negative controls that
  fail when the fall-through, the legacy schema, or the precedence is reverted.

That distinction is stated in the reply rather than blurred.

## Resolver precedence: corrected after a first attempt got it wrong

The reply claims the legacy field is preserved exactly, so the code has to earn
that. An initial implementation added a `tomo` guard and put the acquisition record
ahead of `scan.voxelsize`. That was wrong. The pre-patch reader was three
`tryFile` candidates with **no acquisition-record branch at all**, so a document
carrying both `scan.voxelsize` and `scan.tomo...samplePixelSize` resolved to
`scan.voxelsize` historically. Both the guard and the ordering were removed;
effective precedence is now

```
top-level `voxelsize`  >  `scan.voxelsize`  >  acquisition record  >  source
```

A regression test asserts `7.91` (legacy field) against `8.64` (acquisition record)
— distinct numbers, so it cannot pass by coincidence — and one earlier test that
asserted the opposite was replaced.

## What the draft deliberately does not do

* It does not claim a full build of the 7-file branch, because there has not been
  one. The CI result above is about the patch applied to the pinned revision, and
  the reply says so.
* It does not claim the renderer produces different pixels, or that any render was
  re-run: the renderer diff is comments only, and neither finding can reach the two
  evidence volumes.
* It does not ask for a re-review timing or make any commitment about the prize.
* It does not thank the reviewer for anything beyond the review itself, and does
  not over-apologise: the point is the fix, not the contrition.
