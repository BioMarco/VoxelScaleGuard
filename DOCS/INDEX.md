# Documentation index

All project documentation lives in `DOCS/`. The repository root holds only
`README.md` (the landing page) and `AGENTS.md` (operating rules for contributors
and agents).

## Read in this order

**Resuming work in a new session? Start at [`RESUME.md`](RESUME.md)** — it has the
state verification commands, the next step with acceptance criteria, the decisions
already made, and the environment traps.

| # | Document | Why |
|---|---|---|
| 0 | [`RESUME.md`](RESUME.md) | Handoff: verify the inherited state, then continue from the next step |
| 1 | [`PROJECT_STATUS.md`](PROJECT_STATUS.md) | Current state, the gaps, the environment traps, the activity log |
| 2 | [`ROOT_CAUSE_ANALYSIS.md`](ROOT_CAUSE_ANALYSIS.md) | The cause, at file-and-line resolution, with every claim tagged by how it was established |
| 3 | [`RESULTS.md`](RESULTS.md) | Everything actually executed — **and §7, everything that was not** |
| 4 | [`FEASIBILITY.md`](FEASIBILITY.md) | The GO decision: why this is worth doing, what it costs, what was left out |
| 5 | [`ARCHITECTURE.md`](ARCHITECTURE.md) | The fix's design, and the alternatives that were rejected on evidence |
| 6 | [`TEST_PLAN.md`](TEST_PLAN.md) | Executed vs. blocked, and what would falsify each claim |

## Reference

| Document | Contents |
|---|---|
| [`RESEARCH.md`](RESEARCH.md) | Sources, exact commits and licences; the architecture of the Villa monorepo; and §4, the assumptions in the original brief that did not survive contact with the source |
| [`PRIZE_REQUIREMENTS.md`](PRIZE_REQUIREMENTS.md) | The Progress Prize rules as published, separated from what this project merely considers useful; and where this project stands against each criterion |
| [`PR_DRAFT.md`](PR_DRAFT.md) | Pull request draft. **Not submitted.** |
| [`SUBMISSION_DRAFT.md`](SUBMISSION_DRAFT.md) | Progress Prize submission draft. **Not submitted.** |

## Evidence and artefacts

| Path | What it is |
|---|---|
| `../patch/vc_render_tifxyz.patch` | The proposed fix: one file, +177/−65, against `villa` @ `757f70c` |
| `../harness/` | The reproducer. Compiles the pinned revision's real translation units, plus a verbatim copy of the pre-patch reader, plus tests |
| `../harness/tests/test_render_voxel_size.cpp` | 16 cases / 82 assertions, including five that reproduce the defect |
| `../harness/tools/probe_render_voxel_size.cpp` | The before/after demonstrator over the real published documents |
| `../research/fetch_volume_metadata.mjs` | Read-only live catalog probe |
| `../research/raw_metadata/` | The raw published documents it fetched |

## The two-minute summary

`vc_render_tifxyz` writes the physical voxel size into OME-Zarr axis metadata and
TIFF resolution tags. It obtains that number from a **local** file, using a reader
that recognises only a top-level `voxelsize` key — while the process has *already
fetched the correct value over the network* and is holding it. For three of four
published volumes probed here it falls back to a scale of `1.0`, declared as
**nanometres**, so the declared physical voxel size is wrong by **×2400, ×8640 or
×45532**. The rendered pixels are correct; every physical number attached to them
is not.

The one volume the old reader gets right is the legacy-shaped one that the
repository's only live-S3 test happens to pin — which is why this survived.

**The patched binary has never been compiled or run.** That is the single largest
gap and it is stated in the first line of `RESULTS.md`, in §7 of that document,
and in `PROJECT_STATUS.md` §6.
