#!/usr/bin/env python3
"""Self-test for ci/compare_render_outputs.py using synthetic fixtures.

The comparison script runs once, inside CI, on outputs that cannot be
regenerated on demand without another 30-minute run. A crash or a wrong key name
in it would throw away that run's evidence. So its parsing and its
same/different verdicts are exercised here, locally, against fixtures built to
match the documented output shapes -- including the two cases that matter:

  * pixels identical but metadata corrected  -> regression check must pass
  * pixels different                         -> must be reported as different

This tests the REPORTER, not the renderer. It proves nothing about
vc_render_tifxyz; it only proves the report reads what the renderer writes.

Usage: python ci/selftest_compare.py
"""

import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SCRIPT = ROOT / "ci" / "compare_render_outputs.py"

try:
    from PIL import Image
except Exception as exc:  # noqa: BLE001
    print(f"Pillow required for this self-test: {exc}")
    sys.exit(2)


def write_zarr(out: Path, tag: str, unit: str, scale: list) -> None:
    """Shape mirrors what vc_render_tifxyz writes: <tag>.zarr/<g>/.zattrs."""
    level = out / f"{tag}.zarr" / "0"
    level.mkdir(parents=True, exist_ok=True)
    (level / ".zarray").write_text(json.dumps({"zarr_format": 2, "shape": [1, 128, 128]}))
    (level / ".zattrs").write_text(json.dumps({
        "multiscales": [{
            "axes": [{"name": "z", "type": "space", "unit": unit},
                     {"name": "y", "type": "space", "unit": unit},
                     {"name": "x", "type": "space", "unit": unit}],
            "datasets": [{
                "path": "0",
                "coordinateTransformations": [
                    {"type": "scale", "scale": scale},
                ],
            }],
        }],
    }))


def write_tif(out: Path, tag: str, pixel_value: int, dpi: float | None) -> None:
    d = out / f"{tag}.tif"
    d.mkdir(parents=True, exist_ok=True)
    im = Image.new("L", (128, 128), color=pixel_value)
    kwargs = {}
    if dpi is not None:
        kwargs["dpi"] = (dpi, dpi)
    im.save(d / "000000.tif", **kwargs)


def build(out: Path, *, baseline_px: int, patched_px: int,
          baseline_unit: str, patched_unit: str,
          baseline_scale: list, patched_scale: list,
          baseline_dpi: float | None, patched_dpi: float | None) -> None:
    write_zarr(out, "0009B-baseline", baseline_unit, baseline_scale)
    write_zarr(out, "0009B-patched", patched_unit, patched_scale)
    write_zarr(out, "0172-baseline", baseline_unit, baseline_scale)
    write_zarr(out, "0172-patched", patched_unit, patched_scale)
    write_tif(out, "0009B-baseline", baseline_px, baseline_dpi)
    write_tif(out, "0009B-patched", patched_px, patched_dpi)
    write_tif(out, "0172-baseline", baseline_px, baseline_dpi)
    write_tif(out, "0172-patched", patched_px, patched_dpi)


def write_logs(ci: Path, tag: str, line: str, exit_code: int = 0) -> None:
    ci.mkdir(parents=True, exist_ok=True)
    (ci / f"render-{tag}.log").write_text(line + "\n")
    (ci / f"render-{tag}.exit").write_text(str(exit_code))


def run_case(name: str, expect: str, **build_kwargs) -> bool:
    tmp = Path(tempfile.mkdtemp(prefix="vsg-selftest-"))
    try:
        out, ci = tmp / "out", tmp / "ci-out"
        build(out, **build_kwargs)
        for tag, line in (
            ("0009B-baseline", "Voxel size: 1.0 (no metadata found; override with --voxel-size)"),
            ("0009B-patched", "Voxel size (remote volume metadata): 8.64 micrometer"),
            ("0172-baseline", "Voxel size (from volume metadata): 7.91 nanometer"),
            ("0172-patched", "Voxel size (remote volume metadata): 7.91 micrometer"),
        ):
            write_logs(ci, tag, line)
        proc = subprocess.run([sys.executable, str(SCRIPT), str(out), str(ci)],
                              capture_output=True, text=True)
        ok = proc.returncode == 0 and expect in proc.stdout
        print(f"{'PASS' if ok else 'FAIL'}  {name}"
              + ("" if ok else f"  (expected {expect!r} in output; rc={proc.returncode})"))
        if not ok:
            print(proc.stdout[-2500:])
            print(proc.stderr[-1500:])
        return ok

    finally:
        shutil.rmtree(tmp, ignore_errors=True)


print("=" * 70)
print("compare_render_outputs.py self-test (synthetic fixtures)")
print("=" * 70)

results = []

# The real scenario: physical metadata corrected, pixels untouched.
results.append(run_case(
    "pixels identical, metadata corrected -> regression check reports identical",
    "DECODED PIXELS IDENTICAL: YES",
    baseline_px=42, patched_px=42,
    baseline_unit="nanometer", patched_unit="micrometer",
    baseline_scale=[1.0, 1.0, 1.0], patched_scale=[1.0, 8.64, 8.64],
    baseline_dpi=None, patched_dpi=25400.0 / 8.64,
))

# Metadata corrected AND the pixel data changed: must NOT be reported as identical.
results.append(run_case(
    "pixels differ -> must report pixels NOT identical",
    "DECODED PIXELS IDENTICAL: NO",
    baseline_px=42, patched_px=43,
    baseline_unit="nanometer", patched_unit="micrometer",
    baseline_scale=[1.0, 1.0, 1.0], patched_scale=[1.0, 8.64, 8.64],
    baseline_dpi=None, patched_dpi=25400.0 / 8.64,
))

# Nothing changed at all: pixels and bytes both identical.
results.append(run_case(
    "no change at all -> pixels identical and bytes identical",
    "raw file bytes identical: YES",
    baseline_px=7, patched_px=7,
    baseline_unit="micrometer", patched_unit="micrometer",
    baseline_scale=[1.0, 1.0, 1.0], patched_scale=[1.0, 1.0, 1.0],
    baseline_dpi=2939.0, patched_dpi=2939.0,
))

# The script must survive a render that produced nothing at all.
tmp = Path(tempfile.mkdtemp(prefix="vsg-selftest-"))
try:
    out, ci = tmp / "out", tmp / "ci-out"
    out.mkdir(parents=True)
    ci.mkdir(parents=True)
    write_logs(ci, "0009B-baseline", "Error: opening local zarr failed", exit_code=1)
    write_logs(ci, "0009B-patched", "Error: opening local zarr failed", exit_code=1)
    proc = subprocess.run([sys.executable, str(SCRIPT), str(out), str(ci)],
                          capture_output=True, text=True)
    ok = proc.returncode == 0 and "cannot compare" in proc.stdout
    print(f"{'PASS' if ok else 'FAIL'}  empty outputs -> script reports, does not crash")
    if not ok:
        print(proc.stdout[-2000:], proc.stderr[-1000:])
    results.append(ok)
finally:
    shutil.rmtree(tmp, ignore_errors=True)

print()
if all(results):
    print(f"SELFTEST OK ({len(results)}/{len(results)})")
    sys.exit(0)
print(f"SELFTEST FAILED ({sum(results)}/{len(results)})")
sys.exit(1)
