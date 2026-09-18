# NOTICE

**This repository is not under a single licence.** It contains three kinds of
material with three different sets of terms, and this file says which paths are
which. Read it before reusing anything here.

| Terms | Applies to | File |
|---|---|---|
| **MIT** | the original work of this repository | [`LICENSE`](LICENSE) |
| **GNU GPL v3.0-or-later**, Copyright (C) 2023 EduceLab | material derived from or copied out of Volume Cartographer | [`LICENSE-GPL-3.0.txt`](LICENSE-GPL-3.0.txt) |
| **CC BY-NC 4.0** unless otherwise noted; separate citation requirements | the third-party tomographic data and the images derived from it | [`DATA_ATTRIBUTION.md`](DATA_ATTRIBUTION.md) |

The MIT licence in `LICENSE` does **not** apply to the GPL or the data material,
and nothing here purports to relicense them.

---

## 1. Original work — MIT

Copyright (c) 2026 Marco Pontesilli. Full text in [`LICENSE`](LICENSE).

Everything not listed in §2 or §3 below. Concretely:

* `README.md`, `AGENTS.md`, this file, `DATA_ATTRIBUTION.md`, `.gitignore`,
  `.gitattributes`
* all of `DOCS/` **except** the raster panels embedded in
  `DOCS/evidence/before-after.png`, which are rendered from third-party data (§3)
* `ci/` — the workflow helpers, the figure generators and the self-tests
* `tools/`
* `research/` — the probe scripts and `research/vc_render_tifxyz_build_analysis.md`
  (but **not** the fetched documents in `research/raw_metadata/`, which are §3)
* `harness/` — **except** the four files listed in §2
* `.github/workflows/renderer-validation.yml`
* `patch/` as a *collection*: the selection, the justification, the evidence and
  the surrounding documentation are original. The *diff itself* is §2.

## 2. Derived from or copied out of Volume Cartographer — GPL-3.0-or-later

Volume Cartographer is a library and toolkit for virtually unwrapping volumetric
datasets, **Copyright (C) 2023 EduceLab**, licensed **GNU GPL v3.0-or-later**. Its
programme notice reads:

> This program is free software: you can redistribute it and/or modify it under
> the terms of the GNU General Public License as published by the Free Software
> Foundation, either version 3 of the License, or (at your option) any later
> version.
>
> This program is distributed in the hope that it will be useful, but WITHOUT ANY
> WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
> PARTICULAR PURPOSE. See the GNU General Public License for more details.

It lives in `ScrollPrize/villa` under `volume-cartographer/`, where the licence is
`volume-cartographer/LICENSE` and the notices are
`volume-cartographer/NOTICE` (upstream's own image label agrees:
`volume-cartographer/Dockerfile:10` →
`org.opencontainers.image.licenses="GPL-3.0"`). **A copy of the full licence text
is included here as [`LICENSE-GPL-3.0.txt`](LICENSE-GPL-3.0.txt)**, which GPL-3.0
§4 requires for anyone receiving the covered material. It was copied byte-for-byte
from upstream (35,832 bytes, SHA-256
`95dd6ceb0e40950eb88fef3a6eb017802f13c6e87fefc72500ebcededd24c760`).

### 2.1 Paths

| Path | What it is | Status |
|---|---|---|
| `patch/vc_render_tifxyz.patch` | a unified diff against three Volume Cartographer files; it reproduces their code as context and as added lines | **modified version of the upstream work**, GPL-3.0-or-later |
| `harness/src/vsguard/upstream_read_volume_voxel_size.hpp` | a byte-verbatim copy of `readVolumeVoxelSize` from `volume-cartographer/apps/src/vc_render_tifxyz.cpp` @ `757f70c0140a4cfbbbd44975ef09558444b96980`, lines 986–1003 | **verbatim copy**, GPL-3.0-or-later |
| `harness/src/vsguard/render_voxel_size_resolution.hpp` / `.cpp` | original code that nevertheless `#include`s Volume Cartographer headers and calls `vc::metadata::voxelSizeFromStoreMetadata` | **treated as GPL-3.0-or-later** — conservative treatment, see §2.4 |
| `harness/tests/test_render_voxel_size.cpp` | original tests that include the verbatim copy above and the two files above | **treated as GPL-3.0-or-later** — conservative treatment, see §2.4 |

Each of those files carries a header saying so. Nothing else in this repository
contains Volume Cartographer code.

### 2.2 Statement of modification, with dates

GPL-3.0 §5(a) requires prominent notices stating that the work was modified and
when. This repository modifies the upstream work in exactly one place:

* **`patch/vc_render_tifxyz.patch`** modifies three upstream files:
  `volume-cartographer/apps/src/vc_render_tifxyz.cpp`,
  `volume-cartographer/core/src/Zarr.cpp`,
  `volume-cartographer/core/include/vc/core/util/Zarr.hpp`.
  The change was authored **2026-09-15 to 2026-09-18** and is described in
  `DOCS/ROOT_CAUSE_ANALYSIS.md` and `DOCS/ARCHITECTURE.md`. It is a proposed
  upstream contribution and is **not** applied to upstream: upstream at the pinned
  commit is unmodified, and the patch is distributed as a diff.
* The verbatim copy in §2.1 is **not modified**. `harness/setup.ps1` re-creates it
  from git at the pinned commit, and `harness/tests/test_render_voxel_size.cpp`
  asserts that it is unpatched.

### 2.3 Upstream notices preserved

GPL-3.0 §4 requires keeping intact all notices stating that the licence applies
and the warranty disclaimers, and upstream's `NOTICE` also carries notices for
third-party components that Volume Cartographer uses. Those are reproduced
verbatim here for that reason:

```
Volume Cartographer:
A library and toolkit for virtually unwrapping volumetric datasets
Copyright (C) 2023  EduceLab

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program.  If not, see <https://www.gnu.org/licenses/>.

------------------------
External License Notices
------------------------
JSON for Modern C++ (https://github.com/nlohmann/json)
Copyright (c) 2013-2021 Niels Lohmann

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

---------------------------------------------
OpenABF (https://gitlab.com/educelab/OpenABF)
Copyright 2023 EduceLab

This product includes software developed at
EduceLab, University of Kentucky (https://www.cs.uky.edu/dri/)

--------------------------------------
bvh (https://github.com/madmann91/bvh)
Copyright (C) 2020 Arsène Pérard-Gayot

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

-------------------------------------------------
This project provides color map data derived from
mpl-colormaps (https://github.com/BIDS/colormap)
by Nathaniel Smith & Stefan van der Walt
```

The header of this quoted block is followed by the `(at your option) any later
version.` clause in upstream's `NOTICE`, which is what makes the terms
GPL-3.0-**or-later** rather than GPL-3.0-only.

### 2.4 What remains genuinely uncertain here — not resolved by this file

Adding a notice does not settle a legal question, and this file does not claim to.
Two things are **not determinable** from this repository, and they are recorded
rather than papered over:

1. **Whether this repository as a whole is an "aggregate"** in the sense of
   GPL-3.0 §5's final paragraph — *"A compilation of a covered work with other
   separate and independent works, which are not by their nature extensions of the
   covered work … is called an 'aggregate' … Inclusion of a covered work in an
   aggregate does not cause this License to apply to the other parts of the
   aggregate."* Whether the documentation, CI scripts and figure generators are
   such separate works, or parts of one work based on the GPL programme, is a
   judgement the licence text does not make for us. **The MIT licence in `LICENSE`
   rests on the aggregate reading being correct.** If it is not, the practical
   consequence is confined to those files: the safe course is to treat them as
   GPL-3.0-or-later too, which is why they are not marked MIT anywhere in-tree.
2. **Whether `harness/src/vsguard/render_voxel_size_resolution.*` and
   `harness/tests/test_render_voxel_size.cpp` are derivative or combined works.**
   They are original code, but they include GPL headers, are compiled together
   with GPL translation units, and call GPL functions. This is a legal question,
   not a determinable fact, so they are **treated conservatively as
   GPL-3.0-or-later** in §2.1 and carry GPL headers. That treatment is a
   deliberate over-inclusion — it grants recipients more permission than may
   strictly be required — and it is not a statement that the strict reading is
   correct.

A third item is unresolved for a different reason and does not affect reuse of
this repository: the licence of `villa`'s `spiral-fitting/`, `ink-detection/` and
`lasagna/` subdirectories, which declare nothing anywhere in the checkout. No
file here comes from them.

`DOCS/LICENSING_PROPOSAL.md` is the working record of how this position was
reached, including the checks that established it and the claims that were wrong
before them. It is a proposal document, and where it disagrees with this NOTICE,
**this NOTICE governs**.

## 3. Third-party data and images derived from it — not MIT

The tomographic volumes, the metadata documents fetched from the public Open Data
bucket, and the raster images rendered from those volumes are **third-party
material with their own terms**, principally CC BY-NC 4.0, and they are **not**
covered by the MIT licence in `LICENSE`. They are itemised, with sources, authors,
links and the required citations, in [`DATA_ATTRIBUTION.md`](DATA_ATTRIBUTION.md).

## 4. Build-time dependencies, not redistributed

`nlohmann/json` v3.11.3 (MIT) and `doctest` v2.4.11 (MIT) are downloaded at build
time by `harness/fetch_deps.mjs` into `harness/third_party/`, which is
git-ignored. They are **not** part of this repository and no notice for them is
required of it, though both are noticed in §2.3 and in the fetched headers
themselves.

The eight Volume Cartographer source files that `harness/setup.ps1` copies into
`harness/src/villa/` are **also not in this repository**: the `.gitignore` pattern
`villa/` has no leading slash, so it excludes a `villa` directory at any depth.
They are generated locally from a `villa` checkout, are GPL-3.0-or-later by
provenance, and are listed in §2.1 for completeness because a working-tree archive
would include them.
