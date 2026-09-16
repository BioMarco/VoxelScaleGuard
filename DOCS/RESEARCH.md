# RESEARCH

Independent reconnaissance for VoxelScale Guard. Sources consulted, what they
contain, and — importantly — which of the original brief's assumptions did not
survive contact with the source.

---

## 1. Sources and exact revisions

| Source | Revision / date | Licence | How consulted |
|---|---|---|---|
| `https://github.com/ScrollPrize/villa` | `757f70c0140a4cfbbbd44975ef09558444b96980` (`main`), cloned shallow to depth 50 | MIT (`villa/LICENSE`, "Copyright (c) 2024 Vesuvius Challenge") | read the source at that commit; git commands run with a transient `-c safe.directory='*'` |
| `https://scrollprize.org/prizes` | fetched during this session | site content CC BY-NC 4.0 unless stated | full text read |
| `https://scrollprize.org/data` | see `PRIZE_REQUIREMENTS.md` | — | via the prizes page and the live bucket |
| `https://vesuvius-challenge-open-data.s3.amazonaws.com/…` | live, anonymous | Open Data | GET of 8 candidate documents, 4 found |
| `ScrollPrize/villa` issues #1403, #1226 | full text + all comments | MIT (repo) | GitHub REST API |
| `ScrollPrize/villa` PRs #1417, #1228, #1229, #1313, #1541, #1797 | metadata, bodies, review comments, diffs | MIT (repo) | GitHub REST API |

**Access limitation, recorded honestly:** `git log` on the shallow clone covers
only the last 50 commits, so full-file history queries were answered through the
GitHub API and by reading the working tree rather than through local `git log
--follow`. The commit hash of `main` was read from
`.git/refs/heads/main` and confirmed with `git rev-parse HEAD`.

**Local toolchain:** MSVC 14.34.31933, Windows SDK 10.0.22000.0, PowerShell 7.6.6,
Git 2.55.0.windows.5, Node v24.19.0, Python 3.13. No vcpkg, Qt, OpenCV, Docker,
MSYS2 or WSL. Details in `RESULTS.md` §1.

---

## 2. The Villa monorepo: what it is and where the relevant work lives

`villa` is the Vesuvius Challenge monorepo. Top level:

```
volume-cartographer/   VC3D — the virtual-unwrapping toolkit (C++/CMake/Qt). THE relevant subtree
vesuvius/              Python ML pipelines (ink detection, surface prediction, spiral fitting)
lasagna/               surface-prediction tooling
segmentation/          segmentation data/tooling
foundation/            volume registration and related utilities
ink-detection/  spiral-fitting/  dinovol/  scripts/  deprecated/
```

### The role of the monorepo in virtual unwrapping

The pipeline that matters here is: **CT volume → surface (tifxyz quadmesh) →
rendered/flattened 2-D image → ink detection**. `volume-cartographer` owns every
stage except ink detection:

* `core/` — the C++ library: `Volume` (local and remote zarr access, metadata),
  `VcDataset`/`Zarr` (zarr IO, `.zattrs`), `QuadSurface`/`Surface` (the tifxyz
  quadmesh and its sampling), `Slicing` (multi-slice reads through a surface),
  `render/` (`ChunkCache`, `ZarrChunkFetcher`, prefetch), `GrowPatch` (surface
  growth), `flattening/ABFFlattening`, `Atlas`, `VoxelSizeMetadata`.
* `apps/` — 40-plus CLI tools (`vc_*`) plus **VC3D**, the Qt GUI.
* `utils/` — `Json` (a thin wrapper over nlohmann/json, so only `Json.cpp`
  includes it), hashing, misc.

### Components on the path this project touches

| Concern | Component |
|---|---|
| Remote volume access (S3/HTTPS/Zarr) | `core/src/Volume.cpp` (`Volume::NewFromUrl`), `core/render/ZarrChunkFetcher.*`, `core/util/RemoteUrl.*` (locator parsing, `s3://` → HTTPS), `core/util/HttpFetch.*`, `core/util/RemoteFileCache.*` |
| Metadata handling | `core/src/VoxelSizeMetadata.cpp` (the **shared** voxel-size resolver), `core/src/Volume.cpp` (`loadRemoteVolumeMetadata`, `applyResolvedVoxelSize`), `utils/src/Json.cpp` |
| Physical scale output | `apps/src/vc_render_tifxyz.cpp` (resolves the size, computes DPI, computes the `.zattrs` scale), `core/src/Zarr.cpp` (`writeZarrAttrs`), `core/src/Tiff.cpp` (`voxelSizeToDpi`, RESOLUTION tags) |
| Surface growth | `core/src/GrowPatch.cpp`, `apps/src/vc_grow_seg_from_seed.cpp`, `apps/src/vc_grow_seg_from_segments.cpp` |
| Rendering | `apps/src/vc_render_tifxyz.cpp`, `apps/src/RenderPrefetch.hpp`, `apps/VC3D/*` (the GUI invoker) |

---

## 3. Documented limitations and recent relevant changes

Read from the source at the pinned revision:

* **`Volume::voxelSize()` returns 0 for "unknown"** and documents why: a physical
  measurement must degrade to "unavailable" rather than throw
  (`core/src/Volume.cpp:1575-1582`).
* **`applyResolvedVoxelSize`** warns when a store publishes no voxel size, with an
  explicit note that otherwise "every physical measurement derived from it —
  surface areas, the `min_area_cm` gate, the scale bar — silently becomes 0 too
  (#1603)" (`core/src/Volume.cpp:63-82`). `#1603` is a further issue in the same
  family, **after** the ones in the brief.
* **`VoxelSizeMetadata.hpp`** documents the exact schemas recognized and, notably,
  *why it does not search for anything that merely looks like a resolution*, and
  *why `source.resolution` is required rather than defaulted* (`:11-35`). It also
  documents that `resolveLocalStoreVoxelSize` exists so tools that read a store
  without building a `Volume` "agree with `Volume::voxelSize()` on what the store
  says" (`:38-43`). That function was written for exactly the class of bug found
  here; `vc_render_tifxyz` simply never adopted it.
* **`RemoteUrl.hpp`** documents the two-identity design: `sourceUrl` is the
  fragment-free URL for network requests, `portableLocator` keeps
  `#vc-base-scale=N` for persistence (`:22-36`). This is the fact that makes the
  fragment concern in PR #1417's review both real and avoidable.
* **`AGENTS.md` / `CLAUDE.md`** in the repo root state the project targets Ubuntu
  and macOS on amd64/arm64, that Windows builds go through MSYS2 UCRT64 or
  MSVC+vcpkg, and that flatboi (SLIM flattening, PaStiX-based) is unavailable on
  Windows. They also forbid unrequested installs — which is why the vcpkg
  dependency closure was not fetched.
* **`volume-cartographer/README.md`**: upstream requires CMake ≥ 3.28; the
  `windows-msvc` preset builds Qt/OpenCV/Ceres/CGAL from source via vcpkg.

Recent relevant merges found while looking for the fix (see
`ROOT_CAUSE_ANALYSIS.md` §4-§5):

| PR | State | Relevance |
|---|---|---|
| #1227 (via #1226) | merged | fixed `vc_grow_seg_from_seed`'s zero voxel size; **does not reach the renderer** |
| #1229 | merged 2026-08-04 | `.zattrs` scale must describe the render level, not level 0 — an adjacent, already-fixed scale bug in the same function |
| #1454 | merged 2026-08-20 | umbilicus frame metadata + a shared resolver; evidence the maintainers are consolidating resolution paths |
| #1417 | **closed unmerged** 2026-08-27 | the renderer fix; lapsed to the 14-day inactivity bot |
| #1228, #1313 | closed unmerged 2026-08-09 | the VC3D enabling condition, and the `--voxel-unit` default |
| #1541 | closed unmerged 2026-09-08 | a nested `samplePixelSize` search, aimed at the seeding side |
| #1797 | open | records `.remote_source.json` so `--remote-url` can be omitted |

---

## 4. Assumptions in the brief that did not survive verification

The brief asked not to confirm its own hypotheses automatically. Three did not
hold, and the project's shape changed accordingly.

### 4.1 "PROBLEM A: `vc_grow_seg_from_seed` … the surface can be deleted" — **no longer live**

Verified by reading `main`, not by trusting the issue's open state. The quoted
`std::ifstream(vol_path/"meta.json")` is absent; the tool resolves through
`resolveLocalStoreVoxelSize` (`apps/src/vc_grow_seg_from_segments.cpp:1165`). The
reporter's own follow-up comment concedes they had been on a different branch.
**Consequence:** the 99-generation / 15.6M-vx² run is not a validation target, and
its figures are never used as evidence here.

### 4.2 "PROBLEM B: a default of 1.0 may be used" — **confirmed, and larger than stated**

Not merely "a default of 1.0". The default is emitted as a *confident physical
scale* in the OME-Zarr axis metadata, with the unit defaulting to `nanometer`
while the value is a micrometer quantity. Measured error on real stores: ×2400 to
×45532.

### 4.3 "Reading that `meta.json` through the same S3-aware path the volume already uses would fix both" — **half right, and the wrong half**

Wiring an S3-aware read into the renderer would fix the *remote* case. It would
**not** fix the *local* case, because the renderer's own reader also lacks the
`samplePixelSize` schema: a locally downloaded modern store is misread the same
way. And it would not fix the unit mismatch, which leaves output ×1000 wrong even
when the size resolves correctly. Both of those are demonstrated in
`RESULTS.md` §2.

### 4.4 Implicit assumption: "the issue being open proves the bug" — **it does not**

The issue being open is not evidence. The bug was established by reading the
source at a pinned commit and by executing it. Conversely, issue #1226 being
*closed* did not mean the family was fixed: it fixed a different path.

### 4.5 Implicit assumption: "the obvious fix is a wrapper or a new client" — **neither**

The brief invited a standalone program or a Python wrapper. Neither is appropriate:
the value is already fetched and normalised inside the process, so a wrapper would
add a second source of truth for a fact the code already has. The correct change is
local and inside the existing function, reusing an abstraction the repository
already provides. This is recorded in `FEASIBILITY.md` §4.

---

## 5. The catalog: what the published volumes actually look like

Measured, not quoted. Script: `research/fetch_volume_metadata.mjs`, output in
`research/raw_metadata/`.

Two document generations exist:

**Legacy (`meta.json`).** Flat, with a top-level `voxelsize` in micrometres:

```json
{ "height":7654, "slices":14830, "type":"vol", "voxelsize":7.91, "width":8096, "format":"zarr", ... }
```

**Modern (`metadata.json`).** Top-level keys `["masking","scan","zarr_export"]`,
**no** `voxelsize` anywhere the old reader looks. The resolution is a millimetre
value inside an ESRF/BM18 acquisition record:

```json
{ "scan": { "tomo": { "acquisition": { "detector": { "samplePixelSize": 0.00864 } } } }, ... }
```

`0.00864 mm × 1000 = 8.64 µm`, which matches the volume's own name
(`…-8.640um-…`) — an independent confirmation that this is the intended field and
the intended unit, and one that does not depend on trusting the code.

Four volumes probed, three of them modern. The legacy volume (`PHerc0172`,
`voxelsize: 7.91`) is the one `core/test/test_volume_live_s3.cpp` pins — the
structural reason the defect stayed invisible.

`PHerc0172` is also the volume used in villa#1226's survey comment; that comment
reports 56 of 71 catalog volumes as reachable only through the gated
`samplePixelSize` fallback. **That survey is cited as corroboration with
attribution, not reproduced here** — four volumes is not seventy-one.

---

## 6. What this reconnaissance changed

| Original plan | What the evidence supported |
|---|---|
| Investigate two programs | Investigate one — `vc_grow_seg_from_seed` is already fixed |
| Possibly build a standalone tool | A contained patch to the existing function, reusing `resolveLocalStoreVoxelSize` |
| The renderer's `1.0` fallback is the issue | The fallback is one of five interacting defects, including a unit mismatch that makes output ×1000 wrong even on the path that "works" |
| Investigate the two review concerns on #1417 | Reproduce them as executable tests: one confirmed and shown reachable, one confirmed and shown worse than described |
| Verify against real data | Done for metadata resolution against the live bucket; **not** done for a full render, because the renderer cannot be built here |
