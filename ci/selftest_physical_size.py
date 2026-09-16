#!/usr/bin/env python3
"""Self-test for ci/check_physical_size.py, using synthetic fixtures.

Why this exists: the checker runs once per case inside a CI job that costs a full
dependency install and build. A bug in the checker would either pass everything
(bad) or fail the run for the wrong reason (also bad), and either way would waste
the run. So its verdicts are exercised here first, against fixtures built to the
documented shapes.

Critically, it asserts that the checker REJECTS the exact defect this work is
about: a micrometre number labelled "nanometer".

Usage: python ci/selftest_physical_size.py
"""

import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CHECKER = ROOT / "ci" / "check_physical_size.py"

try:
    from PIL import Image
except Exception as exc:  # noqa: BLE001
    print(f"Pillow required for this self-test: {exc}")
    sys.exit(2)


def write_zattrs(path: Path, value: float, unit: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps({
        "multiscales": [{
            "axes": [{"name": "z", "type": "space", "unit": unit},
                     {"name": "y", "type": "space", "unit": unit},
                     {"name": "x", "type": "space", "unit": unit}],
            "datasets": [{
                "path": "0",
                "coordinateTransformations": [{"type": "scale", "scale": [value, value, value]}],
            }],
        }],
    }))


def write_tiff(path: Path, um_per_pixel: float, *, with_resolution: bool = True) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    im = Image.new("L", (16, 16), color=1)
    kwargs = {}
    if with_resolution:
        kwargs["dpi"] = (25400.0 / um_per_pixel, 25400.0 / um_per_pixel)
    im.save(path, **kwargs)


def run(label: str, *, um: float, zarr_value: float, zarr_unit: str,
        tiff_um: float, with_resolution: bool = True, expect_ok: bool) -> bool:
    tmp = Path(tempfile.mkdtemp(prefix="vsg-phys-"))
    try:
        zattrs = tmp / "out.zarr" / ".zattrs"
        tiff = tmp / "out.tif" / "00.tif"
        write_zattrs(zattrs, zarr_value, zarr_unit)
        write_tiff(tiff, tiff_um, with_resolution=with_resolution)
        proc = subprocess.run(
            [sys.executable, str(CHECKER), "--zattrs", str(zattrs), "--tiff", str(tiff),
             "--expect-um", str(um), "--label", label],
            capture_output=True, text=True)
        ok = (proc.returncode == 0) == expect_ok
        print(f"{'PASS' if ok else 'FAIL'}  {label} (expected "
              f"{'accept' if expect_ok else 'reject'}, got rc={proc.returncode})")
        if not ok:
            print(proc.stdout[-2000:])
        return ok
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


print("=" * 72)
print("check_physical_size.py self-test (synthetic fixtures)")
print("=" * 72)

results = []

# 1. The correct outcome for the case the patch got wrong.
results.append(run("8640 nm declared as 8640 nanometer", um=8.64,
                   zarr_value=8640.0, zarr_unit="nanometer", tiff_um=8.64, expect_ok=True))

# 2. THE DEFECT: micrometre number under a nanometre label.
results.append(run("DEFECT: 8.64 labelled nanometer (should be rejected)", um=8.64,
                   zarr_value=8.64, zarr_unit="nanometer", tiff_um=8.64, expect_ok=False))

# 3. The other unit spellings, all denoting 8.64 um.
results.append(run("8.64 micrometer", um=8.64, zarr_value=8.64,
                   zarr_unit="micrometer", tiff_um=8.64, expect_ok=True))
results.append(run("0.00864 millimeter", um=8.64, zarr_value=0.00864,
                   zarr_unit="millimeter", tiff_um=8.64, expect_ok=True))
results.append(run("0.00000864 meter", um=8.64, zarr_value=0.00000864,
                   zarr_unit="meter", tiff_um=8.64, expect_ok=True))

# 4. .zattrs right but TIFF wrong -> must be rejected (they must agree).
results.append(run("zattrs right, TIFF 1000x off", um=8.64, zarr_value=8640.0,
                   zarr_unit="nanometer", tiff_um=0.00864, expect_ok=False))

# 5. TIFF right but .zattrs wrong -> also rejected.
results.append(run("TIFF right, zattrs 1000x off", um=8.64, zarr_value=0.00864,
                   zarr_unit="nanometer", tiff_um=8.64, expect_ok=False))

# 6. No resolution tag at all, when one was requested -> rejected.
results.append(run("TIFF carries no resolution", um=8.64, zarr_value=8640.0,
                   zarr_unit="nanometer", tiff_um=8.64, with_resolution=False,
                   expect_ok=False))

print()
if all(results):
    print(f"SELFTEST OK ({sum(results)}/{len(results)})")
    sys.exit(0)
print(f"SELFTEST FAILED ({sum(results)}/{len(results)})")
sys.exit(1)
