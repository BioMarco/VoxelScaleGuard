// Dump the published metadata documents of real open-data volumes.
//
// Purpose: establish, from the live catalog rather than from an issue report,
// which voxel-size fields each volume actually publishes and where they live.
// Read-only: GET requests against the public anonymous Open Data bucket.
//
// Usage: node research/fetch_volume_metadata.mjs [outdir]

import { mkdir, writeFile } from 'node:fs/promises';
import { dirname, join } from 'node:path';

const BUCKET = 'https://vesuvius-challenge-open-data.s3.amazonaws.com';

// The volume named in villa#1403 / villa#1226, plus the legacy-style volume that
// villa#1226 identified as the single catalog entry the ungated lookup reaches.
const VOLUMES = [
  'PHerc0009B/volumes/20250521125136-8.640um-1.2m-116keV-masked.zarr',
  'PHercParis4/volumes/20260310173927-45.532um-11.0m-110keV-masked.zarr',
  'PHercParis4/volumes/20260323153942-2.400um-0.2m-137keV-masked.zarr',
  'PHerc0172/volumes/20241024131838-7.910um-53keV-masked.zarr',
];

const CANDIDATES = ['meta.json', 'metadata.json'];

// Mirror of vc::metadata::voxelSizeFromStoreMetadata() tier 1: a voxel size the
// document states outright, at the top level only.
const EXPLICIT_KEYS = [
  'voxelsize', 'voxel_size_um', 'voxelSizeUm',
  'pixel_size_um', 'pixelSizeUm', 'resolution_um',
];

const outdir = process.argv[2] ?? join('research', 'raw_metadata');
await mkdir(outdir, { recursive: true });

const get = async (url) => {
  try {
    const res = await fetch(url, { headers: { 'User-Agent': 'voxelscale-guard-research' } });
    if (!res.ok) return { status: res.status, body: null };
    return { status: res.status, body: await res.text() };
  } catch (err) {
    return { status: `error:${err.message}`, body: null };
  }
};

const summary = [];

for (const volume of VOLUMES) {
  const record = { volume, documents: {} };

  for (const name of CANDIDATES) {
    const url = `${BUCKET}/${volume}/${name}`;
    const { status, body } = await get(url);
    const entry = { url, status, found: body !== null };

    if (body !== null) {
      const safe = `${volume.replaceAll('/', '__')}.${name}`;
      await writeFile(join(outdir, safe), body);
      entry.savedAs = join(outdir, safe);

      let doc = null;
      try { doc = JSON.parse(body); } catch { entry.parseError = true; }

      if (doc && typeof doc === 'object' && !Array.isArray(doc)) {
        entry.topLevelKeys = Object.keys(doc);
        // Tier 1: the ungated top-level lookup that vc_render_tifxyz implements.
        entry.tier1 = {};
        for (const key of EXPLICIT_KEYS) {
          if (key in doc) entry.tier1[key] = doc[key];
        }
        entry.tier1Resolves = Object.keys(entry.tier1).length > 0;
        // Tier 2: the ESRF/BM18 acquisition record, with and without the wrapper.
        entry.esrfScanWrapped =
          doc?.scan?.tomo?.acquisition?.detector?.samplePixelSize ?? null;
        entry.esrfRootLevel =
          doc?.tomo?.acquisition?.detector?.samplePixelSize ?? null;
      }
    }
    record.documents[name] = entry;
  }

  summary.push(record);
}

console.log(JSON.stringify(summary, null, 2));
