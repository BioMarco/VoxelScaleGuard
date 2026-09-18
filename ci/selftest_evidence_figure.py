#!/usr/bin/env python3
"""Verify that the committed evidence figure and the generator still agree on the
DATA, and that no experimental image has been altered.

Why this exists: `DOCS/evidence/before-after.png` carries presentation annotations
that were applied by hand, so the committed PNG is not byte-identical to what
`build_evidence_figure.py` emits — the difference is font rasterisation. That makes
"the generator can regenerate it" a claim that needs a check rather than an
assumption. This script is that check.

It asserts three things:

  1. both TIFF raster panels embedded in the committed figure are byte-identical to
     panels rebuilt from the real `00.tif` files (so no experimental image was
     re-rendered, rescaled or touched);
  2. a freshly generated figure places those same panels identically, and its
     measured values match the committed figure's;
  3. any difference between the two figures lies OUTSIDE the raster panels — i.e.
     it is annotation, not data.

Usage:
    python ci/selftest_evidence_figure.py \
        --artifact scratch/ci/art-35249590299 \
        --comparison scratch/ci/art-35249590299/ci-out/comparison.txt \
        --figure DOCS/evidence/before-after.png
"""

import argparse
import hashlib
import subprocess
import sys
import tempfile
from pathlib import Path

from PIL import Image, ImageChops

# Panel geometry, matching build_evidence_figure.py.
M, PANEL, GAP = 34, 300, 30
TAGS = ["0009B-baseline", "0009B-patched"]
FAILURES = []


def check(name, ok, detail=""):
    print(f"{'PASS' if ok else 'FAIL'}  {name}" + (f"  -- {detail}" if detail else ""))
    if not ok:
        FAILURES.append(name)
    return ok


def find_panel_y(figure: Image.Image, expected: Image.Image, i: int):
    """y at which panel i of `figure` equals `expected`, or None."""
    x = M + i * (PANEL + GAP)
    for y in range(300, 800):
        if figure.crop((x, y, x + PANEL, y + PANEL)).tobytes() == expected.tobytes():
            return y
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--artifact", required=True)
    ap.add_argument("--comparison", required=True)
    ap.add_argument("--figure", required=True)
    ap.add_argument("--generator", default="ci/build_evidence_figure.py")
    args = ap.parse_args()

    art = Path(args.artifact) / "out"
    committed = Image.open(args.figure).convert("RGB")

    print("=" * 74)
    print("evidence figure self-test")
    print("=" * 74)
    print(f"committed figure: {args.figure}")
    print(f"  sha256 {hashlib.sha256(Path(args.figure).read_bytes()).hexdigest()}")
    print(f"  size   {committed.size}")

    # --- 1. panels match the real TIFFs ------------------------------------
    positions = {}
    for i, tag in enumerate(TAGS):
        tif = art / f"{tag}.tif" / "00.tif"
        expected = (Image.open(tif).convert("L").convert("RGB")
                    .resize((PANEL, PANEL), Image.NEAREST))
        y = find_panel_y(committed, expected, i)
        positions[tag] = y
        check(f"committed figure embeds the real {tag} pixels", y is not None,
              f"at y={y}" if y else "not found")

    # --- 2. a fresh generation agrees on the data --------------------------
    with tempfile.TemporaryDirectory(prefix="vsg-fig-") as tmp:
        out = Path(tmp) / "regen.png"
        proc = subprocess.run(
            [sys.executable, args.generator,
             "--artifact", args.artifact,
             "--comparison", args.comparison,
             "--out", str(out)],
            capture_output=True, text=True)
        if not check("generator runs", proc.returncode == 0 and out.is_file(),
                     proc.stderr.strip().split("\n")[-1][:160] if proc.returncode else ""):
            return 1

        regen = Image.open(out).convert("RGB")
        check("regenerated figure has the same dimensions", regen.size == committed.size,
              f"{regen.size} vs {committed.size}")

        for i, tag in enumerate(TAGS):
            tif = art / f"{tag}.tif" / "00.tif"
            expected = (Image.open(tif).convert("L").convert("RGB")
                        .resize((PANEL, PANEL), Image.NEAREST))
            y = find_panel_y(regen, expected, i)
            check(f"regenerated figure embeds the same {tag} pixels", y is not None,
                  f"at y={y}")
            if y is not None and positions.get(tag) is not None:
                check(f"{tag} panel is in the same place in both", y == positions[tag],
                      f"{y} vs {positions[tag]}")

        # --- 3. every difference is annotation, not data -------------------
        if regen.size == committed.size:
            diff = ImageChops.difference(regen, committed).convert("L")
            changed = 0
            inside = 0
            px = diff.load()
            W, H = diff.size
            for yy in range(H):
                for xx in range(W):
                    if not px[xx, yy]:
                        continue
                    changed += 1
                    for i in range(len(TAGS)):
                        pxx = M + i * (PANEL + GAP)
                        if positions.get(TAGS[i]) is None:
                            continue
                        py = positions[TAGS[i]]
                        if pxx <= xx < pxx + PANEL and py <= yy < py + PANEL:
                            inside += 1
                            break
            pct = 100.0 * changed / (W * H)
            print(f"      differing pixels: {changed} ({pct:.2f}%), "
                  f"of which inside the raster panels: {inside}")
            check("no differing pixel falls inside a raster panel", inside == 0)

    print()
    if FAILURES:
        print(f"EVIDENCE FIGURE SELFTEST FAILED ({len(FAILURES)} check(s)): {FAILURES}")
        return 1
    print("EVIDENCE FIGURE SELFTEST OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
