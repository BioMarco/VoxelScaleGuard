# PR_DRAFT

**Branch pushed to the fork. The pull request is NOT open. Nothing has been
submitted anywhere.**

| | |
|---|---|
| Fork | **<https://github.com/BioMarco/villa>** (created 2026-09-18, public, forked from `ScrollPrize/villa`) |
| Branch | **`fix/render-voxel-size-from-open-volume`** |
| Commit | **`d419dece6af51e0e015f6dc1df92c0312be76075`** |
| Contents | **exactly 3 files**, +250/−65 — verified locally and via the GitHub API |
| Based on | `ScrollPrize/villa` `main` @ `b1ef996e357de0b2f24fb30198c6d9c32611d4fb` (2026-09-18), 0 behind |
| Open the PR at | <https://github.com/BioMarco/villa/pull/new/fix/render-voxel-size-from-open-volume> |

**Two things you must do yourself before opening it** (§"Before you open the PR"):
write your own comment in the marked block, and tick the template's verification
checkbox only if it is true for you. I have deliberately left the checkbox
**unticked** and the comment block **empty**.

---

# PART 1 — the pull request body

Paste this into the PR. It follows villa's `.github/pull_request_template.md`: the
template's five italic prompts first, then a `## Details` section. The lines the
template asks you to fill in are answered; the one block that must be yours is
marked.

---

**In one sentence:** renders no longer declare a physically wrong voxel size — the
OME-Zarr `.zattrs` axis units and the TIFF resolution tags now describe the volume
that was actually rendered.

**One real example:** starting with the published volume
`PHerc0009B/volumes/20250521125136-8.640um-1.2m-116keV-masked.zarr` and a mesh
authored for it (`20250510172639-on-20250521125136-8.64um.tifxyz`),
`vc_render_tifxyz` built from `main` wrote `.zattrs` declaring the voxel size as `1`
in **nanometres**; the same command with this patch writes `8.64` in
**micrometres**.

<!-- Both binaries were built and run on a GitHub-hosted runner by the CI workflow
     in the evidence repository, with identical inputs; see "Evidence" below. Do not
     change this to a first-person claim unless you have run the comparison
     yourself. -->

**Before:** the render attaches a physically wrong scale to everything it produces.
On that volume the declared physical voxel size is wrong by **×8640**; on others it
was measured to be wrong by ×2400 and ×45532. The rendered pixels are fine — every
physical number attached to them is not, and the TIFF gets no resolution tag at all.

```
$ vc_render_tifxyz -v cache/ -s seg.tifxyz -g 0 --scale 1 -n 1 \
    --crop-x 3145 --crop-y 3412 --crop-width 128 --crop-height 128 \
    --zarr-output out.zarr --tif-output out.tif
Voxel size: 1.0 (no metadata found; override with --voxel-size)

out.zarr/.zattrs -> axes unit "nanometer", scale [1, 1, 1]      # 0.001 um/voxel
out.tif/00.tif   -> no XResolution tag at all
```

**After this PR:** the same command, same inputs, same settings:

```
Voxel size (remote volume metadata): 8.64 micrometer

out.zarr/.zattrs -> axes unit "micrometer", scale [8.64, 8.64, 8.64]
out.tif/00.tif   -> XResolution 2939.81 px/inch  ( == 25400 / 8.64 )
```

**Proof:** an evidence figure built from the real artifacts of
[this run](https://github.com/BioMarco/VoxelScaleGuard/actions/runs/35249590299) is
attached — it shows the declared metadata, the two rendered images side by side, and
a difference panel that is empty. The run builds `main` **and** `main + this
patch` from the same commit and runs both on two public volumes, `PHerc0009B`
(8.64 µm) and `PHerc0172` (7.91 µm). What to look at:

* the `.zattrs` unit and scale;
* the TIFF `XResolution`, read twice — once with Pillow, once with `tiffinfo`;
* the **decoded-pixel digests, which are identical** between the two binaries: the
  images are unchanged and only the physical metadata is corrected.

```
PHerc0009B  baseline  .zattrs nanometer  scale [1,1,1]      TIFF no resolution tag
            patched   .zattrs micrometer scale [8.64,...]   TIFF 2939.81 px/inch
PHerc0172   baseline  .zattrs nanometer  scale [1,1,1]      TIFF no resolution tag
            patched   .zattrs micrometer scale [7.91,...]   TIFF 3211.13 px/inch

decoded pixels, baseline vs patched, both volumes: identical
file bytes differ (14244 -> 14296) because the resolution tag is the fix
```

**Why / where this is useful:** anything that measures a render in physical units
reads these numbers — ink-detection input scaling, mesh/volume frame checks, scale
bars on published images (a Grand Prize submission requirement), and any downstream
tool that opens the `.zarr` and trusts its axes. Correcting them makes the output of
`vc_render_tifxyz` comparable with the volume it came from.

**Why I am sending this** — <!-- THIS BLOCK MUST BE WRITTEN BY YOU, IN YOUR OWN
WORDS. villa's CONTRIBUTING.md asks that any LLM-assisted PR carry human-written
commentary explaining why it is relevant. Suggested ground to cover, in your voice:
what you were doing when you hit this, why a wrong declared voxel size matters for
your work, and what you want the maintainers to look at. Do not describe it as a
tooling exercise; say what it cost you. -->

- [ ] I personally verified that the example and proof above were produced by this PR on the stated data.

<!-- Leave the box above UNTICKED until you have personally run the comparison.
     Tick it only if you have. -->

## Details

### The defect

`vc_render_tifxyz` wrote a physical voxel size into OME-Zarr `.zattrs` axis units
and per-axis scale, and into the TIFF resolution tag. It resolved that number
**before** opening the source volume, from a local file only, using a private
reader that recognised a single schema:

```cpp
if (auto v = tryFile(volPath / "meta.json", nullptr)) return v;
if (auto v = tryFile(volPath / "metadata.json", "scan")) return v;
if (auto v = tryFile(volPath / "metadata.json", nullptr)) return v;
```

It looked only for a top-level `voxelsize`. Most of the published catalog instead
records its resolution as `scan.tomo.acquisition.detector.samplePixelSize` (mm) in
`metadata.json`, so the reader resolved nothing and the render continued at a scale
of `1.0`, declared with `--voxel-unit`'s default of `nanometer`. The error is the
product of two independent mistakes: an unresolved number, and a unit that does not
describe it.

Measured against four published stores' own metadata documents, that reader
resolves **one** — the legacy-shaped `PHerc0172`, which is the volume
`core/test/test_volume_live_s3.cpp` happens to pin. That is why this survived.

### The cause, and why the value is taken from the open volume

The value was already available. A few lines later the same function has opened the
volume remotely via `Volume::NewFromUrl()`, and `Volume` construction has already
fetched and normalised that same store document:

* `Volume.cpp:1043` calls `vc::metadata::voxelSizeFromStoreMetadata(full)`;
* `Volume.cpp:1345` does the same for the remote path in
  `loadRemoteVolumeMetadata()`;
* `Volume::voxelSize()` (`Volume.cpp:1575`) returns that normalised value, `0` when
  unknown, in **micrometres**.

So this patch resolves the size **after** the volume is open and reads
`remoteVolume->voxelSize()`. Concretely, versus the earlier approach in #1417:

* **no extra network request** — the document has already been fetched;
* **it cannot disagree with the volume being rendered**, because it is the same
  object the renderer is streaming from;
* it reuses `vc::metadata::resolveLocalStoreVoxelSize()` for the local case, the
  shared resolver `vc_grow_seg_from_segments` already uses, so this tool stops
  having a third opinion about what a store's document says;
* the URL-fragment hazard raised in #1417's review — `joinRemoteUrlPath` does not
  strip a `#vc-base-scale=N` fragment — is **unreachable by construction**, because
  no URL is constructed here at all.

The ordering *is* the fix: resolving earlier is exactly why the private reader
existed.

### Behaviour of each source, and of the CLI

Priority, highest first: explicit `--voxel-size` → the open volume → the store's
local document, when no volume is open → unresolved.

| Source | Value used | Declared unit |
|---|---|---|
| `--voxel-size` (+ `--voxel-unit`) | converted to µm for the TIFF tag; the caller's own number for `.zattrs` | the caller's `--voxel-unit` |
| the open (streamed) volume | `Volume::voxelSize()` | `micrometer` |
| local `meta.json` / `metadata.json`, when no volume is open | the shared resolver's value | `micrometer` |
| nothing usable | **nothing declared**: no `multiscales` block, no TIFF resolution tag; warning on stderr | — |

The opened volume is consulted **before** the local document. `--volume` is often a
chunk cache for a remote source, and such cache directories carry the store's own
root metadata (`docs/remote_file_cache.md`), so a stale local mirror must not be
able to override the document `Volume` construction has just fetched. It is also the
streamed volume, not the cache directory, that is being rendered.

**CLI compatibility is preserved deliberately, including the meaning of
`--voxel-unit`.** In `main` that flag describes the number supplied on the command
line: the pre-patch code multiplied by `0.001` for `nanometer` *solely* to reach
micrometres for the TIFF tag, and passed the caller's raw number and unit straight
to `writeZarrAttrs`. This patch keeps that, so
`--voxel-size 8640 --voxel-unit nanometer` still means 8640 nm. The documented
invocation in the tutorial — `--voxel-size 9.362 --voxel-unit micrometer` — is
unaffected.

The only output-side change relative to `main` is which unit label accompanies a
**metadata-sourced** size: `micrometer` instead of whatever `--voxel-unit` said.
For the default `nanometer` that is precisely the ×1000 correction. If a caller
explicitly passed `--voxel-unit micrometer` (as the tutorial does), it is a no-op.

### `.zattrs` number and unit are now computed together

`writeZarrAttrs` writes `scale` and `axes[*].unit` from two separate arguments and
never checks that they agree, so a number and a unit can silently describe
different physical sizes. This patch computes them as a pair
(`zarrVoxelValue()` / `zarrVoxelUnit()`), with the invariant

```
zarrScaleValue(...) x micrometersPerUnit(zarrUnit(...)) == micrometerPerVoxel
```

asserted in the test suite for nm/µm/mm/m in both long and short spellings, at
level 0 and through the pyramid. This is not hypothetical: an earlier revision of
this patch wrote the converted micrometre number under the caller's unit label,
which declared 8.64 nm for `--voxel-size 8640 --voxel-unit nanometer`. It was
caught by CI checks that convert `.zattrs` and the TIFF tags to micrometres and
compare both against the requested size.

### The size is unknown: nothing is declared

`writeZarrAttrs()` wrote the per-axis `scale` unconditionally and only made the
axis `unit` conditional (`Zarr.cpp:374` vs `:391-394`), so a render with no usable
size still emitted `coordinateTransformations.scale = [1, 2, 4, …]`. A `scale` with
no unit is a physical measurement that was never made, and a reader cannot tell it
from a real one.

The patch makes a non-positive `baseVoxelSize` mean "unknown" and omits the
`multiscales` block entirely, and the renderer passes 0 in that case. The TIFF is
unchanged: `tifDpi` stays 0, which already means "do not set the resolution tags".
The stderr warning says the scale is unknown and that none will be declared, and it
names the unit a caller must supply (`--voxel-size <value> --voxel-unit
micrometer`) — the flag defaults to `nanometer`, so the previous advice would have
produced a self-consistent 1000× error.

### Scope of the change

The renderer: the private reader is replaced by the shared resolver plus the open
volume, the resolution moves to after the volume is opened, and the number/unit pair
is computed together. Plus the `writeZarrAttrs()` contract above, which is only
reachable when a size is genuinely unavailable. No new dependency, no interface
change, no change to the rendered pixels, no change to any option's meaning.

### Evidence, and how to reproduce it

* **Fork branch:** <https://github.com/BioMarco/villa/tree/fix/render-voxel-size-from-open-volume>
* **Evidence repository:** <https://github.com/BioMarco/VoxelScaleGuard> — the
  workflow, the reporter scripts, the run logs and the test harness.
* **Workflow:** `.github/workflows/renderer-validation.yml`. It clones
  `ScrollPrize/villa` at a chosen commit, builds `vc_render_tifxyz` twice from the
  same tree (once clean, once with the patch applied by `git apply`), checks the
  applied diff is byte-identical to the committed patch, and runs both binaries on
  public catalog data with identical arguments.
* **Run:** [35249590299](https://github.com/BioMarco/VoxelScaleGuard/actions/runs/35249590299)
  (build + before/after + physical-size checks). Toolchain in that run: gcc 13.3.0,
  cmake 3.31.6, ninja 1.13.2, `ubuntu-24.04`.
* **Measured**, on `PHerc0009B` at `-g 0 --scale 1`, 128×128 crop, one slice,
  streamed from the public Open Data bucket:

  | | baseline (`main`) | patched |
  |---|---|---|
  | log line | `Voxel size: 1.0 (no metadata found…)` | `Voxel size (remote volume metadata): 8.64 micrometer` |
  | `.zattrs` unit | `nanometer` | `micrometer` |
  | `.zattrs` scale, level 0 | `[1, 1, 1]` | `[8.64, 8.64, 8.64]` |
  | TIFF `XResolution` | absent | `2939.814697265625` px/inch |
  | decoded pixels | — | **identical** |

* **Control:** the same binary pair on `PHerc0172` (7.91 µm), the legacy-shaped
  volume the existing live-S3 test pins: `.zattrs` goes from `nanometer`/`[1,1,1]`
  to `micrometer`/`[7.91,…]`, TIFF gains `3211.1252` px/inch, decoded pixels
  identical.
* **Unit coherence:** `--voxel-size` given as `8640 nanometer`, `8.64 micrometer`,
  `0.00864 millimeter` and `0.00000864 meter` all declare and resolve to the same
  8.64 µm, with `.zattrs` and the TIFF agreeing in every case.
* **Unusable input** (`--voxel-size 0`, `-3`, `nan`, unknown unit) exits non-zero
  and writes no physical scale, matching `main`'s behaviour.
* **Unknown size**, driven end to end with the real binary by rendering a local-only
  store with no metadata document: the render declares **no** `multiscales` block
  and **no** TIFF resolution tag, and says so on stderr. The same run on `main`
  declares `nanometer`/`[1,1,1]`, i.e. 1 nm — the fabricated measurement this
  removes.

### Limitations, stated plainly

* **Two volumes, one crop, one slice.** This demonstrates the correction and the
  absence of a pixel regression; it is not a survey of the catalog, and no claim is
  made about volumes not rendered here.
* **One build configuration** (`QuickBuild`, gcc 13.3, Linux, `--scale 1`). Other
  presets, compilers and scales are unmeasured.
* **Identical decoded pixels** is a property of these runs on these volumes, not a
  proof that no input can change them. The patch touches no pixel-producing code
  path.
* The patch also changes the log line to state the size exactly as `.zattrs`
  declares it; `main` printed the caller's unit next to a value that could be in
  micrometres. No code in the tree parses that line (checked).
* Two strings a user can see change: `Voxel size (from CLI): …` becomes
  `Voxel size (command line): …`, and the unsupported-unit error drops the words
  "for TIFF resolution" because the abort now precedes both outputs. Both are
  cosmetic; neither is parsed by anything in the tree.

### Credits

The idea of having the renderer consult the **volume's own remote voxel size** is
**NicolasHuberty**'s, from PR #1417, which lapsed to an inactivity bot rather than
being rejected. The `samplePixelSize` schema handling,
`vc::metadata::resolveLocalStoreVoxelSize`, and the `Volume::voxelSize()` semantics
this reuses are **Bullo27**'s and the villa maintainers' (PRs #1227, #1229, #1454).
The VC3D enable-predicate diagnosis is **Bullo27**'s from PR #1228. The issue
reports are **DarthCeltic**'s (#1403) and **Bullo27**'s (#1226).

For the avoidance of doubt: the `vc_grow_seg_from_seed` half of #1403 is **already
fixed upstream**. This contribution does not claim it.

### Follow-up, deliberately not in this PR

`apps/VC3D/SegmentationCommandHandler.cpp:2076-2078` suppresses `--voxel-size` for
a native-resolution remote volume, so this CLI fix is not reachable from VC3D — the
route most users take. That changes GUI behaviour and its predicate has a lapsed
history (#1228), so it is proposed separately rather than bundled here.

---

# PART 2 — before you open the PR

## What you must do yourself

1. **Write the "Why I am sending this" block** in Part 1. `CONTRIBUTING.md`:
   *"Any LLM generated PR must be accompanied by human-written commentary explaining
   why this PR is relevant or useful"*, and it expects the work to come out of
   *"a human interacting with the codebase in an attempt to work on the scroll
   data"*. Say what you were doing and why the wrong declared size mattered. I
   cannot write this for you and have not tried to.
2. **Tick the checkbox** `- [ ] I personally verified that the example and proof
   above were produced by this PR on the stated data.` — only if you have. The
   honest reading: the evidence was produced by the CI workflow on the stated data,
   and you should satisfy yourself of that before ticking. If you would rather
   verify it directly, the workflow takes a commit as input and can be dispatched
   from the Actions tab.
3. **Decide about the evidence image.** `CONTRIBUTING.md` asks for before/after
   evidence *"in metric form and also in the form of images or videos"*, and notes
   that *"Any bugfix PR must be accompanied by a screenshot of the error (either
   terminal or within the tool), and the script/tool running without error
   afterward"*. `DOCS/evidence/before-after.png` covers the metadata, the two
   rendered images and the difference panel; the terminal logs are in
   `DOCS/CI_VALIDATION.md`. If you want a conventional terminal screenshot as well,
   take one from the run log and attach it too.
4. **Confirm the two open PRs on the same file are not a problem for you:**
   [#1797](https://github.com/ScrollPrize/villa/pull/1797) and
   [#1717](https://github.com/ScrollPrize/villa/pull/1717) both touch
   `vc_render_tifxyz.cpp`. Neither fixes this defect, and the applied diff is
   identical to the committed patch, but a maintainer may ask you to rebase if one
   of them lands first.

## What is already in place

State of the world, so nothing has to be re-done or re-checked before you open the PR.
None of this is a claim about *you* having run anything — the builds and renders were
done by the CI workflow in the evidence repository, and these bullets describe the
resulting repository state.

* the fork exists: **<https://github.com/BioMarco/villa>** (public, `fork: true`,
  parent `ScrollPrize/villa`);
* the branch is pushed: **`fix/render-voxel-size-from-open-volume`** @
  `d419dece6af51e0e015f6dc1df92c0312be76075`;
* checked via the GitHub API that the commit contains **exactly three files**
  (`vc_render_tifxyz.cpp` +235/−65, `Zarr.hpp` +5/−0, `Zarr.cpp` +10/−0), and that
  the branch is **0 behind / 1 ahead** of `ScrollPrize/villa` `main`;
* nothing from VoxelScaleGuard is in the fork: no harness, no CI, no documents, no
  data, no images. The branch differs from upstream `main` by those three files and
  nothing else.

## Open the PR

<https://github.com/BioMarco/villa/pull/new/fix/render-voxel-size-from-open-volume>

Base: `ScrollPrize/villa` `main`. Opening it is your action, not this repository's.
**Do not merge anything into VoxelScaleGuard's `main`**; that instruction stands.
