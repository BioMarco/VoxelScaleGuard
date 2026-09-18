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

## `terminal-before-after.png`

The same run, printed. Header, baseline and patched render logs, the comparison
report and the summary section, laid out as a terminal frame.

**It is not an interactive session, and the image says so in its own header.** It is
GitHub Actions output: `ci/build_terminal_evidence.py` draws the lines it is given,
and every line it draws is checked verbatim against the run's own logs by
`ci/selftest_terminal_evidence.py` — 10 lines from each render log and 52 from
`comparison.txt`. The last section of the image is the generator's own labelled
summary, and it is labelled *as* a summary so it cannot be mistaken for log output.

```bash
python ci/build_terminal_evidence.py \
    --artifact <unzipped artifact dir> \
    --comparison <unzipped artifact dir>/ci-out/comparison.txt \
    --out DOCS/evidence/terminal-before-after.png
python ci/selftest_terminal_evidence.py \
    --artifact <unzipped artifact dir> \
    --comparison <unzipped artifact dir>/ci-out/comparison.txt
```

The self-test takes no `--figure`: it re-derives every line from the logs and checks
the *generator's* line list, rather than comparing images. The figure path is fixed
inside the generator, so there is nothing to pass. (An earlier revision of this file
documented a `--figure` argument that does not exist; corrected 2026-09-18 after the
command exited 2.)

## Provenance and licence of the data shown

Everything rendered above comes from Vesuvius Challenge Open Data bucket volumes
(`PHerc0009B`, `PHerc0172`). The published datasets are **CC BY-NC 4.0** unless
otherwise noted — see `scrollprize.org/docs/02_data.md`, and `DOCS/RESEARCH.md` §1.
The bucket metadata documents committed under `research/raw_metadata/` are verbatim
copies of those published documents.

So this directory contains **derived works of third-party, non-commercial-licensed
data**, and the attribution that licence requires — source, terms, and the fact that
the panels are rendered derivatives — needs to be stated in the repository's own
`NOTICE`/`LICENSE` material. It is currently recorded only in
`DOCS/LICENSING_PROPOSAL.md` §4, which flags it as a gap. This note is not a
substitute for that; it is the pointer to it.
