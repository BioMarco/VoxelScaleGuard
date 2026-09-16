# TEST_PLAN

What was tested, how, and what remains planned. **Executed** means it ran and the
transcript is in `RESULTS.md`. **Planned** means it is designed but blocked.

---

## 1. Strategy

Three layers, because the patch cannot be built here:

| Layer | What it can prove | Built? |
|---|---|---|
| **L1 — Real upstream units, executed** | The schema gap, the value-validation gap, the fragment hazard, and the fixed priority chain | **Executed** |
| **L2 — Pre-patch behaviour, executed** | That the defect exists in the shipped code, not merely in a reading of it | **Executed** |
| **L3 — The patched binary** | That the patch compiles, and that `.zattrs` / TIFF tags really change | **Planned — blocked** (no Qt/OpenCV/vcpkg) |

The point of L1 and L2 is that they are not simulations: they compile the pinned
revision's own `.cpp` files. The control is that upstream's 13-case suite passes
unmodified inside the same test binary. If the harness were paraphrasing, that
suite would be the thing that failed first.

---

## 2. Cases required by the brief, and their status

The brief listed twelve scenarios. All twelve are executed **at the resolution
level**; the last column says where.

| # | Scenario | Status | Where |
|---|---|---|---|
| 1 | Local volume, valid metadata | **Executed** | legacy `meta.json` case; probe row `PHerc0172` |
| 2 | Remote volume, valid metadata | **Executed (partial)** | real remote documents applied to the resolver. The HTTP path itself is exercised through a fetch stub plus live document fetches; `Volume::NewFromUrl`'s own remote path is upstream code, covered by their live tests |
| 3 | Metadata missing | **Executed** | `{"height":10}` → `stated == false`; `resolveVoxelSize` → `Unspecified` |
| 4 | Voxel size zero | **Executed** | pre-patch returns `0.0`; fixed chain rejects |
| 5 | Non-finite or negative | **Executed** | `{0, -1, NaN, +inf}` rejected at every tier; `-3` pre-patch returns `-3.0` |
| 6 | Size specified explicitly | **Executed** | priority test: CLI beats all metadata |
| 7 | Native resolution | **Executed** | `ds_scale = 1` in the probe's patched-side computation; no `2^g` factor |
| 8 | Reduced resolution | **Executed** | unit tests for `ds_scale`; full level-1 case in L3 |
| 9 | Surface genuinely below `min_area_cm` | **Not applicable to this patch** | that gate is in the growth tools, already fixed upstream, untouched here (`ARCHITECTURE.md` §5) |
| 10 | Valid surface wrongly deleted | **Not applicable to this patch** | same; the renderer never deletes output |
| 11 | Rendering with verified physical scale | **Planned** | L3 — needs a real render, inspect `.zattrs` |
| 12 | Rendering with metadata unavailable | **Executed (logic)** / **Planned (artifact)** | logic: `Unspecified` + unit omitted. Artifact: L3 |

Cases 9 and 10 are recorded as **not applicable rather than passed**. Claiming
them would be claiming credit for upstream's work on a code path this patch does
not touch.

---

## 3. L1/L2 — executed tests

Built and run with `pwsh -File harness/build.ps1 -Configuration Release`, then:

```
harness/build/Release/test_upstream_voxel_size_metadata.exe   13 cases / 54 assertions
harness/build/Release/test_render_voxel_size.exe              16 cases / 82 assertions
harness/build/Release/probe_render_voxel_size.exe             before/after on 4 real stores
```

### 3.1 Control — upstream, unmodified

`src/villa/test_voxel_size_metadata.cpp`, copied byte-for-byte. **13 cases / 54
assertions, all pass.** This is the sanity check on the whole harness.

### 3.2 Defect reproduction — the five pre-patch behaviours

| Case | Pins |
|---|---|
| `deployed reader cannot read a modern published store` | returns nothing for an 8.64 µm `metadata.json` store |
| `the legacy store is the only shape the deployed reader reads` | returns 7.91 µm, agreeing with the shared resolver — the control proving the reader is not broken everywhere |
| `the deployed reader ignores the 45.532 um Paris4 store` | returns nothing for 45.532 µm |
| `the deployed reader returns a zero voxel size as a value` | returns `0.0` as if it were a measurement |
| `the deployed reader returns a negative voxel size as a value` | returns `-3.0` as if it were a measurement |

These pass, which is the assertion that the defect is present.

### 3.3 The fixed decision procedure — eleven cases

Sources and priority: CLI > local store > open remote volume > remote fetch >
unresolved; CLI unit conversion for nm/µm/mm/m; refusal of an unknown unit;
refusal of a non-positive or non-finite CLI value; every one of the six
`voxelsize` aliases resolves *and* reports `statedVoxelSize`; a document that
says nothing reports `stated == false` even though the resolver also returns
nothing; unusable candidates rejected at every tier; and a present-but-unusable
local value does not block the remote fallback (the exact review concern).

### 3.4 The two review concerns on PR #1417

| Concern | Test | Outcome |
|---|---|---|
| URL fragments break remote metadata discovery | `the fragment concern is real in joinRemoteUrlPath` | **Confirmed**: the child is appended after the fragment, producing a path that cannot exist. `spec.sourceUrl` gives the correct one |
| A raw locator silently resolves nothing | `resolveRemoteStoreVoxelSize must be given a fragment-free URL` | **Confirmed**: raw locator → no value; parsed `sourceUrl` → 45.532 |
| Invalid local metadata blocks the remote fallback | `an unusable local value does not block the remote fallback` | **Confirmed and addressed** |
| …and is worse than described | the two `returns a … voxel size as a value` cases | **Confirmed**: the reader *returns* `0.0`/`-3.0` rather than merely mis-signalling |

Reachability of the fragment hazard was checked in the source rather than
assumed: the GUI passes `volume->remoteLocator()`, which keeps
`#vc-base-scale=N`. So it is reachable for a rebased remote volume — but it is
**latent in the current code**, which has no remote path at all.

### 3.5 Anti-tamper checks

| Check | Command | Result |
|---|---|---|
| The patch is well-formed and matches the tree exactly | `git apply --check --reverse patch/vc_render_tifxyz.patch` | exit 0 |
| …and is not already applied | `git apply --check patch/vc_render_tifxyz.patch` | exit 1, "does not apply" |
| The old reader is gone | `grep readVolumeVoxelSize` on the patched file | only comments |
| No stale unit reaches the writer | both `writeZarrAttrs` call sites | pass `zarr_voxel_unit` |
| Upstream files were not edited | byte-for-byte copy in `setup.ps1`; never written again | — |

Tests were **not** weakened to pass. Two assertions were corrected during
development because they misstated the pre-patch behaviour — the reader *does*
return `0` and `-3` rather than returning nothing. The correction made the finding
stronger, and `RESULTS.md` §3.2 records it.

---

## 4. L3 — planned, blocked

### 4.1 Build the patched binary

```
cd villa/volume-cartographer
set VCPKG_ROOT=<bootstrapped vcpkg>
cmake --preset windows-msvc
cmake --build build\windows-msvc --target vc_render_tifxyz
```

**Blocked by:** vcpkg absent, Qt/OpenCV/Ceres/CGAL absent, CMake 3.24 installed vs
3.28 required. Multi-GB download and a long source build. **Requires
authorisation; not attempted.**

Acceptance criteria once buildable:

1. `vc_render_tifxyz.cpp` compiles with no new warnings, and clean under
   `ci-strict-warnings-gcc` (`-Werror`).
2. `vc_render_tifxyz --help` still lists `--voxel-size` and `--voxel-unit`
   unchanged.
3. A local legacy store (`PHerc0172` shape) renders **byte-identical** output and
   the same `.zattrs` as before — the regression case.

### 4.2 The before/after artifact test

Using the volume from the report:

```
VOL=s3://vesuvius-challenge-open-data/PHerc0009B/volumes/20250521125136-8.640um-1.2m-116keV-masked.zarr
vc_render_tifxyz -v <cache-dir> --remote-url "$VOL" -g 0 --scale 1 \
    -s <segment.tifxyz> --zarr-output out.zarr --tif-output out_tif
```

| Assertion | Before | Expected after |
|---|---|---|
| log line | `Voxel size: 1.0 (no metadata found; override with --voxel-size)` | `Voxel size (remote volume metadata): 8.64 micrometer` |
| `out.zarr/0/.zattrs` axis unit | `nanometer` | `micrometer` |
| `out.zarr/0/.zattrs` scale | `[1, 1, 1]` | `[1, 8.64, 8.64]` (at `-g 0 --scale 1`) |
| TIFF `XResolution` | absent (`dpi == 0`) | present, ≈ `25400 / 8.64 ≈ 2939.8` px/inch |
| rendered pixels | *baseline* | byte-identical to baseline |

The pixels assertion is the important one: `base_voxel_size` feeds only `tifDpi`
and the `.zattrs` scale (`ROOT_CAUSE_ANALYSIS.md` §6), so a difference there would
indicate the patch broke something it should not have touched.

Verify with `python -c "import json;print(json.load(open('out.zarr/0/.zattrs')))"`
and `tiffinfo out_tif/00.tif`.

### 4.3 The unknown-size artifact test

A volume whose metadata cannot be resolved: assert the run completes, the warning
is emitted, and the `.zattrs` axes carry **no** `unit` — rather than asserting
`1.0 nanometer`.

### 4.4 Regression suite

`ctest --preset ci-release-tests-*`, plus the live-S3 tests
(`core/test/test_volume_live_s3.cpp`, `test_normal_grid_live.cpp`,
`test_pherc0172_live.cpp`) to confirm no metadata behaviour moved.

### 4.5 Recommended addition to upstream's own suite

`core/test/test_volume_live_s3.cpp` currently pins `PHerc0172` — the one catalog
volume the old reader happened to handle. **Pointing that live test at a modern
`metadata.json`-shaped volume is the change that would have caught this class of
bug**, and it is the single highest-value test to add. It is not done here:
changing an existing live test's fixture is a maintainer decision, and the
assertions in that file are pinned to PHerc0172's shape.

An offline equivalent is proposed instead, and is what the harness implements: the
real `PHerc0009B` document, embedded, asserted to resolve to `8.64`.

---

## 5. What would falsify the claims in this project

Recorded so the work can be checked rather than believed:

| Claim | Falsified if |
|---|---|
| The pre-patch reader cannot read a modern store | `readVolumeVoxelSize` returns a value for the embedded `PHerc0009B` document |
| The defect is live at the pinned revision | any of the five DEFECT cases fails when run against that revision |
| The old reader returns unusable values | `{"voxelsize":0}` yields no value rather than `0.0` |
| The declared scale is ×8640 wrong for `PHerc0009B` | `writeZarrAttrs` does not write `baseVoxelSize` as the axis scale, or the unit default is not `nanometer` |
| The patch removes the error | the patched chain returns anything but 8.64 / 45.532 / 2.4 for those stores |
| The patch leaves pixels alone | any geometry or sampling path reads `base_voxel_size` |
| The harness runs real upstream code | upstream's 13-case suite fails in the harness |
| Nothing downstream needs the fallback | any in-tree consumer reads the `.zattrs` unit or requires a non-zero DPI |
