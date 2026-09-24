# `patch/` — licence and status of the patch

## What is here

`vc_render_tifxyz.patch` — a unified diff that modifies **three** files of
[Volume Cartographer](https://github.com/ScrollPrize/villa/tree/main/volume-cartographer):

| Upstream file | Change |
|---|---|
| `volume-cartographer/apps/src/vc_render_tifxyz.cpp` | +238 / −65 — the fix |
| `volume-cartographer/core/src/Zarr.cpp` | +85 / −34 — when the voxel size is unknown, write no axis unit and encode relative pyramid scaling instead of a placeholder physical scale, while keeping the `multiscales` discovery block |
| `volume-cartographer/core/include/vc/core/util/Zarr.hpp` | +35 / −3 — the documented contract for the above |

It applies to `villa` @ `757f70c0140a4cfbbbd44975ef09558444b96980` and to upstream
`main` @ `b1ef996e357de0b2f24fb30198c6d9c32611d4fb` (2026-09-18), where all three
blobs are byte-identical to the pinned revision.

## Licence — GPL-3.0-or-later, **not** MIT

**This patch is a modified version of GPL-3.0-or-later code.** All three files it
touches live under `volume-cartographer/`, which upstream licenses under the GNU
General Public License v3.0-or-later, **Copyright (C) 2023 EduceLab**. The
repository's MIT licence in [`../LICENSE`](../LICENSE) does **not** apply to this
patch; see [`../NOTICE.md`](../NOTICE.md) §2.

The diff reproduces substantial parts of those files, as context and as added
lines, and the files it produces on application are modified versions of them.
Accordingly:

* the full licence text is included in this repository as
  [`../LICENSE-GPL-3.0.txt`](../LICENSE-GPL-3.0.txt), which GPL-3.0 §4 requires for
  anyone receiving the covered material;
* **statement of modification and date** (GPL-3.0 §5(a)): the three files above
  were modified in this contribution between **2026-09-15 and 2026-09-18**. The
  change is described in `../DOCS/ROOT_CAUSE_ANALYSIS.md` and
  `../DOCS/ARCHITECTURE.md`;
* **statement of licence** (GPL-3.0 §5(b)): the modified files, and this diff
  itself, are released under the **GNU GPL v3.0-or-later**;
* upstream's `NOTICE`, including its programme notice, warranty disclaimer and
  external notices, is reproduced verbatim in `../NOTICE.md` §2.3, and upstream's
  own `volume-cartographer/LICENSE` and `volume-cartographer/NOTICE` are the
  authoritative originals.

Nothing here relicenses upstream code, and no MIT claim is made over any of it.

## Status: proposed, not applied, not merged

The patch is a **proposed** upstream contribution. It has **not** been sent as a
pull request, is not applied to `villa`, and is not merged anywhere. Upstream at
the pinned commit is unmodified; downstream, the change exists only as this diff.
A prepared fork branch exists at
<https://github.com/BioMarco/villa/tree/fix/render-voxel-size-from-open-volume>
and no pull request has been opened from it.

## Why this file exists rather than a comment inside the patch

A notice written into `vc_render_tifxyz.patch` itself would stop it being usable
with plain `git apply` in every context, and — more importantly — the CI workflow
compares the *applied* diff byte-for-byte against this artefact
(`PATCH_IDENTICAL`). Anything prepended to the file would either break that check
or make it compare something other than what upstream would apply. The notice
therefore lives beside the artefact, and the patch stays exactly the bytes that
`git apply` consumes.

## Regenerating it

The pathspec must name **all three** files. `../AGENTS.md` §4 has the exact
command; `ci/preflight_workflow.py` and the CI workflow both assert that the
artefact touches exactly three files, because a regeneration command that named
only the renderer once produced a 2/3 patch that still reverse-applied cleanly —
see `../DOCS/RESULTS.md` §12.1.
