# DATA_ATTRIBUTION

**Third-party tomographic material used by this repository, and the images derived
from it. None of it is covered by this repository's MIT licence
([`LICENSE`](LICENSE)).** The general licence map is in [`NOTICE.md`](NOTICE.md);
this file is the attribution those data require.

---

## 1. What the data is licensed under

The Vesuvius Challenge data portal aggregates **two** datasets with different
citation requirements, and the portal states the terms for both:

> **Licenses**
> - [CC‑BY‑NC 4.0](https://creativecommons.org/licenses/by-nc/4.0/) (unless
>   otherwise noted for specific assets)
> - Scrolls 1-4 and Fragments 1-6 scanned at DLS before 2025 are from the
>   EduceLab-Scrolls Dataset, copyrighted by EduceLab/The University of Kentucky.
>   Permission to use the data linked herein according to the terms outlined above
>   is granted to Vesuvius Challenge, with additional citation requirements listed
>   in How to Cite.

Source: <https://scrollprize.org/data> (`villa/scrollprize.org/docs/02_data.md`
lines 143-146 at the pinned revision) [read].

So: **CC BY-NC 4.0 unless otherwise noted**, and two separate datasets with
separate citations. CC BY-NC 4.0 requires attribution, a link to the licence,
an indication of any modifications, and non-commercial use.

## 2. Which dataset each volume used here belongs to, decided from the material

The portal names the datasets (lines 53-54 [read]):

> - **Vesuvius Challenge - CT Scans of Herculaneum Papyri**: newer scans released
>   directly by Vesuvius Challenge. Most current releases on this portal belong to
>   this dataset.
> - **EduceLab-Scrolls**: the legacy dataset. Scrolls 1-4 and Fragments 1-6 scanned
>   before 2025 at DLS belong to this dataset.

It does **not** label individual volumes, so the assignment below is derived from
each volume's own metadata document and from the structure of that document. The
derivation is stated so it can be checked rather than trusted.

| Volume | Scan date and facility, from the document itself | Document shape | Dataset |
|---|---|---|---|
| `PHerc0009B` | `2025-05-09`, `beamline: bm18` (ESRF) | modern: resolution as `scan.tomo.acquisition.detector.samplePixelSize` | **Vesuvius Challenge – CT Scans of Herculaneum Papyri** (post-2025, ESRF) |
| `PHercParis4` (45.532 µm) | `2026-03-10`, `beamline: bm18` | modern, same shape | **Vesuvius Challenge – CT Scans of Herculaneum Papyri** |
| `PHercParis4` (2.400 µm) | `beamline: bm18`, no acquisition date recorded | modern, same shape | **Vesuvius Challenge – CT Scans of Herculaneum Papyri** |
| `PHerc0172` | no acquisition record; the document is the legacy flat form | legacy: top-level `voxelsize: 7.91`, `uuid: 20241024131838-masked_raw-zarr`, `name: scroll5-masked_raw-zarr` | **EduceLab-Scrolls** — pre-2025, legacy shape, and "scroll5" is one of Scrolls 1-4 and Fragments 1-6 |

**This distinction is not cosmetic.** The two datasets carry different required
citations, and generalising one dataset's terms to the other would be wrong. The
`PHerc0172` row is the weaker of the four derivations — it rests on the document's
shape and naming rather than on an explicit statement, and it is recorded here as a
derivation, not as a quotation. Nothing in this repository depends on the
distinction beyond the citations below.

## 3. What a reuser must cite

### 3.1 For the `PHerc0009B` and `PHercParis4` material

Use the portal's citation for newer scans, verbatim:

> Giorgio Angelotti, Stephen Parsons, Sean Johnson, Elian Rafael Dal Prà, Johannes
> Rudolph, Paul Tafforeau, Alessandro Mirone, Paul Henderson, Hendrik Schilling,
> Forrest McDonald, David Josey, Youssef Nader, C. Seth Parker, W. Brent Seales.
> *Vesuvius Challenge - CT Scans of Herculaneum Papyri*. Vesuvius Challenge.

### 3.2 For the `PHerc0172` material

The portal states, for EduceLab-Scrolls:

* in any published abstract, cite `EduceLab-Scrolls` as the source of the data;
* in any published manuscript, reference: Parsons, S., Parker, C. S., Chapman, C.,
  Hayashida, M., & Seales, W. B. (2023). *EduceLab-Scrolls: Verifiable Recovery of
  Text from Herculaneum Papyri using X-ray CT*. ArXiv [Cs.CV].
  <https://doi.org/10.48550/arXiv.2304.02084>;
* and include language similar to: *"Data used in the preparation of this article
  were obtained from the EduceLab-Scrolls dataset [citation above]."*

The EduceLab-Scrolls material is **copyrighted by EduceLab / The University of
Kentucky**.

## 4. The files in this repository that carry third-party material

### 4.1 `research/raw_metadata/*.json` — verbatim copies, unmodified

Four metadata documents downloaded from the public Open Data bucket. They are
byte-identical to what the bucket served; **no modification has been made**, which
is what the recorded hashes establish. Per-file source URL, byte count and SHA-256:
[`research/raw_metadata/PROVENANCE.md`](research/raw_metadata/PROVENANCE.md).

| File | Volume | Dataset |
|---|---|---|
| `PHerc0009B__volumes__20250521125136-8.640um-1.2m-116keV-masked.zarr.metadata.json` | `PHerc0009B` | §3.1 |
| `PHerc0172__volumes__20241024131838-7.910um-53keV-masked.zarr.meta.json` | `PHerc0172` | §3.2 |
| `PHercParis4__volumes__20260310173927-45.532um-11.0m-110keV-masked.zarr.metadata.json` | `PHercParis4` | §3.1 |
| `PHercParis4__volumes__20260323153942-2.400um-0.2m-137keV-masked.zarr.metadata.json` | `PHercParis4` | §3.1 |

`research/metadata_probe.json` is **not** third-party: it is a summary this project
assembled over the four documents above. Its inputs are third-party.

### 4.2 `DOCS/evidence/before-after.png` — a derived work

**This image is derived from third-party tomographic data and must not be treated
as MIT-licensed.** It contains two rendered panels and it is a derivative of the
volumes named above.

What was done to the source data, so that "indicate modifications" is satisfied:

1. The images **are not the raw CT volumes.** They are flattened renders produced
   by `vc_render_tifxyz`, the tool this repository investigates, from a surface
   mesh (`20250510172639-on-20250521125136-8.64um.tifxyz`) over the `PHerc0009B`
   volume. The rendering pipeline is not this project's work and is not altered by
   it: the patch changes only the *declared physical scale*, never the pixels.
2. The two raster panels embedded in the figure are the resulting `00.tif` slices,
   placed side by side **without retouching, cropping or colour adjustment**.
   `ci/selftest_evidence_figure.py` asserts that both panels are byte-identical to
   panels rebuilt from the real TIFFs, which is what makes that claim checkable.
3. Around them, original annotation was added: titles, a table of the declared
   metadata values, and a third panel stating that the decoded pixels are
   identical.
4. Both renders come from the baseline and the patched build respectively, run
   with identical arguments, and their decoded pixel arrays are identical.

The third-party content is the papyrus shown in those two panels. The annotations,
the layout and the difference panel are this project's original work.

### 4.3 `DOCS/evidence/terminal-before-after.png` — not third-party material

This figure reproduces log output from this project's own CI run. It embeds **no**
raster image data from the volumes, so the CC BY-NC terms do not attach to it. It
does quote file paths and metadata values from the volumes, which is
non-substantial factual quotation.

## 5. Where this attribution is surfaced

Deliberately reachable from the places a reader actually looks, rather than filed
away in a technical document:

* [`README.md`](README.md) → the **Data and licences** section, which links here
  and to `NOTICE.md`;
* [`NOTICE.md`](NOTICE.md) → §3, the licence map;
* [`DOCS/evidence/README.md`](DOCS/evidence/README.md) → beside the figures
  themselves, including the licence of the data shown in them;
* [`research/raw_metadata/PROVENANCE.md`](research/raw_metadata/PROVENANCE.md) →
  next to the copied documents, with their hashes;
* [`DOCS/RESEARCH.md`](DOCS/RESEARCH.md) §1 and §5 → the source table used during
  the investigation.

## 6. What is deliberately *not* claimed

* The MIT licence in [`LICENSE`](LICENSE) is **not** applied to any of this
  material, nor to the panels inside `before-after.png`.
* No licence is asserted over the tomographic data itself. The terms above are
  those of its publishers, reported as the portal states them.
* The dataset assignment in §2 is a derivation from each volume's own metadata, not
  a quotation from an official per-volume index, and it is recorded as such —
  particularly for `PHerc0172`.
