# ROOT_CAUSE_ANALYSIS

**Subject:** `vc_render_tifxyz` resolves a physically wrong voxel size, silently.
**Upstream revision analysed:** `ScrollPrize/villa` @ `757f70c0140a4cfbbbd44975ef09558444b96980` (`main`)
**Method:** reading the source at that revision, plus executing the real upstream
translation units (`VoxelSizeMetadata.cpp`, `RemoteUrl.cpp`, `Json.cpp`) and a
byte-verbatim copy of the renderer's own reader, over metadata documents fetched
from the live public bucket.

Every claim below is marked with how it was established:

* **[read]** — read from the pinned source; file and line cited.
* **[exec]** — produced by running code, with the command recorded in `RESULTS.md`.
* **[live]** — observed against published data from the real catalog.

---

## 1. The symptom, as reported

`ScrollPrize/villa#1403` reports that rendering against a remote volume prints

```
Voxel size: 1.0 (no metadata found; override with --voxel-size)
```

and continues with the wrong physical scale. The reporter explicitly declined to
send a patch, saying they did not know whether anything downstream relied on the
current fallback. That question is answered in §6.

The report bundles two concerns. They have **different** status, which matters:

| Concern | Status on `main` |
|---|---|
| `vc_grow_seg_from_seed` deletes its output when the voxel size is 0 | **Already fixed** — not by the render half's fix, and not by anything this project did |
| `vc_render_tifxyz` renders at voxel size 1.0 | **Still present** |

## 2. `vc_grow_seg_from_seed` — not a bug any more (independent verification)

**[read]** `apps/src/vc_grow_seg_from_segments.cpp:1165`:

```cpp
if (const auto resolved = vc::metadata::resolveLocalStoreVoxelSize(vol_path)) {
    voxelsize = static_cast<float>(*resolved);
}
```

with the comment at `:1161-1163` stating the intent: *"Resolved the same way
Volume does, so a store that states its voxel size only through an acquisition
record is not read as 0."*

**[read]** The reader that issue #1403 quotes — `float voxelsize =
json::parse(std::ifstream(vol_path/"meta.json"))["voxelsize"];` — is not present
in `main`. The reporter's own follow-up comment on #1403 (2026-08-11) concedes
this: they had been working from a checkout of `merge-ink-pipelines`, not `main`.
The value now comes from `Volume::voxelSize()` at `:365`-equivalent code.

**Conclusion:** the deletion-and-exit-0 symptom is historical. This project did
not fix it and does not claim it. Reproducing the reported 99-generation,
15.6M-vx² run is therefore not a meaningful validation target; those figures are
the reporter's, are unverified by us, and are not used as evidence anywhere here.

## 3. The live defect in `vc_render_tifxyz`

### 3.1 Where a physical voxel size could come from

**[read]** `vc_render_tifxyz.cpp`, pre-patch, lines 972-1003 (numbering at the
pinned revision: the function is at `:986`). The complete set of sources the tool
consulted:

```cpp
static std::optional<double> readVolumeVoxelSize(const std::filesystem::path& volPath)
{
    auto tryFile = [](const std::filesystem::path& p, const char* key) -> std::optional<double> {
        if (!std::filesystem::exists(p)) return std::nullopt;      // (1)
        try {
            Json j = Json::parse_file(p.string());
            Json sub = key ? (j.is_object() && j.contains(key) ? j[key] : Json{}) : j;
            if (key && !sub.is_object()) return std::nullopt;
            if (sub.is_object() && sub.contains("voxelsize") && sub["voxelsize"].is_number())
                return sub["voxelsize"].get_double();              // (2)
        } catch (...) {}
        return std::nullopt;
    };
    if (auto v = tryFile(volPath / "meta.json", nullptr)) return v;
    if (auto v = tryFile(volPath / "metadata.json", "scan")) return v;
    if (auto v = tryFile(volPath / "metadata.json", nullptr)) return v;
    return std::nullopt;
}
```

Three independent defects are visible in those eleven lines.

### 3.2 Root cause A — no remote path at all, and the value was already in hand

**(1)** is `std::filesystem::exists`. For a streaming volume `vol_path` is a
local cache directory that contains no metadata document, so this is false for
every candidate and the function returns `std::nullopt` without ever attempting a
network read. **[read]**

**[read]** But the tool *has already opened the volume remotely* — `:1315`:

```cpp
remoteVolume = Volume::NewFromUrl(remoteUrl, remoteAuth);
```

and `Volume::NewFromUrl` (`core/src/Volume.cpp:1345`) calls
`loadRemoteVolumeMetadata(remoteUrl, auth)`, which fetches and normalises exactly
the document the renderer needed, and stores the result in
`metadata_["voxelsize"]` (`Volume.cpp:67-82`, `:1352`). `Volume::voxelSize()`
(`Volume.cpp:1575-1582`) returns it, or `0.0` when it could not be resolved.

So the renderer fetches the correct value, keeps it in an object it holds, and
then asks the filesystem instead. This is the architectural fault: **two
independent resolution paths for one fact**, one of which cannot see the network.

### 3.3 Root cause B — the local reader knows fewer schemas than the rest of VC3D

**(2)** looks only for a literal top-level `voxelsize` key (and the same key one
level under `scan`).

**[read]** The rest of the codebase resolves through
`vc::metadata::voxelSizeFromStoreMetadata` (`core/src/VoxelSizeMetadata.cpp`),
which recognizes six top-level aliases **and** the ESRF/BM18 acquisition record
`scan.tomo.acquisition.detector.samplePixelSize` (millimetres, `×1000` to µm)
**and** the same record at the document root, **and** the `source.metadata` form
used by derived stores. `resolveLocalStoreVoxelSize` was added to
`VoxelSizeMetadata.hpp:38-43` precisely so that a tool reading a store without
constructing a `Volume` "agree[s] with `Volume::voxelSize()` on what the store
says".

`vc_grow_seg_from_segments` adopted it. `vc_render_tifxyz` did not.

**[live] [exec]** This is the dominant real-world case, not a corner. Of four
published volumes probed:

| Published store | Top-level `voxelsize`? | `samplePixelSize` (mm) | True size |
|---|---|---|---|
| `PHerc0009B/…-8.640um-…` | no | `0.00864` | **8.64 µm** |
| `PHercParis4/…-45.532um-…` | no | `0.045532` | **45.532 µm** |
| `PHercParis4/…-2.400um-…` | no | `0.0024` | **2.4 µm** |
| `PHerc0172/…-7.910um-…` | **yes** (`7.91`) | — | 7.91 µm |

The old reader resolved exactly **one** of the four. That one is the legacy-shaped
volume, and it is the volume that `core/test/test_volume_live_s3.cpp` pins:

```cpp
constexpr const char* kVolumeUrl =
    "s3://vesuvius-challenge-open-data/PHerc0172/volumes/"
    "20241024131838-7.910um-53keV-masked.zarr";
```

**The single live-S3 test in the suite exercises the only catalog entry the buggy
reader gets right.** That is why the gap survived. This corroborates the survey
in villa#1226 (which measured 1 of 71 volumes unaffected) without depending on it.

### 3.4 Root cause C — the reader validates the type but not the value

**(2)** checks `is_number()` and nothing else. The sign and finiteness check lived
at the *call site* (`:1387`, `std::isfinite(*mv) && *mv > 0.0`), so the reader
cannot distinguish "0 µm/voxel" from an answer. **[exec]** confirms the
consequence: a store containing `{"voxelsize": 0}` makes `readVolumeVoxelSize`
return `0.0`, and `{"voxelsize": -3}` makes it return `-3.0`. Both are handed to
the caller as if they were measurements. A negative value then flows into
`voxelSizeToDpi` (`core/include/vc/core/util/Tiff.hpp:15`), yields a negative DPI,
and `dpi > 0.f` silently suppresses the TIFF resolution tags
(`core/src/Tiff.cpp:236-239`) — a second silent degeneration.

The shared resolver rejects both (`positiveNumber`,
`VoxelSizeMetadata.cpp:68-74`). **[exec]**

### 3.5 Root cause D — a placeholder is presented as a physical measurement

**[read]** On any miss the pre-patch code set `base_voxel_size = 1.0`, left
`hasPhysicalVoxelSize` false, and passed `1.0` to `writeZarrAttrs` anyway
(`:1374-1375`, `:1394-1396`, `:1426-1428`), which writes it as the OME-Zarr axis
scale (`core/src/Zarr.cpp:386-404`). The unit written is
`--voxel-unit`, whose default is `"nanometer"` (`:1084`).

**[exec] [live]** The declared physical voxel size is therefore
`value × 0.001 µm`, i.e. `0.001 µm` when unresolved. Against the real documents:

| Store | Declared as shipped | Correct | Error |
|---|---|---|---|
| `PHerc0009B/…-8.640um-…` | `1` nm = 0.001 µm | 8.64 µm | **×8640** |
| `PHercParis4/…-45.532um-…` | `1` nm = 0.001 µm | 45.532 µm | **×45532** |
| `PHercParis4/…-2.400um-…` | `1` nm = 0.001 µm | 2.4 µm | **×2400** |
| `PHerc0172/…-7.910um-…` | `7.91` nm = 0.00791 µm | 7.91 µm | **×1000** |

The error is the product of two independent factors: how wrong the resolved
number is (1.0 vs the true size, up to ×45.5), and the unit default (×1000).

Note the last row. Even where the reader succeeds, the output is still wrong by
1000×, because the value it read is in micrometres while it is declared as
nanometres. **Fixing discovery alone changes 8640× wrong into 1000× wrong.** This
is why the unit is in scope rather than a separate nicety, and it is verified
independently of the discovery bug.

### 3.6 The GUI route bypasses the CLI fix entirely

**[read]** `apps/VC3D/SegmentationCommandHandler.cpp:2074-2078`:

```cpp
_cmdRunner->setRenderVoxelSize(
    renderVolume ? renderVolume->voxelSize() : 0.0,
    renderVolume &&
        (renderVolume->baseScaleLevel() > 0 ||
         renderVolume->hasExplicitVoxelSizeOverride()));
```

and `CommandLineToolRunner.cpp:147-149`:

```cpp
_useRenderVoxelSize = enabled && std::isfinite(voxelSizeUm) && voxelSizeUm > 0.0;
```

The enable predicate is "is this volume rebased, or was its size explicitly
overridden?" A native-resolution remote volume satisfies neither, so no
`--voxel-size` is passed, and `--remote-url` *is* passed
(`CommandLineToolRunner.cpp:657-659`, fed from
`SegmentationCommandHandler.cpp:2014` = `volume->remoteLocator()`).

So the renderer receives a remote locator and no explicit size. That is precisely
the configuration root causes A–D act on. The predicate asks a proxy question
("was this rebased?") instead of the real one ("do we have a positive voxel
size?"). This is the same defect as PR #1228 described; it was closed unmerged on
2026-08-09, and the condition is unchanged at this revision. **[read]**

## 4. Relationship to villa#1226 and to the earlier fix

`#1226` (`vc_grow_seg_from_seed: remote volumes at native resolution get
voxelsize 0…`, closed as completed 2026-08-11) has the **same underlying cause**
as root cause A on the seeding side: `Volume::NewFromUrl` gated the
`samplePixelSize` fallback behind `spec.baseScaleLevel > 0`, so at native
resolution the only reachable source was disabled.

That gating is gone at this revision: `loadRemoteVolumeMetadata(remoteUrl, auth)`
takes two parameters (`Volume.cpp:84`), and the gate is replaced by
`vc::metadata::voxelSizeFromStoreMetadata`, applied unconditionally
(`Volume.cpp:93`, `:1345`). **[read]**

**Different execution paths, shared cause:** `#1226`'s fix repaired *Volume
construction*; `vc_render_tifxyz` never used Volume construction for this value in
the first place, so that fix could not reach it. `#1403` is therefore **not a
regression** of the `#1226` fix and **not** a duplicate of it — it is a second,
independent consumer of the same fact that was left on its own weaker path.

## 5. Not a stale checkout, and not already fixed

The renderer half was checked at the pinned `main` revision, not at a branch:

* the private `readVolumeVoxelSize` is present at `:986`; **[read]**
* `--voxel-unit` still defaults to `"nanometer"` at `:1084`; **[read]**
* `writeZarrAttrs` is still called with the pre-patch `voxel_unit` at both sites
  (pre-patch `:1581`, `:1789`); **[read]**
* the GUI suppression predicate is unchanged (`SegmentationCommandHandler.cpp:2076-2078`). **[read]**

**[read]** A prior fix exists and is *closed unmerged*: PR #1417,
`vc_render_tifxyz: discover remote volume voxel size` (author `NicolasHuberty`,
2 commits, head `459ae44406`), closed 2026-08-27 by
`github-actions[bot]`: *"Closed automatically under the repository PR time limits
because this PR has had no activity for 14 days."* It was not rejected by a
maintainer. Its review found no defects (one Copilot pass raised two concerns,
discussed in §7).

## 6. Does anything depend on the current fallback?

The reporter's open question. Answered by reading the only consumers of the two
values the fallback feeds:

* **TIFF DPI.** `tifDpi` reaches `TiffWriter` and is applied only under
  `if (dpi > 0.f)` (`core/src/Tiff.cpp:236-239`). There is no branch that
  *requires* a DPI to be present. **[read]**
* **OME-Zarr axis scale.** `writeZarrAttrs` computes `sYX = baseVoxelSize / px ×
  2^l` and `sZ = baseVoxelSize × step`, and assigns `unit` only when it is
  non-empty (`core/src/Zarr.cpp:386-404`, `:372-376`). Nothing reads the value
  back. **[read]**
* **The render itself.** `buildOffsetList`'s comment (`:348-353`) states the
  slice offsets are in *level-g voxels* and are deliberately **not** scaled by
  `ds_scale`. `base_voxel_size` is not used for geometry at all. **[read]**

**Conclusion:** `base_voxel_size` affects only the physical-scale metadata the
tool writes. It does not affect the rendered pixels. So the correction changes
declared physical scale and nothing about surface growth, sampling or image
content — which is what makes it safe to change, and also why it went unnoticed:
the pictures looked fine.

## 7. The two review concerns on PR #1417, reproduced rather than assumed

Review of #1417 raised two concerns. Both were treated here as hypotheses and
executed. **[exec]**

### 7.1 Remote URL fragments — CONFIRMED

`vc::joinRemoteUrlPath` (`core/src/RemoteUrl.cpp:180-202`) splits on `?` only. It
never looks for `#`. Given a locator carrying the base-scale selector:

```
https://example.test/x.zarr#vc-base-scale=1
```

it produces

```
https://example.test/x.zarr#vc-base-scale=1/metadata.json
```

A request target that cannot exist. `vc::parseRemoteVolumeSpec`
(`RemoteUrl.cpp:147-178`) already separates the two identities —
`spec.sourceUrl` is documented as "the fragment-free URL used for network
requests" — so this is avoidable, not merely theoretical. The failure is silent:
it is indistinguishable from a store that publishes no voxel size.

Is the locator reachable in practice? **[read]** Yes: the GUI passes
`volume->remoteLocator()` (`SegmentationCommandHandler.cpp:2014`) to
`--remote-url`, and `remoteLocator()` is the *portable* identity, which
deliberately retains `#vc-base-scale=N` when set (`Volume.cpp:1315`,
`RemoteUrl.hpp:22-36`, `RemoteUrl.cpp:171-174`). It is set for rebased volumes.
So the fragment case is reachable whenever a rebased remote volume is rendered
from the GUI. Note this specifically defeats the naive version of the fix.

The concern is a **latent** bug for the *current* code, which never calls
`joinRemoteUrlPath` with a user locator: the pre-patch code has no remote path at
all. It becomes real only in the presence of a remote-discovery fix — which is
exactly what #1417 added, and exactly what the patch here adds. It is therefore
handled by construction: the patch never fetches metadata itself, it reads
`Volume::voxelSize()`, and `Volume::NewFromUrl` already works from
`spec.sourceUrl`. See §8.

### 7.2 Invalid local metadata — REAL, and worse than described

The concern was that an invalid local `voxelsize` makes the renderer skip the
remote fallback, because the first tier "succeeded". **[exec]** The actual
behaviour is worse: the reader does not merely signal success with an unusable
value, it **returns** the unusable value, including `0.0` and `-3.0` (§3.4). The
pre-patch call-site check caught the positive-value case and *misreported* it as
"invalid metadata voxelsize; using default 1.0", losing the distinction between a
store that published nonsense and one that published nothing.

Both are addressed in the patch: resolution goes through the shared resolver,
which validates once, and the origin of the value is tracked explicitly so that
"unusable" and "absent" produce different messages and different output.

## 8. What this analysis therefore implies for a fix

1. Do not add a second metadata fetcher. Read `Volume::voxelSize()` from the
   volume the renderer **already opened**. This removes root cause A, makes the
   fragment concern unreachable by construction, costs no request, and cannot
   disagree with the volume being streamed.
2. Use `vc::metadata::resolveLocalStoreVoxelSize` for the local case, so the tool
   agrees with `Volume::voxelSize()` and with `vc_grow_seg_from_segments`
   instead of keeping a third, weaker opinion.
3. Resolve **after** the volume is open. The ordering is the fix, not a detail.
4. Track where the value came from, so "unknown" cannot be emitted as a
   measurement, and so the emitted unit always matches the number.
5. Fix the enable predicate in the GUI, or the CLI fix is unreachable through the
   route actual users take.

Items 1–4 are implemented; item 5 is prepared and justified but not applied,
because it changes GUI behaviour and public-looking behaviour that the argument
above does not by itself authorise. See `FEASIBILITY.md`.

## 9. Limitations of this analysis

* **The patched application binary was not compiled or run here.** Qt, OpenCV,
  Ceres, CGAL, TIFF and the rest are not installed, and the full
  volume-cartographer dependency closure is a heavy install that was not
  authorised. The defect, the root causes and the decision procedure were
  established by executing the real upstream translation units plus a verbatim
  copy of the renderer's reader, not the renderer itself. See `RESULTS.md` §7 for
  exactly what that does and does not cover.
* **No end-to-end render on a real volume was performed.** The physical-scale
  consequence is derived from `writeZarrAttrs` and `Tiff.cpp` by reading them,
  and from the live metadata documents by executing the resolver; the `.zattrs`
  file itself was not produced.
* **The 56-of-71 catalog survey in #1226 is not independently reproduced.** We
  probed four volumes. The survey is cited as corroboration with attribution, not
  as our measurement.
