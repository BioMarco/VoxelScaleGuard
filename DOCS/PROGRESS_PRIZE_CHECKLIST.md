# PROGRESS PRIZE — submission checklist

**Nothing has been submitted. No form has been filled in. This is a checklist, not a
claim.**

Two kinds of statement appear below and they must not be blurred:

* **[live]** — read from the official page on a stated date. Re-check before acting,
  because the page can change and it is the authority, not this file.
* **only you can attest** — a personal or account fact that no amount of fetching can
  establish for me. I have not ticked these and will not.

---

## 1. The state of things, kept separate on purpose

| Stage | State |
|---|---|
| Contribution ready to propose to villa maintainers | **Yes** — patch, tests, evidence, PR text |
| Pull request **open** against `ScrollPrize/villa` | **No** |
| Pull request **accepted** / merged | **No** |
| Progress Prize application **sent** | **No** |
| Prize **won** | **No** |

This matters for the submission, because a Progress Prize entry is judged on what
exists at the moment it is sent. Anything phrased as "will be merged" or "is being
adopted" would be an unverified claim.

---

## 2. What the official page says [live]

Re-checked **2026-09-18** at <https://scrollprize.org/prizes>. Verbatim where quoted.

**Deadline.** *"Submissions are evaluated monthly, and multiple submissions/awards
per month are permitted. The next deadline is 11:59pm Pacific, September 30th,
2026!"* — that is **12 days** from the re-check date. No later deadline is stated;
if a month is missed, the evaluation recurs monthly.

**Award structure.** *"Best Submission of the Month: $20,000, guaranteed every
month, to the single best submission — selected by the Vesuvius Challenge team."*
Beyond that, typically `$20,000, $10,000, $5,000, $2,500, $1,000, $500 or $250`.

**What it favours** (abridged quotes that bear on this work):

* *"Are released or open-sourced early."*
* *"Resolve outstanding bugs in tools that people are using, and that you are using
  yourself, evidenced by before/after screenshots, logs, etc."*
* *"Improve results quantitatively and/or qualitatively on real data."*
* *"Are well documented."*

**Core requirements** (the page's own headings):

1. *Problem Identification and Solution* — address a specific challenge using
   Vesuvius Challenge scroll data; provide a clear implementation path and a
   demonstration of its use; demonstrate significant advantages over existing
   solutions.
2. *Documentation* — comprehensive documentation and usage examples.
3. *Technical Integration* — accept standard community formats (OME-Zarr / Zarr,
   tifxyz quadmeshes, triangular meshes), maintain consistent output formats, be
   designed for modular integration.

**Participation requirement, quoted:** *"To qualify, you must have registered on the
[Vesuvius Challenge Discord] at the time of the submission."*

**How to submit:** the page links a Google form —
<https://docs.google.com/forms/d/e/1FAIpQLScNBMj25FMnphngRG1Ciryv_2_Mkdq2YPJOD9WqPfZExII2iQ/viewform>.
(Prize award is at the sole discretion of Scroll Prize, Inc.; the page also carries
full Terms and Conditions. Read them there rather than relying on this summary.)

---

## 3. What only you can confirm

These are the items I cannot check for you:

- [ ] **Discord registration.** The rule is verbatim above. Whether you are
      registered is not something I can see.
- [ ] **Whether you want to submit at all**, and in which month. The evaluation is
      monthly, so waiting costs a month but not the contribution.
- [ ] **The submission form's own fields** — name, contact, and any attestation it
      asks for. Fill them yourself.
- [ ] **That the story in the submission is yours.** The page favours work that
      *"actually gets used"* and bugs found by people using the tools. Say honestly
      how this arose; the defect was reported by others (see §5) and this project's
      verification is what is new.
- [ ] **Whether the repository's public face is right for reviewers**, since they
      will read it: <https://github.com/BioMarco/VoxelScaleGuard>.

---

## 4. Materials to send

Everything below already exists. Nothing needs to be written from scratch.

| What the page asks for | Where it is |
|---|---|
| Problem identification, implementation path | `DOCS/ROOT_CAUSE_ANALYSIS.md` (cause at file:line); `DOCS/ARCHITECTURE.md` (the fix's shape and the alternatives rejected) |
| Demonstration of use on real data | `DOCS/CI_VALIDATION.md`; `DOCS/evidence/before-after.png`; run <https://github.com/BioMarco/VoxelScaleGuard/actions/runs/35249590299> |
| Metrics / before-after evidence | `DOCS/RESULTS.md` §9 (compiled and run), §10 (the unit regression and its fix), §11 (the pre-PR review); `CI_VALIDATION.md` §7 |
| Documentation and usage examples | `README.md`; `DOCS/INDEX.md` (reading order); `harness/` builds and runs in minutes |
| Reproduction, no paid services | `.github/workflows/renderer-validation.yml` — public apt packages on a free runner, no private registry |
| Standard formats | OME-Zarr `.zattrs` and TIFF resolution tags are exactly what the patch corrects; the outputs stay in those formats |
| Open source, permissive | repository is public; the patch derives from `villa`, which is **MIT** |
| Advantages over existing solutions | `DOCS/FEASIBILITY.md` §7 — the comparison with #1417 and the other lapsed attempts |
| The artifact itself | `patch/vc_render_tifxyz.patch`; the fork branch <https://github.com/BioMarco/villa/tree/fix/render-voxel-size-from-open-volume> |

**Worth deciding before sending:** whether to state the PR status plainly. It is
currently *prepared and pushed, not opened*. A submission that says so is stronger
than one a reviewer can catch in a minute. If you open the PR first, the statement
becomes "PR #NNNN open".

---

## 5. Honest framing to keep

The page rewards *"bugs in tools that people are using"*. Credit for finding and
reporting belongs to others, and the submission should say so plainly:

* **NicolasHuberty**, PR #1417 — the approach of consulting the volume's remote
  voxel size; lapsed to an inactivity bot rather than being rejected;
* **Bullo27** and the villa maintainers — PRs #1227, #1229, #1454: the
  `samplePixelSize` schema handling, `resolveLocalStoreVoxelSize`, and
  `Volume::voxelSize()` semantics;
* **Bullo27**, PR #1228 — the VC3D enable-predicate diagnosis;
* **DarthCeltic** #1403 and **Bullo27** #1226 — the issue reports.

What is new here is the verification and the correction: the defect reproduced on
real catalog data, a patch that compiles and runs, before/after on two published
volumes with pixel equality, and a test suite. The `vc_grow_seg_from_seed` half of
#1403 is **already fixed upstream** and is not this project's work.

Also worth stating for credibility, because a reviewer will find it anyway: the
first version of this patch **did not compile**, and the second wrote a wrong unit
on one path. Both are recorded in `RESULTS.md` §9.1 and §10. Presenting the work
without them would not survive reproduction.

---

## 6. Suggested order

1. Open the PR against villa (needs your comment and your checkbox tick — see
   `PR_DRAFT.md` Part 2). Maintainer feedback before a prize submission is only an
   advantage.
2. Confirm your Discord registration.
3. Re-read <https://scrollprize.org/prizes> for the current deadline before
   submitting; do not rely on the date in this file.
4. Fill in the submission form, attaching or linking the materials in §4.
5. Only then, if you want, consider the follow-up work that would strengthen a
   later entry: the VC3D GUI predicate (`FEASIBILITY.md` §8), and rendering more
   volumes.
