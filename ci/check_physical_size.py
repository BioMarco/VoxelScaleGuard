#!/usr/bin/env python3
"""Check a rendered .zattrs and TIFF against the PHYSICAL size that was asked for.

This is the check the first version of the patch would have failed. It exists
because comparing exit codes, or numbers without their units, cannot see a
number/unit mismatch: `.zattrs` saying 8.64 with unit "nanometer" is a valid
document that denotes 8.64 nm, i.e. 1000x smaller than the 8640 nm the caller
asked for. Both numbers were "right"; only the pair was wrong.

So this script converts everything to micrometres and compares:

    1. `.zattrs`   : declared scale * micrometres-per-declared-unit
    2. TIFF tags   : 25400 / XResolution
    3. the request : the physical size the command line asked for

All three must agree.

Usage:
    check_physical_size.py --zattrs <path> --tiff <file-or-dir> \
        --expect-um <micrometers> [--label NAME]

Exit code 0 if all checks pass, 1 otherwise. Always prints a report.
"""

import argparse
import json
import sys
from pathlib import Path

# Micrometres per one of the unit spellings --voxel-unit accepts. Mirrors
# explicitMicrometerPerVoxel() in vc_render_tifxyz.cpp.
UM_PER_UNIT = {
    "nanometer": 0.001, "nanometre": 0.001, "nm": 0.001,
    "micrometer": 1.0, "micrometre": 1.0, "um": 1.0, "\u00b5m": 1.0,
    "millimeter": 1000.0, "millimetre": 1000.0, "mm": 1000.0,
    "meter": 1e6, "metre": 1e6, "m": 1e6,
}

# Every case the run must satisfy: the physical size asked for, and what the two
# output formats must independently be found to denote. Kept here (rather than only
# in the workflow) so the expectation is reviewable and testable.
#
# `expect_um` is the physical size in micrometres. Note the first entry: 8640 nm is
# the case the first version of the patch got wrong, declaring 8.64 nm.
PHYSICAL_CASES = [
    ("nm-8640",  8640.0,     "nanometer",  8.64),
    ("nm-7910",  7910.0,     "nanometer",  7.91),
    ("um-8.64",  8.64,       "micrometer", 8.64),
    ("mm-0.00864", 0.00864,  "millimeter", 8.64),
    ("m-0.00000864", 0.00000864, "meter",  8.64),
]

failures = []
notes = []


def say(line=""):
    print(line)


def check(name, ok, detail=""):
    say(f"    {'PASS' if ok else 'FAIL'}  {name}" + (f"  -- {detail}" if detail else ""))
    if not ok:
        failures.append(name)
    return ok


def read_zarr_scale(attrs_path: Path):
    """Return (value, unit) of the level-0 in-plane scale, or (None, reason)."""
    if not attrs_path.is_file():
        return None, None, "no .zattrs written"
    try:
        doc = json.loads(attrs_path.read_text())
    except Exception as exc:  # noqa: BLE001
        return None, None, f"unparseable .zattrs: {exc}"

    mults = doc.get("multiscales") or []
    if not mults:
        return None, None, "no multiscales in .zattrs"
    ms = mults[0]
    units = [a.get("unit") for a in ms.get("axes", [])]
    if len(set(units)) != 1:
        return None, None, f"axes carry mixed units: {units}"
    unit = units[0]
    datasets = ms.get("datasets") or []
    if not datasets:
        return None, None, "no datasets in .zattrs"
    for ct in datasets[0].get("coordinateTransformations", []) or []:
        if ct.get("type") == "scale":
            scale = ct.get("scale") or []
            if not scale:
                return None, None, "empty scale"
            # [z, y, x]; the in-plane value is what the caller's size describes
            # at --scale 1.
            return float(scale[1]), unit, None
    return None, None, "no scale transformation"


def read_tiff_dpi(tiff_arg: Path):
    """Return (x_resolution, unit_code) from the first TIFF found."""
    files = []
    if tiff_arg.is_dir():
        files = sorted(tiff_arg.glob("*.tif"))
    elif tiff_arg.is_file():
        files = [tiff_arg]
    if not files:
        return None, None, "no TIFF produced"
    path = files[0]
    try:
        from PIL import Image
    except Exception:  # noqa: BLE001
        # A setup gap, not a property of the output. Reported as such so it can
        # never be mistaken for "the TIFF has no resolution".
        return None, None, "SETUP: Pillow is not installed, cannot read TIFF tags"
    with Image.open(path) as im:
        tags = dict(im.tag_v2)
        xres = tags.get(282)
        unit = tags.get(296)
    return xres, unit, None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--zattrs", required=True)
    ap.add_argument("--tiff", required=True)
    ap.add_argument("--expect-um", type=float)
    ap.add_argument("--label", default="case")
    ap.add_argument("--tolerance", type=float, default=1e-6)
    ap.add_argument(
        "--expect-unknown", action="store_true",
        help="The size is expected to be UNKNOWN: neither output may declare one. "
             "Asserts the multiscales block and the TIFF resolution tag are absent, "
             "which is what the renderer promises on stderr in that case.")
    args = ap.parse_args()

    if not args.expect_unknown and args.expect_um is None:
        ap.error("--expect-um is required unless --expect-unknown is given")

    say()
    if args.expect_unknown:
        say(f"=== {args.label}: expected UNKNOWN physical size ===")

        attrs = Path(args.zattrs)
        if attrs.is_file():
            try:
                doc = json.loads(attrs.read_text())
                check("zattrs declares no multiscales block", "multiscales" not in doc,
                      f"keys: {sorted(doc)}")
            except Exception as exc:  # noqa: BLE001
                check("zattrs is parseable", False, str(exc))
        else:
            check("no .zattrs written at all", True, "nothing that could declare a scale")

        xres, _unit, err = read_tiff_dpi(Path(args.tiff))
        if err and err.startswith("SETUP:"):
            check("TIFF tags could be read", False, err)
        else:
            check("TIFF declares no resolution", xres in (None, 0), f"XResolution={xres}")
        return 1 if failures else 0

    say(f"=== {args.label}: expected {args.expect_um:g} um per voxel ===")

    # --- 1. .zattrs ---------------------------------------------------------
    value, unit, err = read_zarr_scale(Path(args.zattrs))
    if err:
        check("zattrs declares a scale and a unit", False, err)
        um_from_zarr = None
    else:
        say(f"    declared      : {value:g} {unit}")
        per_unit = UM_PER_UNIT.get(unit)
        if per_unit is None:
            check("declared unit is one we can interpret", False, f"unknown unit {unit!r}")
            um_from_zarr = None
        else:
            um_from_zarr = value * per_unit
            say(f"    .zattrs means : {um_from_zarr:g} um")
            check("zattrs physical size matches the request",
                  abs(um_from_zarr - args.expect_um) <= args.tolerance * max(1.0, args.expect_um),
                  f"{um_from_zarr:g} vs {args.expect_um:g} um")

    # --- 2. TIFF ------------------------------------------------------------
    xres, unit_code, err = read_tiff_dpi(Path(args.tiff))
    um_from_tiff = None
    if err:
        # Distinguish "the tooling is not installed" from "the output is wrong":
        # only the latter is a finding about the renderer.
        check("TIFF tags could be read", not err.startswith("SETUP:"), err)
        if err.startswith("SETUP:"):
            notes.append(err)
    elif xres in (None, 0):
        # A zero/absent resolution is what the renderer writes when it has no
        # usable size at all. With an explicit size that is a failure.
        check("TIFF carries a resolution", False, "XResolution absent")
    else:
        # ResolutionUnit 2 == inch, 3 == centimetre.
        per_inch = 25400.0 if unit_code in (None, 2) else 2540.0
        um_from_tiff = per_inch / float(xres)
        say(f"    TIFF          : XResolution={xres} unit={unit_code} -> {um_from_tiff:g} um/pixel")
        check("TIFF physical size matches the request",
              abs(um_from_tiff - args.expect_um) <= args.tolerance * max(1.0, args.expect_um),
              f"{um_from_tiff:g} vs {args.expect_um:g} um")

    # --- 3. the two must agree ---------------------------------------------
    if um_from_zarr is not None and um_from_tiff is not None:
        check("zattrs and TIFF describe the same physical size",
              abs(um_from_zarr - um_from_tiff) <= args.tolerance * max(1.0, args.expect_um),
              f"{um_from_zarr:g} vs {um_from_tiff:g} um")

    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
