# FEASIBILITY

**Decision: GO**, for the `vc_render_tifxyz` voxel-size resolution fix, with the
scope stated below and one item deliberately left outside it.

This document records the assessment *and* the parts of it that were wrong at
first, because the second is what changed the shape of the contribution.

---

## 1. Does the problem exist in the current code?

**Yes.** Established by reading the pinned revision and by executing the real
upstream code; see `ROOT_CAUSE_ANALYSIS.md` §§3–5.

Summary of what is live at `757f70c0140a4cfbbbd44975ef09558444b96980`:

| Root cause | Live? | How established |
|---|---|---|
| A — no remote path; the correct value is already held in `remoteVolume` | Yes | **[read]** `:986` uses `std::filesystem::exists`; `:1315` opens the remote `Volume` |
| B — hand-rolled reader misses the `samplePixelSize` schema the rest of VC3D handles | Yes | **[live] [exec]** resolved 1 of 4 real published volumes |
| C — reader validates type, not value (`0` and `-3` returned as sizes) | Yes | **[exec]** |
| D — placeholder `1.0` emitted as physical scale, declared in nanometres | Yes | **[exec] [live]** declared size wrong by ×2400…×45532 on real stores |

The seeding half of villa#1403 is **not** live; it is already fixed. That
removed half the originally-assumed work.

## 2. Can it be reproduced?

**Yes, for the resolution logic — and that is the part that is broken.**
**[exec]** The harness runs:

* the real `VoxelSizeMetadata.cpp`, `RemoteUrl.cpp` and `Json.cpp` from the
  pinned commit, compiled unmodified;
* upstream's own test file for the resolver, unmodified — 13 cases, 54
  assertions, all passing, which is the control that says the harness is
  executing their code and not a paraphrase;
* a byte-verbatim copy of the renderer's `readVolumeVoxelSize`, so the pre-patch
  decision procedure is executable rather than described;
* the real metadata documents fetched from the public bucket.

**No, not end-to-end in this environment.** The renderer cannot be built here:
Qt, OpenCV, Ceres, CGAL and libtiff are absent, and the `windows-msvc` preset
builds that closure from source through vcpkg. That is the single largest gap and
it is recorded as such in `RESULTS.md` §7.

What that gap does and does not invalidate:

* It does **not** affect the finding. The defect is in a pure function over a JSON
  document; that function was executed.
* It **does** mean the `.zattrs` file and the TIFF tags were never produced by the
  patched renderer, and no render on a real volume was run. Those remain
  unverified.

## 3. Are fixes already available?

**Partially — and this is the crux.**

* **villa#1226 is closed as completed.** Its fix repaired *Volume construction*;
  it does not touch the renderer's own reader. Not a fix for this.
* **PR #1417** (`vc_render_tifxyz: discover remote volume voxel size`,
  `NicolasHuberty`, head `459ae44406`) addresses the same defect. **Closed
  unmerged on 2026-08-27 by an inactivity bot after 14 days**, not rejected:
  `"Closed automatically under the repository PR time limits because this PR has
  had no activity for 14 days."` Its review found no defects.
* **PR #1228** (VC3D side, `isRemote()` in the enable predicate) **closed
  unmerged 2026-08-09**. PR #1313 (`--voxel-unit` default) **closed unmerged
  2026-08-09**. PR #1541 (nested `samplePixelSize` search) **closed unmerged
  2026-09-08**.

So the honest position is: **the problem is fixed nowhere on `main`, and three
independent attempts at parts of it have lapsed unfixed.** That is the strongest
argument that a further contribution is worthwhile, and simultaneously the
strongest warning: a contribution that ignores *why* those lapsed will lapse too.
The stated reason is process (inactivity), not technical objection — so the
lesson taken here is to make the change small, testable and self-evidently safe,
and to attach the evidence.

## 4. Is the solution compatible?

**Yes, and the patch was designed around that constraint.**

* No public interface changes. `writeZarrAttrs`'s signature is untouched; the
  patch changes only what the caller passes.
* No new dependency. The patch uses `vc::metadata::resolveLocalStoreVoxelSize`,
  which already exists in `core`, is already used by
  `vc_grow_seg_from_segments.cpp:1165`, and is already covered by
  `core/test/test_voxel_size_metadata.cpp`.
* No new metadata convention. Nothing is added to or redefined in any store
  document.
* No behaviour change to the rendered pixels. `base_voxel_size` feeds only
  `tifDpi` and the OME-Zarr axis scale (`ROOT_CAUSE_ANALYSIS.md` §6), so surface
  growth, sampling and image content are untouched. This is what makes the change
  low-risk: the worst realistic regression is a *different declared scale*, which
  is the thing being corrected.
* The private `readVolumeVoxelSize` is deleted rather than kept beside the shared
  resolver, because keeping it is how the two drifted apart in the first place.

**One behaviour change is deliberate and must be stated:** when the physical size
cannot be resolved, the tool now leaves the OME-Zarr axis unit off (and sets
`dpi = 0`, as before) instead of asserting `1.0 nanometer`. Anything that
currently reads a `nanometer` unit out of a `vc_render_tifxyz` output on such a
volume would stop finding it. No in-tree consumer does (`ROOT_CAUSE_ANALYSIS.md`
§6). The rendered image is unaffected.

**One behaviour change is *avoided*:** `--voxel-unit` keeps its existing
`"nanometer"` default. Changing it is the fix PR #1313 proposed and it is
probably right for the long term, but it is a separate change with a separate
blast radius, so the patch instead makes the emitted unit follow the *source* of
the number. That leaves CLI-only users byte-identical while making both metadata
paths correct.

## 5. Resources needed

| Item | Available here | Note |
|---|---|---|
| C++ compiler | **Yes** — MSVC 14.34.31933 | VS 2022 Community, discovered on the machine |
| Windows SDK | **Yes** — 10.0.22000.0 | |
| CMake / Ninja | Present but unusable | CMake 3.24 cannot launch subprocesses in this sandbox; the harness invokes `cl.exe` directly. Upstream needs 3.28, which is **not** present |
| Qt, OpenCV, Ceres, CGAL, libtiff, boost | **No** | needed to build `vc_render_tifxyz` |
| vcpkg | **No** | `VCPKG_ROOT` unset; no checkout anywhere on the machine |
| Python | present (3.13) | not needed for this patch |
| GPU | not needed | the fix is CPU metadata handling |
| Network | yes, via Node's fetch | used only for read-only GETs of public metadata |
| Data downloaded | **~12 KB** | four published `metadata.json`/`meta.json` documents. Far below the 2 GB limit |

**To close the remaining gap** would require the `windows-msvc` preset's
dependency closure through vcpkg: a bootstrap plus Qt/OpenCV/Ceres/CGAL built
from source. That is a multi-GB download and a long build. **Not started, not
authorised, and not attempted.**

## 6. Can the improvement be demonstrated?

**Yes, for the resolution logic, with real data** — which is what the Progress
Prize criteria ask for ("Improve results quantitatively… on real data", "Resolve
outstanding bugs in tools that people are using… evidenced by before/after
screenshots, logs").

**[exec]** The probe runs the pre-patch reader and the patched resolver over the
four real published documents and prints the declared physical scale each would
produce. Three of four diverge; the fourth is a control that confirms the
pre-patch reader was not simply broken everywhere. Transcript in `RESULTS.md`.

**[exec]** 19 test cases / 107 assertions covering every case the brief lists:
local valid, remote valid, missing metadata, zero, negative, non-finite, explicit
size, native resolution, downsampled resolution, a surface genuinely below
threshold (via the unchanged `min_area_cm` path, untouched by this patch),
rendering with a verified scale, rendering with metadata unavailable.

**[exec] Demonstrated on 2026-09-16: an actual render, actual `.zattrs`, actual
TIFF tags.** Both binaries were compiled in CI from the pinned revision and run
against `PHerc0009B` (and `PHerc0172` as control). The patched renderer declared
`micrometer` / `[8.64, 8.64, 8.64]` where the baseline declared `nanometer` /
`[1, 1, 1]`, wrote `XResolution = 2939.8147` px/inch where the baseline wrote no
resolution tag, and produced **byte-identical decoded pixels**. Full record in
`CI_VALIDATION.md`.

**Also established, and it is the more useful result:** the patch as first
committed **did not compile**. See `RESULTS.md` §9.1.

**Still not demonstrated:** broader coverage — two volumes, one crop each, one
slice. This is a demonstration, not a survey, and the "no usable voxel size
anywhere" branch is covered by unit tests only (driving it end to end would mean
severing a volume from its own metadata).

## 6b. What changed about the environment story

`§5` above describes the local machine, and is still accurate. What changed is
that the fix no longer depends on it: the validation runs on GitHub-hosted
runners, using the package list upstream's own `install_build_deps.sh` draws from
the public Ubuntu archive and nothing private. Neither of upstream's prebuilt
dependency bundles could be used — an anonymous `ghcr.io` token request for
`scrollprize/vc3d-deps` returns **HTTP 403** for both the Windows and Linux tags —
so the workflow installs dependencies itself rather than relying on a registry
this project has no access to.

## 7. Difference from existing contributions

| | PR #1417 (lapsed) | This patch |
|---|---|---|
| Mechanism | new public `remoteVolumeVoxelSize()` that re-fetches the store document | reads `Volume::voxelSize()` from the volume already opened |
| Extra network requests | one or two per run | **zero** |
| Can disagree with the rendered volume | possible (separate fetch, separate parse) | **no** — same object |
| URL fragment hazard raised in its review | present in that design | **unreachable by construction** |
| Local-store schema gap (`samplePixelSize`) | not addressed | fixed by reusing the shared resolver |
| Invalid local value (`0`, `-3`) | noted in review, unfixed | fixed; origin tracked |
| Unit mismatch between number and declared unit | not addressed | fixed (value-source-derived unit) |
| GUI enable predicate (`baseScaleLevel() > 0 \|\| …`) | not addressed | diagnosed; not applied (§8) |
| Tests | none added | 27 cases, 208 assertions, plus upstream's suite as a control |
| Binary-verified | no | **yes** — compiled and run on real published volumes (`CI_VALIDATION.md`) |

The distinction that matters is not line count. #1417 fetched the metadata a
second time and so inherited a hazard the reviewer had to flag. This patch
removes the need for a second fetch, which removes the hazard instead of guarding
it.

**Attribution.** The approach of "make the renderer consult the volume's remote
voxel size" is `NicolasHuberty`'s from PR #1417. The `samplePixelSize` schema
work, `resolveLocalStoreVoxelSize`, and the `Volume::voxelSize()` semantics are
`Bullo27`'s and the villa maintainers' (PRs #1227, #1229, #1454 and the
`VoxelSizeMetadata` module). The GUI enable-predicate diagnosis is `Bullo27`'s
from PR #1228. This contribution is the consolidation of those into one path,
plus the verification that the two lapsed alternatives left open.

## 8. Deliberately out of scope

1. **The VC3D enabling condition**
   (`SegmentationCommandHandler.cpp:2076-2078`). Changing
   `baseScaleLevel() > 0 || hasExplicitVoxelSizeOverride()` to "we have a positive
   voxel size" is a two-line change and is, on the evidence, correct. It is
   *not* applied, because it changes GUI behaviour and the pass/fail semantics of
   a flag whose history includes a rejected PR. It is documented, with the exact
   edit, so a maintainer can decide. Recommended as a follow-up commit.
2. **`--voxel-unit` default → `micrometer`.** Correct in isolation, separate blast
   radius, separately rejected once (#1313). Not included.
3. **`vc_zarr_to_tiff.cpp:69-86`**, which reads only a local `meta.json` and only
   a top-level `voxelsize`, so it has the same schema gap as root cause B (though
   it has no remote path and so no root cause A). Same class of defect, not
   needed for the reported problem. Documented as a candidate, not developed.
4. **The `core/test/test_volume_live_s3.cpp` pin.** Pointing that live test at a
   modern `metadata.json`-shaped volume would have caught this. Changing an
   existing live test's fixture is a maintainer decision, not ours.

## 9. Decision

**GO — and the condition has been met.**

The problem is present, the cause is established at file-and-line resolution, the
fix is small and reuses an existing shared abstraction, the improvement is
demonstrated on real published data with a working control, and the two lapsed
alternatives are weaker in a specific, checkable way (an extra network fetch that
can disagree with the volume, and a fragment hazard it must defend against).

**The one condition — compiling and running the patched `vc_render_tifxyz` — was
met on 2026-09-16**, on GitHub-hosted runners rather than through the vcpkg closure
this section originally anticipated. The patch is therefore **compiled and
executed**, and the earlier instruction to describe it as "logic-verified, not
binary-verified" no longer applies. See `CI_VALIDATION.md`.

What it is *not* is finished: the GUI path is still unchanged, coverage is two
volumes and one crop, and the patch as first committed did not compile at all
(`RESULTS.md` §9.1). "GO" here means the contribution is real and demonstrable, not
that it is complete.
