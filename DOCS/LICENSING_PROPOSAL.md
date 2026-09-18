# LICENSING_PROPOSAL

**A proposal for the author's decision. No licence has been applied to this
repository's original work.** Nothing here is legal advice; it states what the
relevant licences say and what the conventions are, so the decision can be made on
facts.

Two readings inform this document, both done on 2026-09-18 from the local
checkouts:

* a **licence and attribution inventory** of this repository and of `villa`
  (53 tracked files here; `villa` at `757f70c0140a4cfbbbd44975ef09558444b96980`,
  2026-09-15, 1065 tracked files under `volume-cartographer/` alone);
* direct reads of the licence files it cites, quoted below with their paths.

Where a fact comes from a file, the file is named. Where the inventory could not
determine something, §7 says so instead of guessing.

---

## 1. The finding that matters

**`villa`'s root is MIT, but the subproject this patch modifies is
GPL-3.0-or-later.** Every document in this repository that said "villa is MIT" and
concluded that the patch "carries the same terms" rested on a false premise.

| Path | Licence | Evidence |
|---|---|---|
| `villa/LICENSE` | **MIT**, Copyright (c) 2024 Vesuvius Challenge | the file itself (21 lines) |
| `villa/volume-cartographer/LICENSE` | **GNU GPL version 3** (35,832 bytes; 641 lines) | `GNU GENERAL PUBLIC LICENSE` / `Version 3, 29 June 2007`; the programme notice at lines 634-636 reads `Volume Cartographer: A library and toolkit for virtually unwrapping volumetric datasets / Copyright (C) 2023 EduceLab`, and line 641 adds `(at your option) any later version.` |
| `villa/volume-cartographer/NOTICE` | **GPL-3.0-or-later**, Copyright (C) 2023 EduceLab | lines 1-8 repeat the notice verbatim |
| `villa/volume-cartographer/Dockerfile:10` | independent corroboration | `LABEL org.opencontainers.image.licenses="GPL-3.0"` |

`volume-cartographer/` is **not a submodule**: `villa/.gitmodules` is 0 bytes and
`volume-cartographer/LICENSE` is a tracked blob (mode `100644`) inside villa's own
repository. It is part of the tree, and villa's root `LICENSE` does not govern it.

The three files the patch touches — `volume-cartographer/apps/src/vc_render_tifxyz.cpp`,
`core/src/Zarr.cpp`, `core/include/vc/core/util/Zarr.hpp` — sit under
`volume-cartographer/`, and **none of them carries a per-file licence header**.
Searching every `*.cpp/*.hpp/*.h/*.c/*.py/*.txt/*.md/*.cmake` under
`volume-cartographer/` for `GNU General Public`, `SPDX-License-Identifier` or
`Copyright (C)` returns hits **only** under `libs/` (vendored third-party); there
are none in `apps/`, `core/` or `utils/`. So the directory-level `LICENSE` +
`NOTICE` are the operative terms, and — usefully — the byte-identical copies this
repository makes were not stripped of anything, because there was nothing to strip.

### The rest of `villa` is a patchwork, so "villa is MIT" is unsafe shorthand

| Path | Licence | Evidence |
|---|---|---|
| `LICENSE` (root) | MIT, © 2024 Vesuvius Challenge | root file |
| `volume-cartographer/LICENSE` + `NOTICE` | **GPL-3.0-or-later**, © 2023 EduceLab | quoted above |
| `foundation/pgs-recon/LICENSE` | **AGPL-3.0** | `GNU AFFERO GENERAL PUBLIC LICENSE`; `VILLA_PROVENANCE.md:11` |
| `foundation/sam2-photogrammetry/LICENSE` | Apache-2.0 | L1 (plus BSD-3-Clause and Meta BSD sub-licences alongside) |
| `deprecated/crackle-viewer/LICENSE` | Apache-2.0 | L1 |
| `deprecated/thaumato-anakalyptor/LICENSE` | MIT, © 2024 Julian Schilliger | L1-3 |
| `dinovol/LICENSE` | MIT, © 2024 Vesuvius Challenge | L1-3 (plus Apache-2.0 DINOv2 notices) |
| `scrollprize.org/LICENSE` | MIT — **code only**; the site's *content* is CC BY-NC 4.0 | `docusaurus.config.js:294`; see §4 |
| `segmentation/models/batchgeneratorsv2/LICENSE` | Apache-2.0 | L1 |
| `volume-cartographer/libs/ECL-MaxFlow/LICENSE` | BSD-3-Clause | L1-3 |
| `volume-cartographer/libs/OpenABF/LICENSE` | Apache-2.0 | L1 |
| `volume-cartographer/libs/cc3d/cc3d.hpp` | **LGPL-3.0-or-later** (in-file) | in-file notice |
| `volume-cartographer/libs/libigl_changes/.../slim.cpp` | MPL-2.0 (in-file) | in-file notice |
| `volume-cartographer/libs/djikstra3d/hedly.h` | CC0-1.0 | `SPDX-License-Identifier: CC0-1.0` |
| `volume-cartographer/libs/djikstra3d/libdivide.h` | Boost **or** zlib (dual) | in-file notice |
| `volume-cartographer/libs/edt/threadpool.h` | zlib-style | in-file notice |

`spiral-fitting/`, `ink-detection/` and `lasagna/` declare **no licence anywhere**
in the checkout (`vesuvius/pyproject.toml:13` does say MIT); and
`segmentation/models/arch/nnunet/pyproject.toml:7` points at a `LICENSE` file that
does not exist. None of this touches the present work, but it does mean the
monorepo has no single answer to "what licence is this?".

**Copyleft in villa, called out so it is not discovered late:**
`volume-cartographer/` (GPL-3.0-or-later), `foundation/pgs-recon/` (AGPL-3.0),
`libs/cc3d` (LGPL-3.0), `libs/libigl_changes` (MPL-2.0). Only the first is touched
by this work.

### Why this was previously mis-stated

`README.md` and `AGENTS.md` both said "`villa` is MIT … The patch is a derivative
work and carries the same terms", and `DOCS/PRIZE_REQUIREMENTS.md` used that to
argue the prize's licence condition was satisfied. The conclusion may still be
reachable — see §4 — but the premise was wrong. All of those documents are
corrected in the same change as this proposal, and each correction is recorded
where it was made rather than silently deleted.

---

## 2. What the licences imply, factually

**The patch.** `patch/vc_render_tifxyz.patch` (blob `c4c1a99`, 20,564 bytes — 20,559
UTF-8 characters, because nine of its bytes are multi-byte) is a `git diff` against
GPL-3.0-or-later files. `git apply --numstat` reports:

```
235  65  volume-cartographer/apps/src/vc_render_tifxyz.cpp
  5   0  volume-cartographer/core/include/vc/core/util/Zarr.hpp
 10   0  volume-cartographer/core/src/Zarr.cpp
```

A diff that reproduces a substantial part of those files, and any file it produces
when applied, is conventionally read as a **modified version / work based on** the
GPL programme. GPL-3.0 §5 conditions conveying that in source form on (a)
prominent notices stating that you modified it, with a relevant date; (b) a
prominent notice that it is released under this Licence; and (c) licensing the
entire work as a whole under this Licence. §5(c) itself adds: *"This License gives
no permission to license the work in any other way, but it does not invalidate such
permission if you have separately received it."*

**Practical consequence for the upstream contribution: none.** `villa` is the
GPL-3.0-or-later project, so contributing the patch upstream is contributing under
its own licence, and no notice beyond the patch itself is required of a
contributor. The licence question is only about how *this* repository is licensed
and about the prize rules (§4).

**The harness mirrors — the one place this repository might not be a clean
aggregate.** Three tracked files under `harness/src/vsguard/` are not merely
"about" upstream code:

| File | Relationship to upstream | Notice today |
|---|---|---|
| `upstream_read_volume_voxel_size.hpp` | **verbatim copy** of upstream's deployed reader (`readVolumeVoxelSize`, lines 986-1003 of the pinned file) | provenance comment naming the repo, commit, file and lines; **no licence or copyright notice** |
| `render_voxel_size_resolution.{hpp,cpp}` | the author's own decision procedure, but it `#include`s the GPL headers and calls upstream's resolver directly (`using vc::metadata::voxelSizeFromStoreMetadata;`, used at `.cpp:177` and `.cpp:193`) — the file says so at `.cpp:14-16` | none |
| `patch_integrity.hpp` | author's own utility; contains no upstream code (only path strings and a quoted compiler error) | n/a |

Whether `render_voxel_size_resolution.*` is a **derivative or a combined work** is
a legal question this document does not settle. It is flagged in §7.

**What is *not* a problem, and was expected to be:** the eight upstream files
`harness/setup.ps1` copies into `harness/src/villa/**` are **not tracked in git**.
`.gitignore`'s `villa/` pattern has no leading slash, so it matches a directory named
`villa` at *any* depth — and therefore excludes `harness/src/villa/` too.
`git check-ignore -v harness/src/villa/Json.cpp` →
`.gitignore:villa/	harness/src/villa/Json.cpp` (the rule was at `.gitignore:13` when
this inventory was taken; it is now at line 22, because a comment explaining exactly
this was added above it on 2026-09-18). A `git clone` of this repository therefore
acquires **none**
of that GPL source. (The comment above the pattern was written for the root clone,
so the exclusion of `harness/src/villa/` reads as accidental rather than designed —
but the effect is the licensing-clean one. See §5.)

The eight copies, and their upstream sources (all GPL `volume-cartographer/`):

| Upstream source | Copy | Byte-identical? |
|---|---|---|
| `utils/src/Json.cpp` | `harness/src/villa/Json.cpp` | yes (SHA-256) |
| `utils/include/utils/Json.hpp` | `harness/src/villa/utils/include/utils/Json.hpp` | yes |
| `core/src/VoxelSizeMetadata.cpp` | `harness/src/villa/vc/core/util/VoxelSizeMetadata.cpp` | yes |
| `core/include/vc/core/util/VoxelSizeMetadata.hpp` | `harness/src/villa/vc/core/util/VoxelSizeMetadata.hpp` | yes |
| `core/src/RemoteUrl.cpp` | `harness/src/villa/vc/core/util/RemoteUrl.cpp` | yes |
| `core/include/vc/core/util/RemoteUrl.hpp` | `harness/src/villa/vc/core/util/RemoteUrl.hpp` | yes |
| `core/test/test_voxel_size_metadata.cpp` | `harness/src/villa/test_voxel_size_metadata.cpp` | yes |
| `apps/src/vc_render_tifxyz.cpp` | `harness/src/villa/apps/src/vc_render_tifxyz.cpp` | yes — taken via `git show <commit>:<path>` (`setup.ps1:77`), `git hash-object` = `9d0c2b8d9feecc33dfb053a23b9b898c4e73e9a3`, matching the patch's `index 9d0c2b8..` |

`harness/CMakeLists.txt` therefore **cannot build from a bare clone**: `setup.ps1`
must run first, and it needs a local `villa` checkout. That is already documented in
`AGENTS.md` §8.

---

## 3. Third-party dependencies

Exactly two third-party code artefacts are pulled by this repository, declared
identically in `harness/fetch_deps.mjs:11-16` and `harness/setup.ps1:91-96` and both
git-ignored (`.gitignore:5`):

| Dependency | Version | Licence | How obtained | Attribution needed? |
|---|---|---|---|---|
| `nlohmann/json` | v3.11.3 | MIT | fetched at build time from `raw.githubusercontent.com` into `harness/third_party/nlohmann/json.hpp`; also apt `nlohmann-json3-dev` in CI | **No, not as configured**: git-ignored, so not redistributed. The fetched header carries its own `SPDX-License-Identifier: MIT` and `SPDX-FileCopyrightText: 2013-2023 Niels Lohmann`. Would need a notice if ever vendored or if a working-tree zip is shipped. Upstream's own `volume-cartographer/NOTICE:20-39` lists it independently. |
| `doctest` | v2.4.11 | MIT | fetched at build time into `harness/third_party/doctest/doctest/doctest.h` | same: header carries `Copyright (c) 2016-2023 Viktor Kirilov` / `Distributed under the MIT Software License`; not redistributed |

No others. (`villa/.../core/test/doctest_compat/doctest/doctest.h` is **not**
doctest — it is upstream's own 4-line forwarder to `vc_test.hpp`.)

Build dependencies are obtained by apt on the CI runner —
`nlohmann-json3-dev`, OpenCV, libtiff, Boost, Ceres, CGAL, Qt6 and the rest, from
the list at `.github/workflows/renderer-validation.yml:187-203`, which mirrors
upstream's own `scripts/install_build_deps.sh`. They are **never redistributed**.

`harness/tools/ninja.exe` is present but referenced by **nothing**, and
`DOCS/RESULTS.md:554-557` already records its ignore entry as vestigial. Its
provenance is not determinable from the checkout (§7); it is not distributed.

---

## 4. Third-party material committed to this repository

This is the part of the position that is **least documented today** and needs
action regardless of the licence decision.

| Material | Where | Terms | State |
|---|---|---|---|
| Prize-rule quotations | `DOCS/PRIZE_REQUIREMENTS.md` (§1, §2.1, §2.2), `DOCS/RESULTS.md:456-458`, `DOCS/SUBMISSION_DRAFT.md:20-24`, `DOCS/PROGRESS_PRIZE_CHECKLIST.md:33-37` | `scrollprize.org` site **content** is CC BY-NC 4.0 (`scrollprize.org/docusaurus.config.js:294`; `docs/02_data.md:145`) — the site's *code* is MIT, a different thing | Short quotations, each already citing the page; `DOCS/RESEARCH.md:14` and `README.md` state the licence. Consistent with attribution; no licence text is reproduced here. |
| Open Data bucket metadata documents | `research/raw_metadata/*.json` — 4 files, 244–9,021 B, committed **verbatim** | Published datasets are CC BY-NC 4.0 unless otherwise noted (`scrollprize.org/docs/02_data.md:145`); Scorlls 1-4 / Fragments 1-6 scanned at DLS before 2025 are EduceLab-Scrolls, © EduceLab / The University of Kentucky, with additional citation requirements (`02_data.md:146`) | **No attribution and no licence notice anywhere.** This is the gap. |
| `research/metadata_probe.json` | repo | author-assembled probe summary over the documents above | inputs third-party; the summary is the author's |
| `DOCS/evidence/before-after.png` | repo, 167,189 B, the only tracked binary | derived from CC BY-NC 4.0 source data — `DOCS/evidence/README.md:21-25` states the panels are the actual `00.tif` from the run, i.e. renders of bucket volumes | Attribution + licence notice + indication that it is a derived work: **not yet stated on or beside the figure** |
| `DOCS/evidence/terminal-before-after.png` | 292,297 B | GitHub Actions logs, the author's own output | no third-party content |

### The licence question the prize rules raise

The official prizes page (<https://scrollprize.org/prizes>, re-read 2026-09-18) says,
for both the 2027 Grand Prize and the Progress Prizes:

> *"Pipeline fully reproducible and code shared under an open source license (e.g.
> MIT), published publicly on GitHub."* — and for the Progress Prize's short list:
> *"Are released or open-sourced early."*

> *"You agree to make your method open source if you win a prize. It does not have to
> be open source at the time of submission, but you have to make it open source under
> a permissive license to accept the prize."* (Terms and Conditions)

Three things follow:

1. "open source" and "permissive" are not synonyms. GPL-3.0-or-later **is** open
   source by the OSI definition; it is **not** permissive, because it imposes
   copyleft conditions. The page says "e.g. MIT" for the former and "permissive" for
   the latter — a discrepancy to **raise with the organisers, not resolve here**.
2. **The conflict is concrete, not hypothetical.** The GPL obligation attaches to the
   patched `villa` files, which are upstream's; this repository cannot waive it, and
   a patch that only exists as a diff against GPL code cannot be relicensed MIT.
3. **This repository's own work can be permissively licensed without difficulty**
   (§5), and that is the larger and more original part of the submission — the
   verification, the harness, the CI, the evidence and the documentation. The patch
   itself is the smallest part and follows its upstream's terms.

So there are two honest framings, and the author should choose knowingly:

* **(a)** Submit as-is and state the split: original work permissive, the patch
  GPL-3.0-or-later because it modifies a GPL-3.0-or-later subproject. Ask the
  organisers in advance how they treat that.
* **(b)** Ask the organisers first whether a contribution to a GPL-3.0-or-later
  subproject satisfies the "permissive" condition, and only then decide.

Neither this document nor the agent that wrote it can decide this. It is asked as
question 3 in §8.

---

## 5. Proposal for this repository's own work

**Proposed: MIT for everything original, with upstream notices preserved where
upstream code lives.** MIT is chosen because it is `villa`'s own root licence, it is
the licence the prize page names first, and it can coexist with the
GPL-3.0-or-later parts as separate works in one repository.

### What would be licensed MIT (wholly this project's work — 44 of the 53 tracked files)

* all of `DOCS/` — every document, and the figures under `DOCS/evidence/`
  (with the third-party data attribution from §4 carried alongside, not replaced)
* `README.md`, `AGENTS.md`, `.gitattributes`, `.gitignore`
* `harness/` except `harness/src/villa/**` (untracked anyway) and except the three
  `harness/src/vsguard/` files in §2's table
* `ci/` — the workflow helpers, the figure generators and the self-tests
* `tools/` — the `git` wrapper and the fork-branch staging script
* `research/` — the probe scripts and the analysis document
* `.github/workflows/renderer-validation.yml`

### What would keep upstream's terms

| Path | Terms |
|---|---|
| `patch/vc_render_tifxyz.patch` | **GPL-3.0-or-later** (derivative of `volume-cartographer` files) |
| `harness/src/vsguard/upstream_read_volume_voxel_size.hpp` | a verbatim extract of upstream GPL-3.0-or-later code — **GPL-3.0-or-later** |
| `harness/src/vsguard/render_voxel_size_resolution.{hpp,cpp}`, `harness/tests/test_render_voxel_size.cpp` | author's own code that includes and calls GPL code; **derivative-or-combined, not determinable here** (§7) — the conservative treatment is GPL-3.0-or-later |
| `harness/src/villa/**` | upstream's terms; GPL-3.0-or-later for all eight; **already untracked, so not distributed** |

### Concrete edits this proposal would need, if approved

1. **`LICENSE`** at the repository root — the MIT text with the author's copyright
   line. *(Not added: awaiting the decision.)*
2. **`NOTICE`** (or a `## Licences` section that `NOTICE` points at) stating plainly:
   * this repository's own work is MIT;
   * `patch/vc_render_tifxyz.patch`, `harness/src/vsguard/upstream_read_volume_voxel_size.hpp`,
     and the (untracked) copies under `harness/src/villa/**` are
     **GPL-3.0-or-later**, Copyright (C) 2023 EduceLab, and are **not** covered by
     the MIT licence;
   * a copy of the GPL-3.0 text and upstream's `NOTICE` are required by GPL-3.0 §4
     for anyone receiving those files, and where they can be obtained
     (`https://github.com/ScrollPrize/villa` → `volume-cartographer/LICENSE` and
     `volume-cartographer/NOTICE`), since `villa/` is git-ignored here;
   * the date and fact of modification for the patched files (GPL-3.0 §5(a));
   * that the copied upstream files are byte-for-byte and unmodified, so a reader
     can tell them apart from this project's code.
3. **A short header** on `harness/src/vsguard/upstream_read_volume_voxel_size.hpp`
   recording that it is upstream code under GPL-3.0-or-later, with origin
   (`villa` @ `757f70c`, file and line range). It currently has a provenance comment
   but **no licence notice**.
4. **`README.md`'s Licences section** updated to state the split rather than the
   previously asserted "villa is MIT".
5. **Third-party data attribution for `research/raw_metadata/**` and
   `DOCS/evidence/before-after.png`** (§4) — required by CC BY-NC 4.0's attribution
   term and missing today. Independent of which licence the author chooses.

### One decision that the inventory resolved in passing

The obvious worry — that `harness/setup.ps1` redistributes eight GPL source files —
**does not apply as the repository is configured**: they are untracked. Two options
remain, and the choice is the author's:

* **leave it as it is** — the files are re-created locally by `setup.ps1` and never
  committed; nothing GPL is redistributed. Cost: this is currently a side effect of
  an over-broad `.gitignore` pattern rather than a stated policy, so it is worth a
  one-line comment in `.gitignore` making it deliberate; and `harness/CMakeLists.txt`
  cannot build from a bare clone without a `villa` checkout (already documented).
* **re-document it explicitly** — say in `README.md` and `AGENTS.md` that
  `harness/src/villa/**` is generated, deliberately untracked, and GPL-3.0-or-later
  by provenance, so no reader has to rediscover this.

Either way, the current state is the licensing-clean one, and this document records
that it was found to be so **by accident**, not by design.

---

## 6. Attribution (unchanged, and non-negotiable)

Independent of the licence choice, `README.md`, `DOCS/PR_DRAFT.md`,
`DOCS/SUBMISSION_DRAFT.md` and `DOCS/FEASIBILITY.md` credit the prior work:
**NicolasHuberty** (PR #1417, the approach), **Bullo27** and the villa maintainers
(PRs #1227, #1229, #1454), **Bullo27** (#1228, the GUI predicate), **DarthCeltic**
(#1403) and **Bullo27** (#1226, the issue reports). The `vc_grow_seg_from_seed` half
of #1403 is recorded as already fixed upstream and explicitly not claimed.

The repository's own licence does not affect this, and the attribution is present
now, before any licence is chosen.

---

## 7. What is genuinely not determinable from the checkout

Recorded rather than guessed:

1. **Whether the repository as a whole is an "aggregate"** under GPL-3.0 §5's final
   paragraph — *"A compilation of a covered work with other separate and independent
   works, which are not by their nature extensions of the covered work … is called an
   'aggregate' … Inclusion of a covered work in an aggregate does not cause this
   License to apply to the other parts of the aggregate."* Whether this repository's
   documentation, CI and evidence generators are such separate works, or parts of a
   single work based on the GPL programme, is the pivot on which the MIT choice for
   the rest of the repository turns. The licence text does not settle it.
2. **Whether `harness/src/vsguard/render_voxel_size_resolution.*` is derivative or
   combined.** It includes GPL headers, links GPL translation units and calls GPL
   code. The conservative treatment in §5 is GPL-3.0-or-later.
3. **`harness/tools/ninja.exe`'s provenance** — present, ignored, referenced by
   nothing, not distributed.
4. **The licences of `spiral-fitting/`, `ink-detection/`, `lasagna/`** — no file and
   no declaration anywhere in the checkout.
5. **`segmentation/models/arch/nnunet`'s licence** — its `pyproject.toml` points at a
   `LICENSE` file absent from the checkout.
6. **Upstream GitHub's current licence detection** for `ScrollPrize/villa` — not
   checked (this inventory used no network). The local `LICENSE`/`NOTICE`/`Dockerfile`
   chain is self-sufficient for the conclusion in §1.
7. **Upstream `educelab/volume-cartographer`'s own licence file** — same reason;
   `villa`'s copy is the authority used here.

None of items 3-7 changes any decision in this document. Items 1 and 2 do, and they
are why §5 is a proposal rather than a statement.

---

## 8. What I need from you

1. **Approve MIT for the original work**, or name a different licence. If the
   "aggregate" reading in §7(1) is uncomfortable, the fallback is to license the
   whole repository GPL-3.0-or-later, which is certainly lawful and costs nothing
   except the prize wording in §4.
2. **Approve the third-party data attribution edits** in §4/§5 item 5 — these are
   required by the source data's terms and are missing today, so they are not really
   optional.
3. **Say whether to raise the "open source vs permissive" wording with the prize
   organisers** before submitting, given that the patched files are GPL-3.0-or-later.

Until then, no `LICENSE` file is added, and the split in §5 is documented only as a
proposal.
