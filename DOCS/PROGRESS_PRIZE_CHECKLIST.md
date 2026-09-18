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

**Participation requirement, quoted — and it is *not* a Progress Prize rule.**
*(Corrected 2026-09-18. This file previously presented the sentence below as a
Progress Prize requirement. On re-reading the live page, it sits under the **2027
Grand Prize**'s "Additional terms", while the Progress Prizes section states no
registration requirement and the Progress Prizes form asks only, optionally, for a
Discord display name. It is kept here because registering can only help and the
form does ask.)*

> *"To qualify, you must have registered on the [Vesuvius Challenge Discord] at the
> time of the submission."* — 2027 Grand Prize, Additional terms

**How to submit:** a Google form, titled for the month it belongs to. The Progress
Prizes form read on 2026-09-18 is
<https://docs.google.com/forms/d/e/1FAIpQLScNBMj25FMnphngRG1Ciryv_2_Mkdq2YPJOD9WqPfZExII2iQ/viewform>.
Its six required fields, the one optional field, and the Terms text it embeds are
transcribed in `PRIZE_REQUIREMENTS.md` §2.4-2.5, and each is mapped to prepared
content in §4.1 below. (Prize award is at the sole discretion of Scroll Prize, Inc.;
read the full Terms on the page or in the form rather than relying on this summary.)

---

## 3. What only you can confirm

These are the items I cannot check for you:

- [ ] **Discord registration.** The rule is verbatim above.
      **User-provided declaration, 2026-09-18: registered on the Vesuvius Challenge
      Discord; username and display name supplied.** This is recorded as the user's
      own statement — **not independently verified**, because nothing in this
      project can see a Discord account. The box stays unticked for that reason, not
      because the answer is unknown. The username is deliberately **not** written
      here or anywhere else in this repository, and must not appear in the PR or in
      the technical documents; the submission form is where it belongs, if the form
      asks for it at all.
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
| Demonstration of use on real data | `DOCS/CI_VALIDATION.md`; `DOCS/evidence/before-after.png` and `DOCS/evidence/terminal-before-after.png`; reference run <https://github.com/BioMarco/VoxelScaleGuard/actions/runs/35249590299> (the run the figures were built from) and verification run <https://github.com/BioMarco/VoxelScaleGuard/actions/runs/35374993168> (commit `953d5f6`, 2026-09-18) |
| Metrics / before-after evidence | `DOCS/RESULTS.md` §9 (compiled and run), §10 (the unit regression and its fix), §11 (the pre-PR review), §12 (the licence and documentation defects); `CI_VALIDATION.md` §7 |
| Documentation and usage examples | `README.md`; `DOCS/INDEX.md` (reading order); `harness/` builds and runs in minutes |
| Reproduction, no paid services | `.github/workflows/renderer-validation.yml` — public apt packages on a free runner, no private registry |
| Standard formats | OME-Zarr `.zattrs` and TIFF resolution tags are exactly what the patch corrects; the outputs stay in those formats |
| Open source, under the page's licence wording | repository is public; **the patch derives from `volume-cartographer/`, which is GPL-3.0-or-later, not MIT** — so this row is an *open question*, not a satisfied one. See §2.3 of `PRIZE_REQUIREMENTS.md` and `LICENSING_PROPOSAL.md` §4 |
| Advantages over existing solutions | `DOCS/FEASIBILITY.md` §7 — the comparison with #1417 and the other lapsed attempts |
| The artifact itself | `patch/vc_render_tifxyz.patch`; the fork branch <https://github.com/BioMarco/villa/tree/fix/render-voxel-size-from-open-volume> |

**Worth deciding before sending:** whether to state the PR status plainly. It is
currently *prepared and pushed, not opened*. A submission that says so is stronger
than one a reviewer can catch in a minute. If you open the PR first, the statement
becomes "PR #NNNN open".

### 4.1 Field-by-field: what goes in each box

The form read on 2026-09-18 has six required fields, one optional field and one
consent checkbox (`PRIZE_REQUIREMENTS.md` §2.4). Every one of them already has
prepared content, so nothing has to be written from scratch at submission time.

| Form field | What to put in it | Source |
|---|---|---|
| `Email` | your own address | only you have it |
| `Your full name` | your own name | only you have it |
| `Team description — individual or team…` | one line: submitting as an individual | — |
| `If you are a member of our Discord server, what is your display name there?` **(optional)** | your Discord display name, if you want to give it. The field is optional and conditional, so leaving it blank is allowed. **Do not put it in the repository or the PR** | your own statement, §3 |
| `URL of your open source / publicly available contribution…` | `https://github.com/BioMarco/VoxelScaleGuard` — add the fork branch `https://github.com/BioMarco/villa/tree/fix/render-voxel-size-from-open-volume` as a second URL, and either run `https://github.com/BioMarco/VoxelScaleGuard/actions/runs/35249590299` or `…/35374993168` as a third if the field allows more than one | `SUBMISSION_DRAFT.md` §How to reproduce |
| `What is your contribution? (1)…(4)` | paste the four answers under **"Answers to the submission form's four questions"** in `SUBMISSION_DRAFT.md`, in the form's own order | `SUBMISSION_DRAFT.md`, that section |
| `Yes, I agree` (Terms) | tick, having read the embedded Terms — they are transcribed in `PRIZE_REQUIREMENTS.md` §2.5 | — |

Two things the form does **not** have, so do not wait for them: no file upload, and
no place for the figures. The contribution URL is what carries the evidence, which is
why the repository has to be readable before the form is sent.

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

Nothing below has been done. Steps 1 and 5 need your explicit authorisation; step 2
is a decision only you can make.

0. **Decide the licence and attribution question** — `LICENSING_PROPOSAL.md` §8. The
   third-party Open Data attribution (its §4) is required by the data's own terms
   whether or not a prize is ever claimed, so it is the one item here that is not
   optional. This does not block submitting: the form's Terms make the licence a
   condition of *accepting* a prize, not of entry (`PRIZE_REQUIREMENTS.md` §2.5).
1. Open the PR against villa (needs your comment and your checkbox tick — see
   `PR_DRAFT.md` Part 2). Maintainer feedback before a prize submission is only an
   advantage. **This is the item `README.md` lists as still open, and it has not been
   authorised.**
2. Note your Discord registration. It is **not** a Progress Prize requirement (that
   correction is in §2 above); the form only asks, optionally, for a display name.
3. Re-read <https://scrollprize.org/prizes> for the current deadline before
   submitting; do not rely on the date in this file. The form is recreated per month
   and its title names the month, so open it from the page rather than from a saved
   link.
4. Fill in the submission form using §4.1 field by field. There is no upload field:
   the contribution URL carries the evidence, so the repository must be readable and
   its links must resolve **before** the form is sent.
5. Only then, if you want, consider the follow-up work that would strengthen a
   later entry: the VC3D GUI predicate (`FEASIBILITY.md` §8), and rendering more
   volumes.
