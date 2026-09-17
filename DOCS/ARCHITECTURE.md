# ARCHITECTURE

The design of the proposed fix, why it takes this shape, and the alternatives
that were rejected on evidence.

---

## 1. Design principle

**One fact, one resolution path.**

Before the patch, the physical voxel size of a rendered volume was established by
three mutually unaware mechanisms:

| Mechanism | Where | What it knows |
|---|---|---|
| `Volume::NewFromUrl` → `loadRemoteVolumeMetadata` → `vc::metadata::voxelSizeFromStoreMetadata` | `core/src/Volume.cpp:84-116`, `:1345` | the full set of published schemas **and** the network |
| `vc::metadata::resolveLocalStoreVoxelSize` | `core/src/VoxelSizeMetadata.cpp:165` | the full set of published schemas, local only. Already used by `vc_grow_seg_from_segments` |
| `readVolumeVoxelSize` (private, renderer-local) | `apps/src/vc_render_tifxyz.cpp:986` | one key name, local only, no value validation |

The renderer used the third. The patch deletes it and routes through the first two,
which is why the fix is small: the work is not writing a resolver, it is removing a
duplicate one.

---

## 2. The resolution chain

```
                        ┌──────────────────────────────────────────┐
   --voxel-size  ──────▶│ 1. explicit CLI override                 │
   --voxel-unit         │    → micrometres via explicitMicrometer… │
                        └──────────────┬───────────────────────────┘
                                       │ miss
                        ┌──────────────▼───────────────────────────┐
                        │ 2. local store document                   │
   vol_path/meta.json   │    vc::metadata::resolveLocalStoreVoxelSize│
   vol_path/metadata.json│   (shared core resolver, as Volume uses) │
                        └──────────────┬───────────────────────────┘
                                       │ miss
                        ┌──────────────▼───────────────────────────┐
                        │ 3. the volume already opened              │
   Volume::NewFromUrl   │    remoteVolume->voxelSize()              │
                        │    (already fetched; no extra request)    │
                        └──────────────┬───────────────────────────┘
                                       │ miss
                        ┌──────────────▼───────────────────────────┐
                        │ 4. Unspecified                            │
                        │    value = 1.0, flagged NOT a measurement │
                        └──────────────────────────────────────────┘
```

Step 3 is the centrepiece. It is the value the rest of the process already agrees
on, it costs no network request, and it cannot disagree with the volume actually
being streamed. It is also what makes the patch immune to the URL-fragment hazard
that a separate fetch would have had to defend against: `Volume::NewFromUrl`
already works from `spec.sourceUrl`, the fragment-free identity.

### 2.1 Ordering is the fix, not an implementation detail

The chain must run **after** the source volume is opened. Pre-patch the value was
resolved before the volume existed, which is precisely why the renderer had no
access to the remote metadata and grew its own filesystem-only reader. The patch
moves the resolution to just after `chunk_cache` is established and the volume is
open.

Consequences that had to be handled:

* `tgt_scale` and `ds_scale` (needed for the TIFF-DPI computation) are already
  defined before that point — verified, no reordering needed.
* `render_level_voxel_size` is used inside the per-segmentation `process_one`
  lambda, so it must be declared in `main`'s scope before the lambda and assigned
  before the lambda runs. Both hold.

---

## 3. Value provenance is explicit

```cpp
enum class VoxelSizeSource { Cli, LocalStoreMetadata, RemoteVolume, Unspecified };

struct ResolvedVoxelSize {
    double micrometerPerVoxel = 1.0;   // always micrometres per voxel
    VoxelSizeSource source = VoxelSizeSource::Unspecified;
    [[nodiscard]] bool isUsable() const { return source != VoxelSizeSource::Unspecified; }
};
```

Two things are carried separately that were previously conflated into one `double`:

* **the number**, always normalised to micrometres per voxel;
* **whether it is a measurement at all.**

`isUsable()` is what stops `1.0` from being written out as a physical scale. This
is the reporter's own worry — *"I don't know whether anything downstream relies on
the current fallback behaviour"* — answered structurally rather than by a comment:
the placeholder can no longer be mistaken for a value, because the type says which
it is.

The enum also makes the emitted number and unit derivable instead of guessed, and —
after the unit regression of `RESULTS.md` §10 — computed **together**, so a number
and a unit that describe different physical sizes are not expressible:

```cpp
zarrVoxelUnit(resolved, voxel_unit)      // caller's unit for Cli; "micrometer" otherwise
zarrVoxelValue(resolved, explicitValue)  // caller's number for Cli; the µm value otherwise
// invariant, asserted in the tests:
//   zarrVoxelValue(...) * micrometersPerUnit(zarrVoxelUnit(...)) == micrometerPerVoxel
```

A size from a store document is in micrometres **by definition** (`voxelsize` is
µm; `Volume::voxelSize()` is µm). So the unit follows the source, and
`--voxel-unit` remains meaningful only for a `--voxel-size` given on the command
line. That is what removes the ×1000 error without changing
`--voxel-unit`'s default.

---

## 4. Validation

| Rule | Where enforced |
|---|---|
| finite | `explicitMicrometerPerVoxel`, and `Volume::voxelSize()` / `positiveNumber` in core |
| strictly positive | same |
| unit recognized (µm / nm / mm / m, and their spellings) | `explicitMicrometerPerVoxel`; an unknown unit is a hard error, surfaced before the volume is opened |
| µm vs mm vs cm distinguished | mm → ×1000, m → ×10⁶, nm → ×0.001, all in one place. The millimetre→micrometre conversion for `samplePixelSize` lives in `VoxelSizeMetadata.cpp:105-106`, i.e. the shared resolver, not duplicated |
| multiple resolution levels | `ds_scale = 2^-group_idx`; the level-g voxel size is `base / ds_scale`, applied consistently for TIFF DPI and `.zattrs` |
| anisotropic voxels | **not assumed isotropic.** The code already models in-plane and through-plane separately: `sYX = baseVoxelSize / pixelsPerVoxel × 2^l` and `sZ = baseVoxelSize × sliceStep` (`core/src/Zarr.cpp:386-394`). This patch does not touch that |

The last row deserves emphasis because the brief asked for it explicitly: the
existing design is already anisotropic-correct, and the patch preserves it. The
failure was in obtaining the base number, not in applying it per axis.

---

## 5. Area handling and output protection

The brief asked whether invalid values can silently delete valid surfaces, and
whether early validation can prevent expensive doomed runs.

**Not in this tool, and the patch does not introduce it.** `min_area_cm` /
`remove_all(seg_dir)` live in the growth tools
(`vc_grow_seg_from_segment*.cpp`, `core/src/GrowPatch.cpp`), not in
`vc_render_tifxyz`. That path was already corrected upstream
(`ROOT_CAUSE_ANALYSIS.md` §2), and this patch does not touch it. This is recorded
rather than "fixed", because fixing something that is not broken would be an
unjustified change.

**Early validation:** the patch validates the CLI inputs *before* opening the
volume, so a bad `--voxel-size` or an unreadable `--voxel-unit` fails in
milliseconds instead of after the remote pyramid is opened. The metadata tiers
cannot be validated earlier than the volume is open, by definition — which is why
they are not.

**Non-destructive by construction:** no file is removed, no directory is created
before it is needed (the zarr output path is created as before), and the unknown
case degrades the *metadata* rather than the artifact. The render still completes
and the pixels are unchanged.

---

## 6. Rejected alternatives

| Alternative | Why rejected |
|---|---|
| **Python wrapper** that pre-computes and passes `--voxel-size` | Adds a second source of truth for a fact the process already holds, and requires every caller to remember it. The brief explicitly warns against hiding a C++ error in a Python wrapper |
| **New S3/HTTP client in the renderer** | The brief asks not to implement a new S3 client without demonstrated need. There is none: `Volume::NewFromUrl` already does it |
| **Expose `loadRemoteVolumeMetadata` again as a new public `remoteVolumeVoxelSize()`** (PR #1417's design) | Works, but re-fetches the document a second time, can disagree with the volume being streamed, and must independently defend against the URL-fragment hazard its own review flagged. Step 3 removes the need instead of guarding it |
| **Keep `readVolumeVoxelSize` and add schema support to it** | Perpetuates the third opinion. It exists in one place only, so deleting it is strictly smaller than fixing it |
| **Change `--voxel-unit`'s default to `micrometer`** (PR #1313's design) | Probably right long-term, but a separate behaviour change with a separate blast radius, separately lapsed once. The source-derived unit achieves correct output without it |
| **Refuse to render when the size is unknown** | Destructive and unjustified: the pixels are fine, only the scale metadata is unavailable. Withholding the scale is proportionate; refusing the render is not |
| **Change `writeZarrAttrs`'s signature to make the scale optional** | A public interface change for a case handled by passing an empty unit — which `writeZarrAttrs` already supports (`core/src/Zarr.cpp:374`) |

---

## 7. Files touched

Three files, one commit's worth of change. The renderer is the fix; the two `core`
files are the writer contract it needs in order not to publish a fabricated scale
when the size is unknown.

```
volume-cartographer/apps/src/vc_render_tifxyz.cpp       +235 / -65
volume-cartographer/core/src/Zarr.cpp                   +10 / -0
volume-cartographer/core/include/vc/core/util/Zarr.hpp   +5 / -0
                                                         = +250 / -65
```

* adds `#include "vc/core/util/VoxelSizeMetadata.hpp"`
* replaces `readVolumeVoxelSize` with `VoxelSizeSource`, `ResolvedVoxelSize`,
  `voxelSizeSourceName`, `zarrVoxelUnit`, `zarrVoxelValue`,
  `explicitMicrometerPerVoxel`,
  `resolveRenderVoxelSize`
* moves the resolution to after the volume is opened
* validates CLI inputs earlier, including the unit
* passes a source-correct unit to both `writeZarrAttrs` call sites
* omits the physical scale, with a distinct warning, when nothing resolves

Deliberately **not** touched: `core/`, `writeZarrAttrs`, `Tiff.cpp`, any
metadata convention, any other `vc_*` tool, the VC3D GUI condition (documented in
`FEASIBILITY.md` §8).

---

## 8. The harness, and why it is separate

The patch cannot be compiled here (no Qt/OpenCV/vcpkg), so the evidence had to be
produced some other way. The harness follows two rules:

1. **Never re-implement what is being tested.** `setup.ps1` copies
   `VoxelSizeMetadata.cpp`, `RemoteUrl.cpp`, `Json.cpp` and upstream's own test
   file **byte-for-byte** from the pinned commit and never edits them.
2. **Execute the pre-patch behaviour, do not describe it.** The defect
   reproduction uses a verbatim copy of `readVolumeVoxelSize`, kept in a file
   whose header says it exists to stay broken.

This gives a real control: upstream's 13-case suite passes unmodified in the same
binary, which is the evidence that the harness is running their code and not a
paraphrase of it.

`harness/src/vsguard/render_voxel_size_resolution.*` is a mirror of the patch's
decision procedure, present so the *fixed* logic is executable too. It is not the
deliverable and is labelled as such: the deliverable is the diff to the real file.

---

## 9. What would make this architecture complete

The chain has a hole that this patch does not close: the GUI suppresses
`--voxel-size` for a native-resolution remote volume
(`SegmentationCommandHandler.cpp:2076-2078`), so the CLI fix is unreachable from
the route most users take. The corrected predicate is "we have a positive voxel
size":

```cpp
const double renderVoxelSizeUm = renderVolume ? renderVolume->voxelSize() : 0.0;
_cmdRunner->setRenderVoxelSize(renderVoxelSizeUm, std::isfinite(renderVoxelSizeUm) && renderVoxelSizeUm > 0.0);
```

That is a two-line change. It is documented here and in `FEASIBILITY.md` §8 but
not applied: it changes GUI behaviour, and `setRenderVoxelSize`'s second argument
has a history that includes a lapsed PR (#1228). It should be its own commit, so a
maintainer can take the CLI fix without it.
