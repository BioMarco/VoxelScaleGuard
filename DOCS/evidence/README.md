# evidence/

Deliverable figures. Each one is built from artefacts of a real run, and every value
in it is traceable to a file or a log rather than to a description.

## `before-after.png`

Before/after for the voxel-size fix, from
[run 35249590299](https://github.com/BioMarco/VoxelScaleGuard/actions/runs/35249590299).

**What is in it.** On `PHerc0009B` (the volume under test) and `PHerc0172` (the
control), with identical arguments:

| | baseline (`main`) | patched |
|---|---|---|
| `.zattrs` unit | `nanometer` | `micrometer` |
| `.zattrs` scale, level 0 | `[1, 1, 1]` | `[8.64, 8.64, 8.64]` |
| TIFF `XResolution` | absent | `2939.8147` px/inch |
| decoded pixels | — | **identical** |

The two raster panels are the actual `00.tif` from the run. The difference panel is
blank, and the figure says so in the panel rather than leaving it empty: the point is
that the **rendered pixels are unchanged** and only the declared physical scale
moves. File sizes differ (14244 → 14296) because the resolution tag *is* the fix,
which is why the regression check is decoded pixels and not file size or file hash.

**Where the values come from.**

* TIFF dimensions, mode, `XResolution`/`YResolution`/`ResolutionUnit` and the decoded
  pixels: read from the real `00.tif` files with Pillow.
* `.zattrs` unit and scale: the `.zattrs` files are **not** in the uploaded artifact,
  because `actions/upload-artifact` skips hidden files by default, so these values
  are quoted verbatim from the run's own comparison report, which read them from
  those files.
* Both binaries were built from the same commit in the same configuration, and the
  applied diff was checked byte-identical to the committed patch before the second
  build.

**Regenerating it.** `ci/build_evidence_figure.py` reproduces the figure, including
its annotations:

```bash
# 1. get the run's artifacts (they are small: logs, .zattrs-derived report, TIFFs)
#    Actions -> the run -> Artifacts -> renderer-validation, unzip it
# 2. regenerate
python ci/build_evidence_figure.py \
    --artifact <unzipped artifact dir> \
    --comparison <unzipped artifact dir>/ci-out/comparison.txt \
    --out DOCS/evidence/before-after.png \
    --run-url https://github.com/BioMarco/VoxelScaleGuard/actions/runs/35249590299
```

The committed PNG carries presentation annotations applied separately from the
generator, so a fresh run of the command above is **not byte-identical** to it — the
residue is font rasterisation, under 1% of pixels, and none of it falls inside the
raster panels. That is checked, not assumed:

```bash
python ci/selftest_evidence_figure.py \
    --artifact <unzipped artifact dir> \
    --comparison <unzipped artifact dir>/ci-out/comparison.txt \
    --figure DOCS/evidence/before-after.png
```

It asserts that both panels embedded in the committed figure are byte-identical to
panels rebuilt from the real TIFFs, that a fresh generation puts those same panels
in the same place, and that **no** differing pixel lies inside a raster panel — i.e.
that the difference between the two files is annotation, never data.
