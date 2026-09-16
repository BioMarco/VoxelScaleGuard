#!/usr/bin/env python3
"""Compare the baseline and patched vc_render_tifxyz runs.

Reads what the two binaries actually wrote and prints a report to stdout. It
asserts nothing by itself: it states measurements, and the workflow log is the
record. The point is to separate three questions that must not be conflated:

  1. did both binaries run and produce output?
  2. did the DECLARED PHYSICAL SCALE change, and is the patched value right?
  3. are the rendered PIXELS unchanged?

(3) is the regression check. It is a byte comparison of the pixel arrays, not a
file-size comparison, because two different renderings can coincidentally be the
same size.

Usage: compare_render_outputs.py <out-dir> <ci-out-dir>
"""

import hashlib
import json
import re
import sys
from pathlib import Path

OUT = Path(sys.argv[1] if len(sys.argv) > 1 else "out")
CI = Path(sys.argv[2] if len(sys.argv) > 2 else "ci-out")

try:
    from PIL import Image
    HAVE_PIL = True
except Exception:
    HAVE_PIL = False


def rule(title):
    print()
    print("=" * 78)
    print(title)
    print("=" * 78)


def show(label, value):
    print(f"  {label:<34} {value}")


def log_voxel_line(tag):
    """The one log line that states what the renderer resolved."""
    path = CI / f"render-{tag}.log"
    if not path.exists():
        return "(no log)"
    hits = [ln.strip() for ln in path.read_text(errors="replace").splitlines()
            if "Voxel size" in ln or "physical scale is unknown" in ln]
    return " || ".join(hits) if hits else "(no voxel-size line)"


def exit_code(tag):
    path = CI / f"render-{tag}.exit"
    return path.read_text().strip() if path.exists() else "?"


def read_zattrs(tag):
    """OME-Zarr axis metadata written by the renderer.

    The renderer writes <tag>.zarr/0/.zarray plus a .zattrs beside it. Which
    level carries the physical scale depends on the --group-idx used, so search
    for .zattrs files and report all of them.
    """
    root = OUT / f"{tag}.zarr"
    found = []
    if not root.is_dir():
        return found, "(no zarr output)"
    for attrs in sorted(root.rglob(".zattrs")):
        try:
            found.append((attrs.relative_to(root).as_posix(), json.loads(attrs.read_text())))
        except Exception as exc:  # noqa: BLE001 - report, never crash the report
            found.append((attrs.relative_to(root).as_posix(), f"unparseable: {exc}"))
    return found, None


def describe_multiscales(doc):
    """Pull (unit, scale) pairs out of an OME-Zarr multiscales document."""
    out = []
    if not isinstance(doc, dict):
        return out
    for ms in doc.get("multiscales", []) or []:
        axes = ms.get("axes", []) or []
        units = [a.get("unit") for a in axes]
        for ds in ms.get("datasets", []) or []:
            for ct in ds.get("coordinateTransformations", []) or []:
                if ct.get("type") == "scale":
                    out.append((ds.get("path"), units, ct.get("scale")))
    return out


def tif_info(tag):
    """TIFF tags straight from the file, no external tool needed."""
    root = OUT / f"{tag}.tif"
    if not root.is_dir():
        return []
    rows = []
    for tif in sorted(root.glob("*.tif")):
        data = tif.read_bytes()
        row = {"file": tif.name, "bytes": len(data),
               "sha256": hashlib.sha256(data).hexdigest()}
        if HAVE_PIL:
            try:
                with Image.open(tif) as im:
                    row["size"] = im.size
                    row["mode"] = im.mode
                    tags = dict(im.tag_v2)
                    row["XResolution"] = tags.get(282)
                    row["YResolution"] = tags.get(283)
                    row["ResolutionUnit"] = tags.get(296)
                    row["pixels_sha256"] = hashlib.sha256(
                        im.tobytes()).hexdigest()
            except Exception as exc:  # noqa: BLE001
                row["error"] = str(exc)
        rows.append(row)
    return rows


TAGS = ["0009B-baseline", "0009B-patched", "0172-baseline", "0172-patched"]

rule("1. Did each binary run, and what did it resolve?")
for tag in TAGS:
    show(f"{tag}: exit code", exit_code(tag))
    show(f"{tag}: voxel-size line", log_voxel_line(tag))

rule("2. OME-Zarr .zattrs written by each run")
for tag in TAGS:
    docs, note = read_zattrs(tag)
    print(f"\n--- {tag}")
    if note:
        print(f"    {note}")
    for rel, doc in docs:
        print(f"    .zattrs at {rel}")
        for path, units, scale in describe_multiscales(doc):
            print(f"        dataset {path}: units={units} scale={scale}")
        if not describe_multiscales(doc):
            print(f"        (no multiscales) raw: {json.dumps(doc)[:300]}")

rule("3. TIFF tags written by each run")
if not HAVE_PIL:
    print("  Pillow unavailable: showing byte digests only.")
for tag in TAGS:
    print(f"\n--- {tag}")
    rows = tif_info(tag)
    if not rows:
        print("    (no TIFF output)")
    for row in rows:
        for k, v in row.items():
            show(f"{row['file']} {k}", v)

rule("4. Regression: are the pixels unchanged between baseline and patched?")


def pixels(tag):
    root = OUT / f"{tag}.tif"
    if not root.is_dir():
        return None
    out = {}
    for tif in sorted(root.glob("*.tif")):
        out[tif.name] = hashlib.sha256(tif.read_bytes()).hexdigest()
    return out


for family in ("0009B", "0172"):
    a = pixels(f"{family}-baseline")
    b = pixels(f"{family}-patched")
    print(f"\n--- {family}")
    if a is None or b is None:
        print("    cannot compare: one side produced no TIFF")
        continue
    if not a:
        print("    no TIFF files on either side")
        continue
    print(f"    files baseline={sorted(a)} patched={sorted(b)}")
    identical = a == b
    print(f"    byte-identical TIFFs: {'YES' if identical else 'NO'}")
    if not identical:
        for name in sorted(set(a) | set(b)):
            print(f"      {name}: baseline={a.get(name)} patched={b.get(name)}")

rule("5. Log error/warning lines (both binaries, both volumes)")
for tag in TAGS:
    path = CI / f"render-{tag}.log"
    if not path.exists():
        print(f"\n--- {tag}: (no log)")
        continue
    lines = path.read_text(errors="replace").splitlines()
    bad = [ln for ln in lines
           if re.search(r"\b(error|warning|fail|unknown)\b", ln, re.I)]
    print(f"\n--- {tag}: {len(bad)} line(s)")
    for ln in bad[:25]:
        print("    " + ln.strip())

print()
print("Report complete.")
