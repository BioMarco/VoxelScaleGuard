# PRIZE_REQUIREMENTS

Rules read from `https://scrollprize.org/prizes` during this session. This file
separates **what the rules officially require** from **what this project thinks is
useful**, as the brief asked.

> Prize terms can change. Scroll Prize, Inc. reserves the right to modify them, and
> the final citation for anything below is the live page, not this document.

---

## 1. The relevant prize: Progress Prizes

The 2027 Grand Prize and the First Letters / Title prizes are awarded for
*results* (a fully unrolled readable scroll; ten letters in 4 cm²). This project
is an engineering contribution, so the applicable category is **Progress Prizes**:

> *"In addition to milestone-based prizes, we offer monthly prizes for open source
> contributions that help read the scrolls."*

* **Best Submission of the Month: $20,000**, guaranteed every month, to the single
  best submission, selected by the Vesuvius Challenge team.
* Additional awards "based on the significance of the contribution, typically
  $20,000, $10,000, $5,000, $2,500, $1,000, $500 or $250."
* Evaluated **monthly**; multiple submissions and multiple awards per month
  permitted.

**This project does not promise a prize.** Prize award is at the sole discretion
of Scroll Prize, Inc., and no submission has been made.

### 1.1 Deadline

The page states: *"The next deadline is 11:59pm Pacific, September 30th, 2026."*

**Re-checked [live] on 2026-09-18** against <https://scrollprize.org/prizes>: the
sentence above is still verbatim current, the Grand Prize deadlines on the same page
read June 25th 2027, and the Progress Prize deadline is **12 days away, not past**.
This section previously claimed the date "has passed as of this work" and that the
next deadline was unconfirmed. **That was a documentation error, not a change on the
page**: the page has not moved, the reading of it was wrong. It is corrected here and
recorded in `RESULTS.md` §8.

An earlier entry in this file implied the work was done after 2026-09-30. No such
date is asserted anywhere now; the date of this correction is **2026-09-16**, and
every "today"/"now" in this document set means the latest verification date stated
in the section at hand.

---

## 2. What the rules officially require of a Progress Prize submission

### 2.1 The qualifying character of the contribution (verbatim, abridged)

The page lists what it favours. The four that bear on this project:

> *"Are **released or open-sourced early**."*

> *"Improve results quantitatively and/or qualitatively on **real data**."*

> *"Resolve outstanding **bugs in tools that people are using**, and that you are
> using yourself, **evidenced by before/after screenshots, logs, etc.**"*

> *"Are **well documented**. It helps a lot if relevant documentation, walkthroughs,
> images, tutorials or similar are included with the work so that others can use
> it!"*

### 2.2 Submission criteria (the "Core Requirements", verbatim headings)

1. **Problem Identification and Solution**
   * Address a specific challenge using Vesuvius Challenge scroll data
   * Provide clear implementation path and a demonstration of its use
   * Demonstrate significant advantages over existing solutions
2. **Documentation**
   * Include comprehensive documentation
   * Provide usage examples
3. **Technical Integration**
   * Accept standard community formats (e.g. OME-Zarr or Zarr arrays, tifxyz
     quadmeshes, triangular meshes)
   * Maintain consistent output formats
   * Designed for modular integration

### 2.3 Licensing

Prizes are conditioned on open-sourcing: *"You agree to make your method open
source if you win a prize… you have to make it open source under a permissive
license to accept the prize."*

**This section was wrong until 2026-09-18 and the error is load-bearing.** It read:
*"`villa` itself is MIT (Copyright (c) 2024 Vesuvius Challenge), so a patch under the
same terms satisfies this."* `villa`'s **root** is MIT, but the three files the
patch modifies live under `villa/volume-cartographer/`, which is
**GPL-3.0-or-later**, Copyright (C) 2023 EduceLab
(`volume-cartographer/LICENSE` + `NOTICE`; corroborated by
`volume-cartographer/Dockerfile:10`). A patch against GPL-3.0-or-later code is a
derivative of it, and cannot be relicensed MIT. The conclusion therefore does not
follow from the premise, and the premise was false.

What *is* true: this repository's **own** work — the investigation, harness, CI,
evidence and documentation, which is the larger and more original part of the
submission — can be licensed permissively without difficulty. The conflict is
narrow and specific: the prize text names *"an open source license (e.g. MIT)"* in
one place and *"a permissive license"* in another, and GPL-3.0-or-later is open
source by the OSI definition but **not** permissive. That is a question for the
organisers, not one to resolve in this document. Full inventory and the two
defensible framings: `LICENSING_PROPOSAL.md` §1 and §4.

**State of play on 2026-09-18.** The licence split itself is no longer open: this
repository's original work is MIT and the derived material is GPL-3.0-or-later, as
mapped path by path in `NOTICE.md`. What remains open is **only** whether that
satisfies the organisers' wording, and the question is drafted and awaiting the
author's decision to send it — `PROGRESS_PRIZE_QUESTION.md`. Nothing has been sent
to the organisers, and this document does not assert that the submission is or is
not eligible under that condition.

Because the condition attaches to *accepting* a prize rather than to submitting
(§2.5), an open answer here does not by itself block an entry.

### 2.4 How submissions are made, and the form's actual fields

Via the linked Google Form, not by email (the email route is for Grand
Prize-class results). **No submission has been made and none will be without
authorisation.**

The form was fetched and read on 2026-09-18. Its title confirms which month the
entry belongs to, and it has **six required fields** — no more, and no upload:

| # | Field (verbatim) | Required |
|---|---|---|
| 1 | `Email` | yes |
| 2 | `Your full name` | yes |
| 3 | `Team description — are you submitting as an individual or as a team, and who is on your team? (If on a team, the team leader should be the one filling out this form)` | yes |
| 4 | `If you are a member of our Discord server, what is your display name there?` | **no** — the label is conditional, so the field is optional |
| 5 | `URL of your open source / publicly available contribution, e.g. GitHub repo or PR (you can add multiple)` | yes |
| 6 | `What is your contribution? Please mention (1) Which scroll data did you work on for this submission? (2) How does it substantially increase the probability of yourself or someone else reading those scrolls or others? (3) What does it enable that was not possible before? (4) What evidence have you provided for this?` | yes |

Plus one required checkbox: `Yes, I agree` to the Terms and Conditions, which are
**reproduced in full inside the form** — the same text as §2.5.

Field 6 is a four-part question, so the prepared prose in `SUBMISSION_DRAFT.md` is
mapped to it explicitly in `PROGRESS_PRIZE_CHECKLIST.md` §4.1. Field 4 is the only
place a Discord display name belongs: **it is optional, and the username is not
recorded anywhere in this repository** deliberately.

### 2.5 Terms and conditions that apply

Verbatim from the form itself (2026-09-18), which matters because it is the text the
submitter actually agrees to:

> *"You agree to make your method open source if you win a prize. It does not have to
> be open source at the time of submission, but you have to make it open source under
> a permissive license to accept the prize."*

Three consequences, stated because two earlier readings of this project got them
wrong:

* **"permissive" here is a condition of *accepting a prize*, not of submitting.**
  The submission may be closed-source at the time of entry. So the GPL-3.0-or-later
  question in §2.3 does **not** block an entry; it conditions the *award*. Note that
  this contribution is *more* open than the Terms require of a submission: it is
  public now, split and documented path by path in `NOTICE.md`.
* **The Grand Prize section states the permissive requirement directly**, not only
  in the general Terms: *"Pipeline fully reproducible and code shared under an open
  source license (e.g. MIT), published publicly on GitHub. It does not have to be
  open source at the time of submission, but you have to make it open source under a
  permissive license, publicly on GitHub, to accept the prize."*
* **Discord registration is not stated for the Progress Prizes.** The quoted
  *"To qualify, you must have registered on the Vesuvius Challenge Discord at the
  time of the submission"* is under the **2027 Grand Prize's** Additional terms.
  `PROGRESS_PRIZE_CHECKLIST.md` previously presented it as a Progress Prize rule;
  that is corrected there. Registration is still recorded as a user declaration
  because it can only help and the form asks for a display name.
* Apart from that: award at the sole discretion of Scroll Prize, Inc., more or fewer
  awards may be issued; milestone submissions close once the winner is announced
  (not applicable here); the winner must provide payment information within 30 days.

---

## 3. Officially required vs. what this project considers useful

Explicitly separated, because conflating them is how a project ends up optimising
for the wrong rubric.

### 3.1 Officially required

| Requirement | Status |
|---|---|
| Open source under a permissive licence, **if a prize is won** | **Split applied; the reading is an open question.** The original work is MIT and the derived patch is GPL-3.0-or-later by necessity (`NOTICE.md` maps it path by path; `LICENSE` states what it does not cover). What is unresolved is whether the organisers' *"permissive license"* wording accepts a GPL-3.0-or-later patch — asked in `PROGRESS_PRIZE_QUESTION.md`, **not sent**. It does **not** block submitting (§2.5) |
| Problem identification and solution, with a demonstration of its use | **Yes** — problem identified and demonstrated both at the metadata-resolution level (`RESULTS.md` §2) and from two real compiled binaries on real published volumes (`RESULTS.md` §9, `CI_VALIDATION.md`) |
| Significant advantage over existing solutions | **Demonstrated against alternatives** — `FEASIBILITY.md` §7 compares concretely with the lapsed PR #1417 |
| Comprehensive documentation and usage examples | **Yes** — this file set |
| Standard formats (OME-Zarr / Zarr / tifxyz) | **Yes** — the fix is about OME-Zarr `.zattrs` and TIFF resolution tags; no format change |
| Maintain consistent output formats | **Yes** — no interface or format change; `writeZarrAttrs`'s signature is untouched |
| Modular integration | **Yes** — reuses `vc::metadata::resolveLocalStoreVoxelSize` from `core` |
| Evaluation monthly via the form | **Pending** — the deadline is confirmed and current (11:59pm Pacific, 30 September 2026); the form's fields are recorded in §2.4; **not submitted** and will not be without authorisation |

### 3.2 Judged useful, not required

These are the project's own choices, and are flagged as such:

| Choice | Why |
|---|---|
| Fixing the defect **inside `villa`** rather than shipping a wrapper | The rules reward contributions that get *used*, and "pipeline should be seamlessly integrated in VC3D" is stated for the Grand Prize. A wrapper would add a second source of truth for a fact the process already holds |
| Adding regression tests to upstream's suite | The rubric's "maintain consistent output formats" and the repository's own `AGENTS.md` ("Tests are not optional") both point this way; a silent scale regression is otherwise invisible |
| Attaching before/after logs on real published volumes | Directly matches the "bugs in tools that people are using, evidenced by before/after … logs" criterion |
| Reporting the **negative** result that `vc_grow_seg_from_seed` is already fixed | Credibility. A submission that claims two fixes when one is upstream's is weaker on inspection |
| Documenting the unverified parts prominently | The prize is judged by a team that will try to reproduce the work |

### 3.3 Explicitly *not* required by the rules, but often assumed

* A new standalone program. The rules ask for a "clear implementation path", not a
  new binary.
* A Docker image — that is asked for the **Grand Prize** ("Please create a Docker
  image that we can easily run to reproduce your work"). For a Progress Prize it
  is at most a convenience.
* Published datasets. The CC-BY-NC-4.0 dataset-release condition attaches to
  *ML datasets and checkpoints*, not to a tooling fix. This project trains
  nothing.

---

## 4. Where this project stands against the rubric

Honest assessment, including the gaps.

| Criterion | Status | Evidence / gap |
|---|---|---|
| Address a specific challenge using scroll data | **Met** | A specific defect in `vc_render_tifxyz`, on real catalog volumes |
| Clear implementation path | **Met** | `ARCHITECTURE.md`, `patch/vc_render_tifxyz.patch` |
| Demonstration of its use | **Partial** | Resolution demonstrated on 4 real published volumes; **no render was run** (`RESULTS.md` §7) |
| Significant advantage over existing solutions | **Partial** | Concretely stronger than the lapsed PR #1417 (`FEASIBILITY.md` §7); but "existing solutions" also includes an unresolved build gap |
| Comprehensive documentation | **Met** | This file set |
| Usage examples | **Met for the harness**; **missing for the patched tool** | `README.md`; a real render invocation is untested |
| Standard formats, consistent output | **Met** | No format or interface change |
| Modular integration | **Met** | Reuses `core`'s shared resolver |
| Well documented, with walkthroughs/images | **Documented; no images** | No before/after screenshots, because no render was produced |
| Quantitatively better on real data | **Met for the number, not for the artifact** | ×2400/×8640/×45532 error removed on real stores; no `.zattrs` file produced |
| Released early | **Not started** | Nothing published; no PR opened, by instruction |

### 4.1 What blocks a submission today

In priority order:

1. ~~**The patched binary has never been built or run** (`RESULTS.md` §7.1).~~
   **Closed 2026-09-16:** both binaries compile and run on GitHub-hosted runners,
   and were run against real published volumes. `CI_VALIDATION.md`.
2. ~~**No rendered output was inspected** (`RESULTS.md` §7.2).~~ **Closed:** real
   `.zattrs` (`micrometer`, `[8.64, 8.64, 8.64]`) and real TIFF resolution tags,
   with decoded pixels byte-identical to the baseline.
3. **The GUI path stays broken** without the `SegmentationCommandHandler`
   predicate change (`RESULTS.md` §7.4), so the fix would not reach most users.
   **Still the main blocker for real-world impact.**
4. ~~**The deadline is unconfirmed** (§1.1).~~ **Closed 2026-09-16:** the deadline
   is confirmed as 11:59pm Pacific, September 30th, 2026 (§1.1). What remains is
   the ordinary work of making a submission, not uncertainty about the date.

Also worth stating before a submission is written: the patch as **first committed
did not compile** (`RESULTS.md` §9.1). It is fixed, but a submission that presents
this as a clean, reviewed fix without that detail would not survive the review
team reproducing it — and the detail is a better story than the omission.

### 4.2 What a submission would be built on, once those clear

* `ROOT_CAUSE_ANALYSIS.md` — the cause, at file-and-line resolution.
* `RESULTS.md` §2 — before/after on real published data, with a control.
* 16 regression cases plus upstream's suite as a control.
* `patch/vc_render_tifxyz.patch`, reviewable in one sitting.

The strongest single asset is the *control*: the fact that the pre-patch reader
resolves exactly the one catalog volume the existing live test pins explains why a
bug this large went unnoticed for so long, and it is checkable in under a minute.

---

## 5. Standing constraints observed

From the brief, and how they were met:

| Constraint | Status |
|---|---|
| Do not promise a prize | Complied with; prize award is discretionary and no claim is made |
| No download over 2 GB without authorisation | Complied with — total downloaded ≈ 1.2 MB of headers + ≈ 12 KB of metadata |
| No heavy installs without authorisation | Complied with — nothing installed; the existing MSVC toolchain was used |
| No paid cloud services | Complied with — none used |
| No pull requests, no publishing | Complied with — nothing pushed or opened |
| Do not modify the upstream repository | Complied with in the sense that matters: no remote write, no push. The local clone is modified in place to hold the patch; `git apply --check --reverse` proves the local diff is exactly the patch |
| Do not touch FitPatch Loop | Complied with — see `PROJECT_STATUS.md` §2 |
