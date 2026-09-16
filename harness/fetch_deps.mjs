// Download the header-only build dependencies for the harness.
// Node's fetch works in this environment where Invoke-WebRequest fails TLS.
//
// Usage: node harness/fetch_deps.mjs

import { mkdir, writeFile } from 'node:fs/promises';
import { dirname, join } from 'node:path';

const HERE = dirname(new URL(import.meta.url).pathname.replace(/^\/([A-Za-z]:)/, '$1'));

const DEPS = [
  ['third_party/nlohmann/json.hpp',
   'https://raw.githubusercontent.com/nlohmann/json/v3.11.3/single_include/nlohmann/json.hpp'],
  ['third_party/doctest/doctest/doctest.h',
   'https://raw.githubusercontent.com/doctest/doctest/v2.4.11/doctest/doctest.h'],
];

for (const [rel, url] of DEPS) {
  const dest = join(HERE, rel);
  await mkdir(dirname(dest), { recursive: true });
  const res = await fetch(url, { headers: { 'User-Agent': 'voxelscale-guard' } });
  if (!res.ok) throw new Error(`${url} -> HTTP ${res.status}`);
  const body = await res.text();
  await writeFile(dest, body);
  console.log(`[deps] ${rel}  ${body.length} bytes`);
}
