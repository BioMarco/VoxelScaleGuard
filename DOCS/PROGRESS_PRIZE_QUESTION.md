# PROGRESS_PRIZE_QUESTION

**A question prepared for the Vesuvius Challenge organisers. It has NOT been sent.**
No message has been sent to the organisers by any channel, and nothing here should be
sent without the author's explicit decision.

The question is real and narrow: the prize Terms and Conditions make a *permissive*
licence a condition of **accepting** a Progress Prize, while the contribution's patch
is necessarily a modification of GPL-3.0-or-later code that the author has no power
to relicense.

---

## 1. The two texts that create the question [live, re-read 2026-09-18]

From the Terms and Conditions at the foot of <https://scrollprize.org/prizes>
(reproduced verbatim inside the Progress Prizes submission form as well):

> *"You agree to make your method open source if you win a prize. It does not have to
> be open source at the time of submission, but you have to make it open source under
> a permissive license to accept the prize."*

From the Progress Prizes section of the same page:

> *"In addition to milestone-based prizes, we offer monthly prizes for open source
> contributions that help read the scrolls."*

And from the 2027 Grand Prize conditions, where the same requirement appears in a
second phrasing:

> *"Pipeline fully reproducible and code shared under an open source license (e.g.
> MIT), published publicly on GitHub."*

"Open source" and "permissive" are not synonyms: the GNU GPL v3 is an open-source
licence by the OSI definition, but it is not permissive, because it imposes copyleft
conditions. The page uses both phrasings, so the question cannot be answered by
reading it more carefully.

## 2. Why it applies to this contribution concretely

* The contribution's patch modifies three files of `volume-cartographer/`, which the
  `villa` monorepo licenses **GPL-3.0-or-later**, Copyright (C) 2023 EduceLab. A
  patch against that code is a modified version of it, and cannot be relicensed MIT
  by its author. This is not a choice the author can make differently.
* Everything else in the contribution — the investigation, the test harness, the
  continuous-integration workflow, the evidence figures, the documentation — is the
  author's original work and **is** offered under the MIT licence.
* The MIT licence is applied and the split is documented path by path in
  [`NOTICE.md`](../NOTICE.md); the third-party tomographic data is handled separately
  in [`DATA_ATTRIBUTION.md`](../DATA_ATTRIBUTION.md) under its own CC BY-NC 4.0
  terms.

## 3. The message, ready to send

**Channel:** the Vesuvius Challenge Discord (the prizes page directs general
questions there), or `grandprize@scrollprize.org` — which the site presents as the
Grand Prize submission address, so Discord is the more appropriate first route for a
rules question. **Neither has been used.**

**Subject line, if email:** `Progress Prize: does a GPL-3.0-or-later patch meet the licence condition?`

```text
Hello,

I am preparing a Progress Prize submission and would like to check one licence
point before I send it. I am not asking you to pre-approve the submission, and I
am not claiming it is eligible — I would just like to understand how you treat
this case.

What the contribution is: a fix for a silently wrong physical voxel size in
vc_render_tifxyz, plus the verification around it — a test harness, a
reproducible CI workflow that builds the renderer before and after and compares
the outputs, and before/after evidence on public catalog volumes.

How it is licensed. I have split it deliberately:

  - The original work is released under the MIT licence. That covers the
    investigation, the harness, the CI workflow, the evidence and the
    documentation, all published at
    https://github.com/BioMarco/VoxelScaleGuard (see NOTICE.md for the
    path-by-path mapping). No part of it is withheld.

  - The patch itself fixes three files in volume-cartographer, which villa
    distributes under GPL-3.0-or-later (Copyright (C) 2023 EduceLab). A patch
    against those files is a modified version of GPL-3.0-or-later code, so I
    cannot unilaterally relicense it as MIT. I have done the opposite of
    narrowing anything: the GPL text is included in the repository, the
    modification is stated with its date, and upstream's notices are preserved.

My question is about the wording in your Terms and Conditions: "you have to make
it open source under a permissive license to accept the prize." The GPL is open
source but not permissive, and the patched files are GPL-3.0-or-later because
that is how villa licenses them.

So: if this contribution were to be awarded a Progress Prize, would the licence
position described above satisfy that condition? Or would you want something
different from me — for example the patch contributed upstream first, so that the
change lives under villa's own licence in villa's own repository?

I am happy to make any change on my side that is actually in my power to make.
Where I have genuinely not been able to determine the answer, I have said so in
the repository rather than picking the reading that suits me.

Thank you for your time.

Marco Pontesilli
https://github.com/BioMarco/VoxelScaleGuard
```

## 4. What this draft deliberately does not do

* It does not claim the submission **is** eligible, and it does not claim it **is
  not**. Both readings are genuinely open, and asserting either would be inventing a
  conclusion the source does not support.
* It does not ask for an exemption, a waiver or special treatment, and it does not
  ask the team to change its rules.
* It does not mention the Discord username or any personal detail beyond a name and
  a repository link, and it does not name the prize amount or appeal to it.
* It does not omit the fact that the author **can** relicense the original work —
  which is the substantive part of the contribution — so the question is not
  overstated.

## 5. Discrepancies found while re-checking, reported without a conclusion

Re-read **2026-09-18** against <https://scrollprize.org/prizes>:

| Item | State |
|---|---|
| Progress Prize deadline | Unchanged and still current: *"The next deadline is 11:59pm Pacific, September 30th, 2026!"* |
| Award structure | Unchanged: *"Best Submission of the Month: $20,000, guaranteed every month"*, then typically $20,000 / $10,000 / $5,000 / $2,500 / $1,000 / $500 / $250 |
| Grand Prize and First Letters / Title deadlines | **June 25th, 2027 (11:59pm Pacific)** — all three, unchanged |
| Terms and Conditions | **Unchanged, still say "permissive license"**; the Grand Prize conditions still say *"open source license (e.g. MIT)"*. The discrepancy in §1 is therefore still live and has not been resolved on the page |
| Discord registration rule | Still under the 2027 Grand Prize's Additional terms, **not** under the Progress Prizes. The Progress Prizes form still asks only for an optional Discord display name |
| Progress Prizes form | Still the same form, still titled for the month (*"September 2026 Progress Prizes"*) |
| Contact route | The site still exposes `grandprize@scrollprize.org` labelled for Grand Prize submissions; no separate Progress Prize email is published, and the Progress Prizes section still points to the Google form |

**No conclusion is invented from these.** In particular, the fact that the Terms
have not changed is not evidence about how the organisers would read them; it only
means the question still needs asking rather than having been answered somewhere
else on the page.
