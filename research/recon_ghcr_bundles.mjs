// Can the prebuilt dependency bundles that upstream CI uses be fetched from here,
// and how large are they? Read-only: resolves manifests, downloads no blobs.
//
// Purpose: `RESULTS.md` §8.4 Blocker C. Upstream's Windows route pulls
// `ghcr.io/scrollprize/vc3d-deps/windows:sha-d84dfa07…` with `oras`, authenticated
// by the CI runner's GitHub token. If that package is not anonymously readable,
// then the "cheapest upstream-supported route" is not merely expensive — it is
// unavailable without credentials, which this project must not handle.
//
// The runtime image is probed for contrast: it is documented as publicly pullable,
// which makes the difference a fact about the package rather than about ghcr.io.
//
// Usage:
//   node research/recon_ghcr_bundles.mjs

const MANIFEST_ACCEPT = [
  'application/vnd.oci.image.manifest.v1+json',
  'application/vnd.oci.image.index.v1+json',
  'application/vnd.docker.distribution.manifest.v2+json',
  'application/vnd.docker.distribution.manifest.list.v2+json',
  'application/vnd.oci.artifact.manifest.v1+json',
].join(', ');

const DEPS_REF = 'sha-d84dfa0745dd2debbcec2126117878b241ffdb7f';

const TARGETS = [
  { repo: 'scrollprize/vc3d-deps', tag: DEPS_REF, kind: 'prebuilt dependency bundle' },
  { repo: 'scrollprize/villa/volume-cartographer', tag: 'edge', kind: 'built runtime image' },
];

async function anonymousToken(repo) {
  const r = await fetch(
    `https://ghcr.io/token?scope=repository:${repo}:pull&service=ghcr.io`,
    { headers: { 'User-Agent': 'voxelscale-guard-recon' } },
  );
  if (!r.ok) throw new Error(`anonymous token request returned HTTP ${r.status}`);
  const body = await r.json();
  if (!body.token) throw new Error('token response contained no token');
  return body.token;
}

async function describe(repo, tag, kind) {
  const label = `${repo}:${tag}`;
  console.log(`\n--- ${kind}`);
  console.log(label);
  let token;
  try {
    token = await anonymousToken(repo);
  } catch (e) {
    console.log(`   anonymous pull: DENIED — ${e.message}`);
    console.log('   => size and contents cannot be measured from here.');
    return;
  }
  const r = await fetch(`https://ghcr.io/v2/${repo}/manifests/${tag}`, {
    headers: { Authorization: `Bearer ${token}`, Accept: MANIFEST_ACCEPT, 'User-Agent': 'voxelscale-guard-recon' },
  });
  if (!r.ok) {
    console.log(`   anonymous pull: DENIED — manifest HTTP ${r.status}`);
    return;
  }
  const m = await r.json();
  const layers = m.layers || [];
  const total = layers.reduce((s, l) => s + (l.size || 0), 0);
  console.log(`   anonymous pull: ALLOWED — ${m.mediaType || '(no mediaType)'}`);
  if (layers.length) {
    console.log(`   ${layers.length} layer(s), ${(total / 1048576).toFixed(1)} MB compressed`);
  } else if (m.manifests) {
    console.log(`   ${m.manifests.length} platform manifest(s): ${m.manifests
      .map((x) => `${x.platform?.os || '?'}/${x.platform?.architecture || '?'}`).join(', ')}`);
    // Resolve the first real platform so the size is reported, not just the shape.
    const first = m.manifests.find((x) => x.platform?.os && x.platform.os !== 'unknown');
    if (!first) return;
    const r2 = await fetch(`https://ghcr.io/v2/${repo}/manifests/${first.digest}`, {
      headers: { Authorization: `Bearer ${token}`, Accept: MANIFEST_ACCEPT, 'User-Agent': 'voxelscale-guard-recon' },
    });
    if (!r2.ok) {
      console.log(`   ${first.platform.os}/${first.platform.architecture}: manifest HTTP ${r2.status}`);
      return;
    }
    const m2 = await r2.json();
    const l2 = m2.layers || [];
    const t2 = l2.reduce((s, l) => s + (l.size || 0), 0);
    console.log(`   ${first.platform.os}/${first.platform.architecture}: ${l2.length} layer(s), ` +
      `${(t2 / 1048576).toFixed(1)} MB compressed`);
  }
}

for (const t of TARGETS) {
  await describe(t.repo, t.tag, t.kind);
}
