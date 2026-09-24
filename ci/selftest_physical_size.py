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


def run_unknown(label: str, *, zattrs_mode: str, resolution: bool,
                expect_ok: bool) -> bool:
    """Exercise --expect-unknown: the branch that emits the warning.

    zattrs_mode selects which .zattrs to synthesise:
      "unitless_relative" -- the correct output: multiscales present, no units,
                             relative pyramid scaling.
      "no_multiscales"    -- the earlier revision's output: the block removed.
      "fabricated"        -- the original defect: a placeholder 1.0 nanometre.
      "unit_without_size" -- a unit declared with no physical measurement.
      "wrong_relative"    -- unitless but the level factors are not the pyramid's.
    """
    tmp = Path(tempfile.mkdtemp(prefix="vsg-unknown-"))
    try:
        zattrs = tmp / "out.zarr" / ".zattrs"
        tiff = tmp / "out.tif" / "00.tif"
        zattrs.parent.mkdir(parents=True, exist_ok=True)

        def axes(with_unit: bool):
            return [{"name": n, "type": "space", **({"unit": "micrometer"} if with_unit else {})}
                    for n in ("z", "y", "x")]

        def datasets(z_factor):
            return [{
                "path": str(l),
                "coordinateTransformations": [
                    {"type": "scale",
                     "scale": [1.0, (2.0 ** l) * z_factor, (2.0 ** l) * z_factor]},
                ],
            } for l in range(6)]

        if zattrs_mode == "unitless_relative":
            doc = {"multiscales": [{"version": "0.4", "name": "render",
                                    "axes": axes(False), "datasets": datasets(1.0)}]}
        elif zattrs_mode == "no_multiscales":
            doc = {"source_zarr": "x", "canvas_size": [4, 4]}
        elif zattrs_mode == "fabricated":
            doc = {"multiscales": [{"version": "0.4", "name": "render",
                                    "axes": axes(True), "datasets": datasets(1.0)}]}
        elif zattrs_mode == "unit_without_size":
            doc = {"multiscales": [{"version": "0.4", "name": "render",
                                    "axes": axes(True), "datasets": datasets(1.0)}]}
        elif zattrs_mode == "wrong_relative":
            doc = {"multiscales": [{"version": "0.4", "name": "render",
                                    "axes": axes(False), "datasets": datasets(3.0)}]}
        else:
            raise AssertionError(zattrs_mode)
        zattrs.write_text(json.dumps(doc))

        write_tiff(tiff, 8.64, with_resolution=resolution)
        proc = subprocess.run(
            [sys.executable, str(CHECKER), "--zattrs", str(zattrs), "--tiff", str(tiff),
             "--expect-unknown", "--label", label],
            capture_output=True, text=True)
        ok = (proc.returncode == 0) == expect_ok
        print(f"{'PASS' if ok else 'FAIL'}  {label} (expected "
              f"{'accept' if expect_ok else 'reject'}, got rc={proc.returncode})")
        if not ok:
            print(proc.stdout[-2000:])
        return ok
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


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

# 7. The UNKNOWN case, correct output: no unit anywhere, no resolution tag, and the
#    multiscales block still present with relative pyramid scaling -> accepted.
#    (Until the review, this script instead required the multiscales block to be
#    ABSENT; that expectation was the defect, not the fix.)
results.append(run_unknown("unknown size: unitless relative scaling, no resolution",
                           zattrs_mode="unitless_relative", resolution=False,
                           expect_ok=True))

# 8. THE REVIEWED DEFECT: the multiscales block removed entirely -> rejected, because
#    a reader can no longer discover the image as a multiscale image.
results.append(run_unknown("unknown size but the multiscales block was removed -> rejected",
                           zattrs_mode="no_multiscales", resolution=False,
                           expect_ok=False))

# 9. A unit declared with no physical measurement -> rejected. That is the invented
#    measurement the original patch removed.
results.append(run_unknown("unknown size but a physical unit was declared -> rejected",
                           zattrs_mode="unit_without_size", resolution=False,
                           expect_ok=False))

# 10. Unitless but the level factors are not the pyramid's -> rejected. Relative
#     scaling has to be the real one, not a placeholder.
results.append(run_unknown("unknown size but the relative scales are wrong -> rejected",
                           zattrs_mode="wrong_relative", resolution=False,
                           expect_ok=False))

# 11. Likewise if a resolution tag survives.
results.append(run_unknown("unknown size but a resolution tag -> rejected",
                           zattrs_mode="unitless_relative", resolution=True,
                           expect_ok=False))

print()
if all(results):
    print(f"SELFTEST OK ({sum(results)}/{len(results)})")
    sys.exit(0)
print(f"SELFTEST FAILED ({sum(results)}/{len(results)})")
sys.exit(1)
