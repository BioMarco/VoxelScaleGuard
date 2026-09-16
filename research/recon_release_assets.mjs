// Which villa release assets exist, and are they built from the commit this
// project pins? Read-only, metadata only: nothing is downloaded.
//
// Purpose: `RESUME.md` §5 claims a prebuilt VC3D Windows package exists, built from
// exactly the pinned commit, and that it bundles the `vc_*` CLI tools. The first
// half is checkable here; the second half is only checkable by downloading and
// unpacking, so this script deliberately stops at metadata.
//
// Usage:
//   node research/recon_release_assets.mjs            # `latest` release only
//   node research/recon_release_assets.mjs --all      # first 6 releases

const PINNED = '757f70c0140a4cfbbbd44975ef09558444b96980';
const API = 'https://api.github.com/repos/ScrollPrize/villa/releases';

const all = process.argv.includes('--all');

const res = await fetch(API, {
  headers: { 'User-Agent': 'voxelscale-guard-recon', Accept: 'application/vnd.github+json' },
});
if (!res.ok) {
  console.error(`HTTP ${res.status} ${res.statusText} — no data recorded.`);
  process.exit(1);
}

const releases = await res.json();
for (const rel of releases.slice(0, all ? 6 : 1)) {
  console.log(`RELEASE ${rel.tag_name} | published ${rel.published_at} | name "${rel.name}"`);
  if (rel.body && rel.body.includes(PINNED)) {
    console.log(`   body names the PINNED commit (${PINNED.slice(0, 7)})`);
  }
  for (const a of rel.assets) {
    console.log(`   ASSET ${a.name}  ${a.size} bytes (${(a.size / 1048576).toFixed(1)} MB)`);
  }
}
