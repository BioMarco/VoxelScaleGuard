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

The path match is exact — `scan.voxelsize`, and only when `scan` carries no `tomo`
acquisition record, so the modern shape keeps its own path. There are tests for
four near-misses (metadata.scan.voxelsize, source.scan.voxelsize,
properties.voxelsize, scan.properties.voxelsize) which must all still resolve
nothing.

Tests: 4 new resolver cases plus 8 in my own harness, including negative controls
that fail if either the fall-through or the legacy schema is removed. I also
inverted my CI check that had asserted the old behaviour — it required the
multiscales block to be absent for an unknown size, which is the defect you found;
it now asserts the block is present, unitless, and correctly relatively scaled.

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
* the test counts are the ones actually run (`35/242` project, `17/87` control,
  `13/13` physical-size self-test).

## What the draft deliberately does not do

* It does not claim CI is green: **no GitHub Actions run had happened when this was
  written**, because CI triggers on push. The author should either push first and
  check the run, or say the push is imminent.
* It does not claim the renderer was re-run: this machine cannot build
  volume-cartographer, so no new render was produced, and none is needed (the
  renderer diff is comments only).
* It does not ask for a re-review timing or make any commitment about the prize.
* It does not thank the reviewer for anything beyond the review itself, and does
  not over-apologise: the point is the fix, not the contrition.
