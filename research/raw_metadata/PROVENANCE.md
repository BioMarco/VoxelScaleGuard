# `research/raw_metadata/` — provenance and terms

**These four `.json` files are verbatim copies of third-party documents. They are
not this project's data and they are not covered by whatever licence this project
chooses for its own work.**

Downloaded anonymously (read-only `GET`, no credentials) from the Vesuvius
Challenge public Open Data bucket by `research/fetch_volume_metadata.mjs` — see
`VOLUMES` and `CANDIDATES` at lines 22-29 of that script. File contents were not
edited; the names below are the script's own `owner/volumes/<name>.<document>`
paths with `/` replaced by `__`.

| Committed file | Source URL | Bytes | SHA-256 |
|---|---|---|---|
| `PHerc0009B__volumes__20250521125136-8.640um-1.2m-116keV-masked.zarr.metadata.json` | `https://vesuvius-challenge-open-data.s3.amazonaws.com/PHerc0009B/volumes/20250521125136-8.640um-1.2m-116keV-masked.zarr/metadata.json` | 7,607 | `e8b7bb92af4f9e149dfc75bee7f605a217dcca12c039c90f7e31428c42d9a59b` |
| `PHerc0172__volumes__20241024131838-7.910um-53keV-masked.zarr.meta.json` | `https://vesuvius-challenge-open-data.s3.amazonaws.com/PHerc0172/volumes/20241024131838-7.910um-53keV-masked.zarr/meta.json` | 244 | `6201efd328d54ea67dadd8207b18cc985513c56c3f6f289ffc2e7549a49c2046` |
| `PHercParis4__volumes__20260310173927-45.532um-11.0m-110keV-masked.zarr.metadata.json` | `https://vesuvius-challenge-open-data.s3.amazonaws.com/PHercParis4/volumes/20260310173927-45.532um-11.0m-110keV-masked.zarr/metadata.json` | 8,076 | `3ff6342a4264e463b4b36e26b297c8ccf645f896f93663822c882c4d56526d94` |
| `PHercParis4__volumes__20260323153942-2.400um-0.2m-137keV-masked.zarr.metadata.json` | `https://vesuvius-challenge-open-data.s3.amazonaws.com/PHercParis4/volumes/20260323153942-2.400um-0.2m-137keV-masked.zarr/metadata.json` | 9,021 | `26792eb8a448ed37e0835effd920552ffd3a182b584b20e8a88a5c1d37981740` |

Hashes were computed from the committed files on 2026-09-18 with
`Get-FileHash -Algorithm SHA256`, so a reader can confirm that what is here is what
the bucket served on that date — and see it if the bucket later changes.

`research/metadata_probe.json` (3,812 bytes, SHA-256
`36ac30f74c6e208d624bffd04833fd4f3551c81d21f5ed2d7724bd011495516c`) is **not** a
fetched document: it is a summary this project assembled over the four above. Its
inputs are third-party; its structure and prose are this project's.

## Terms

The published Vesuvius Challenge datasets are **CC BY-NC 4.0** unless otherwise
noted — `scrollprize.org/docs/02_data.md`, quoted in `DOCS/RESEARCH.md` §1.
Scorlls 1-4 / Fragments 1-6 scanned at DLS before 2025 are EduceLab-Scrolls,
© EduceLab / The University of Kentucky, and carry additional citation
requirements.

CC BY-NC 4.0's attribution term applies to redistributing these copies: source,
licence, and an indication of any changes (there are none — the files are
byte-identical to what the bucket served, which is what the hashes above
establish).

**The attribution that term requires is now carried in the repository**, not only
here: [`../../DATA_ATTRIBUTION.md`](../../DATA_ATTRIBUTION.md) names the rights
holders, gives the source links and both dataset citations, and itemises the
transformations applied to the derived figure. It is linked from the repository
`README.md` and from [`../../NOTICE.md`](../../NOTICE.md) §3. This file remains the
raw evidence — per-file URL, byte count and SHA-256 — behind it.

Two further points that this directory alone would not tell you:

* **The MIT licence at the repository root does not cover these files.** They are
  third-party material with their own terms. `NOTICE.md` §1 says so explicitly.
* **The four documents do not all come from the same dataset**, and therefore do
  not all carry the same required citation. `PHerc0009B` and the two `PHercParis4`
  documents are from scans made at ESRF BM18 in 2025–2026 and belong to the
  *Vesuvius Challenge – CT Scans of Herculaneum Papyri* dataset; `PHerc0172` is the
  legacy flat form and belongs to *EduceLab-Scrolls* (© EduceLab / The University
  of Kentucky). `DATA_ATTRIBUTION.md` §2 records how each was assigned and §3 gives
  both citations.

The same terms reach `DOCS/evidence/before-after.png`, whose raster panels are
renders of two of these volumes, and `DOCS/evidence/README.md` says so beside the
figure.
