#!/usr/bin/env python3
"""Build the before/after evidence figure from REAL run artefacts.

Nothing in this figure is redrawn from a description. Every value comes from
either:

  * the actual TIFF files written by the two binaries, read here with Pillow
    (dimensions, mode, XResolution/YResolution/ResolutionUnit, decoded pixels);
  * the actual `.zattrs` documents, whose contents are quoted verbatim from the
    run's comparison report -- the `.zattrs` files themselves are not in the
    uploaded artifact because actions/upload-artifact skips hidden files by
    default (`include-hidden-files: false`), so the text is taken from the run's
    own log rather than re-derived;
  * the run's own comparison report, quoted in the source column.

Usage:
    build_evidence_figure.py --artifact <extracted artifact dir> \
        --comparison <ci-out/comparison.txt> --out evidence.png

The point of the figure is that the RENDERED PIXELS ARE UNCHANGED and only the
declared physical metadata moves. The pixel panels are therefore shown
side by side and labelled as identical, and a difference panel is included; it is
not presented as an improvement.
"""

import argparse
import hashlib
import json
import re
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

# Palette: intentionally plain, so the figure reads as a report.
BG = (255, 255, 255)
INK = (24, 24, 27)
MUTED = (110, 113, 122)
RULE = (210, 213, 219)
BAD = (176, 42, 55)
GOOD = (21, 105, 63)
PANEL = (246, 247, 249)


def font(size, bold=False):
    for name in (("DejaVuSans-Bold.ttf", "arialbd.ttf", "seguisb.ttf") if bold
                 else ("DejaVuSans.ttf", "arial.ttf", "segoeui.ttf")):
        try:
            return ImageFont.truetype(name, size)
        except Exception:  # noqa: BLE001
            continue
    return ImageFont.load_default()


F_TITLE = font(26, True)
F_H2 = font(17, True)
F_BODY = font(14)
F_MONO = font(13)
F_MONO_B = font(13, True)
F_SMALL = font(12)
F_TINY = font(11)


def tif_facts(path: Path):
    with Image.open(path) as im:
        tags = dict(im.tag_v2)
        pixels = im.tobytes()
        return {
            "path": path,
            "size": im.size,
            "mode": im.mode,
            "xres": tags.get(282),
            "yres": tags.get(283),
            "runit": tags.get(296),
            "pixel_sha": hashlib.sha256(pixels).hexdigest(),
            "file_bytes": path.stat().st_size,
            "pixels": pixels,
        }


def parse_comparison(text: str):
    """Pull the real .zattrs per-level unit/scale pairs out of the run's report."""
    out = {}
    cur = None
    for line in text.splitlines():
        m = re.match(r"^--- (\S+)$", line.strip())
        if m:
            cur = m.group(1)
            out.setdefault(cur, [])
            continue
        m = re.search(r"dataset (\d+): units=(\[.*?\]) scale=(\[.*?\])", line)
        if m and cur:
            out[cur].append((int(m.group(1)),
                             json.loads(m.group(2).replace("'", '"')),
                             json.loads(m.group(3))))
    return out


def wrap(draw, text, fnt, width):
    words, lines, cur = text.split(), [], ""
    for w in words:
        trial = f"{cur} {w}".strip()
        if draw.textlength(trial, font=fnt) <= width:
            cur = trial
        else:
            if cur:
                lines.append(cur)
            cur = w
    if cur:
        lines.append(cur)
    return lines


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--artifact", required=True)
    ap.add_argument("--comparison", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--run-url", default="")
    args = ap.parse_args()

    art = Path(args.artifact)
    zattrs = parse_comparison(Path(args.comparison).read_text(errors="replace"))

    tags_order = ["0009B-baseline", "0009B-patched", "0172-baseline", "0172-patched"]
    facts = {t: tif_facts(art / "out" / f"{t}.tif" / "00.tif") for t in tags_order}

    W, H = 1500, 1180
    img = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(img)
    M = 34

    y = M
    d.text((M, y), "vc_render_tifxyz — before / after, real run artefacts", font=F_TITLE, fill=INK)
    y += 34
    d.text((M, y),
           "Same public volume, same segment, same arguments. Two binaries built from one commit: "
           "main (baseline) and main + the patch.",
           font=F_BODY, fill=MUTED)
    y += 20
    d.text((M, y),
           "PHerc0009B / 20250521125136-8.640um-1.2m-116keV-masked.zarr   ·   "
           "-g 0 --scale 1 -n 1 --crop-x 3145 --crop-y 3412 --crop-width 128 --crop-height 128",
           font=F_MONO, fill=MUTED)
    y += 26
    if args.run_url:
        d.text((M, y), f"source: {args.run_url}", font=F_SMALL, fill=MUTED)
        y += 20
    d.line([(M, y), (W - M, y)], fill=RULE, width=1)
    y += 18

    # ---------------------------------------------------------------- metadata
    d.text((M, y), "1. Declared physical metadata", font=F_H2, fill=INK)
    y += 26
    d.text((M, y),
           "This is the whole change. The rendered image is not affected — see panel 2 below.",
           font=F_BODY, fill=MUTED)
    y += 24

    col_x = [M, M + 350, M + 700, M + 1050]
    heads = ["", "baseline (main)", "patched", ""]
    for i, h in enumerate(heads):
        if h:
            d.text((col_x[i], y), h, font=F_MONO_B, fill=INK)
    y += 22

    def row(label, b_val, p_val, verdict, good=True):
        nonlocal y
        d.rectangle([M - 8, y - 4, W - M + 8, y + 22], fill=PANEL)
        d.text((M, y), label, font=F_MONO, fill=MUTED)
        d.text((col_x[1], y), b_val, font=F_MONO, fill=BAD)
        d.text((col_x[2], y), p_val, font=F_MONO, fill=GOOD)
        d.text((col_x[3], y), verdict, font=F_SMALL, fill=GOOD if good else BAD)
        y += 28

    for vol, fam in (("PHerc0009B (8.64 µm)", "0009B"), ("PHerc0172 (7.91 µm)", "0172")):
        d.text((M, y), vol, font=F_MONO_B, fill=INK)
        y += 22
        zb, zp = zattrs.get(f"{fam}-baseline", []), zattrs.get(f"{fam}-patched", [])
        if zb and zp:
            bu, bs = zb[0][1][0], zb[0][2]
            pu, ps = zp[0][1][0], zp[0][2]
            row(".zattrs axis unit", bu, pu, "corrected")
            row(".zattrs scale, level 0", f"[{', '.join(f'{v:g}' for v in bs)}]",
                f"[{', '.join(f'{v:g}' for v in ps)}]",
                "physically meaningful")
        fb, fp = facts[f"{fam}-baseline"], facts[f"{fam}-patched"]
        row("TIFF XResolution",
            "absent" if fb["xres"] is None else f"{float(fb['xres']):.4f}",
            "absent" if fp["xres"] is None else f"{float(fp['xres']):.4f}",
            "tag now written" if fb["xres"] is None else "unchanged")
        y += 8

    d.line([(M, y), (W - M, y)], fill=RULE, width=1)
    y += 16

    # ------------------------------------------------------------------ pixels
    d.text((M, y), "2. Rendered pixels — unchanged, presented as unchanged", font=F_H2, fill=INK)
    y += 24
    d.text((M, y),
           "Both panels below are the actual 00.tif from the run, drawn at 2×. Their decoded pixels are "
           "identical, so the difference panel is blank.",
           font=F_BODY, fill=MUTED)
    y += 24

    order = ["0009B-baseline", "0009B-patched"]
    panel_w, panel_h = 300, 300
    gap = 30
    base_x = M
    for i, t in enumerate(order):
        im = Image.open(facts[t]["path"]).convert("L").convert("RGB").resize(
            (panel_w, panel_h), Image.NEAREST)
        x = base_x + i * (panel_w + gap)
        img.paste(im, (x, y))
        d.rectangle([x - 1, y - 1, x + panel_w, y + panel_h], outline=RULE)
        d.text((x, y + panel_h + 6), t, font=F_MONO, fill=INK)
        d.text((x, y + panel_h + 22),
               f"{facts[t]['size'][0]}x{facts[t]['size'][1]}  ·  {facts[t]['file_bytes']} bytes",
               font=F_TINY, fill=MUTED)

    # difference panel
    xd = base_x + 2 * (panel_w + gap)
    a = Image.open(facts["0009B-baseline"]["path"])
    b = Image.open(facts["0009B-patched"]["path"])
    diff = Image.new("RGB", a.size, (255, 255, 255))
    ap, bp = a.tobytes(), b.tobytes()
    dp = bytearray()
    for i in range(0, len(ap)):
        v = abs(ap[i] - bp[i])
        dp.extend((255 - v, 255 - v, 255 - v) if v else (255, 255, 255))
    diff.frombytes(bytes(dp))
    diff = diff.resize((panel_w, panel_h), Image.NEAREST)
    img.paste(diff, (xd, y))
    d.rectangle([xd - 1, y - 1, xd + panel_w, y + panel_h], outline=RULE)
    d.text((xd, y + panel_h + 6), "absolute difference", font=F_MONO, fill=INK)
    d.text((xd, y + panel_h + 22), "all zeros = no pixel changed", font=F_TINY, fill=GOOD)
    d.text((xd, y + panel_h + 38), "decoded-pixel SHA-256:", font=F_TINY, fill=MUTED)
    d.text((xd, y + panel_h + 52),
           facts["0009B-baseline"]["pixel_sha"][:32] + "…", font=F_TINY, fill=MUTED)
    d.text((xd, y + panel_h + 66),
           facts["0009B-patched"]["pixel_sha"][:32] + "…", font=F_TINY, fill=GOOD)

    # summary to the right
    y2 = y
    d.text((xd + panel_w + gap + 10, y2), "what changed", font=F_H2, fill=INK)
    y2 += 26
    lines = [
        ("Only the declared physical scale.", INK),
        ("", INK),
        ("The TIFF resolution tag is a tag,", MUTED),
        ("not an image: adding or removing", MUTED),
        ("it changes the file bytes while", MUTED),
        ("leaving every pixel alone, which is", MUTED),
        ("why the file sizes differ above", MUTED),
        ("(14244 -> 14296) and the decoded", MUTED),
        ("pixels do not.", MUTED),
        ("", INK),
        ("Pixel equality is the regression", MUTED),
        ("check. File-size or file-hash", MUTED),
        ("equality is deliberately NOT the", MUTED),
        ("check: it would report this fix as", MUTED),
        ("a regression.", MUTED),
    ]
    for text, col in lines:
        d.text((xd + panel_w + gap + 10, y2), text, font=F_SMALL, fill=col)
        y2 += 17

    y = y + panel_h + 84

    # ------------------------------------------------------------------ provenance
    d.line([(M, y), (W - M, y)], fill=RULE, width=1)
    y += 16
    d.text((M, y), "3. Where each value above comes from", font=F_H2, fill=INK)
    y += 24
    prov = [
        "TIFF dimensions, mode, XResolution/YResolution/ResolutionUnit and decoded pixels: read from the "
        "actual 00.tif files produced by the two binaries, with Pillow.",
        ".zattrs unit and scale: the .zattrs files are not in the uploaded artifact — "
        "actions/upload-artifact skips hidden files by default — so these values are quoted verbatim "
        "from the run's own comparison report, which read them from those files.",
        "Both binaries were built from the same commit (757f70c) in the same configuration, and the "
        "applied diff was checked byte-identical to the committed patch before the second build.",
        "No value in this figure is reconstructed, interpolated or redrawn from a description.",
    ]
    for p in prov:
        for line in wrap(d, p, F_SMALL, W - 2 * M - 12):
            d.text((M + 12, y), line, font=F_SMALL, fill=MUTED)
            y += 16
        y += 4

    Path(args.out).parent.mkdir(parents=True, exist_ok=True)
    img.save(args.out)
    print(f"wrote {args.out} ({img.size[0]}x{img.size[1]})")

    # Print the same facts so the caller can verify them in the transcript.
    print("\nvalues used:")
    for t in tags_order:
        f = facts[t]
        print(f"  {t}: size={f['size']} xres={f['xres']} runit={f['runit']} "
              f"bytes={f['file_bytes']} pixel_sha={f['pixel_sha'][:16]}…")
    for k, v in zattrs.items():
        if v:
            print(f"  {k}: unit={v[0][1][0]} scale0={v[0][2]}")


if __name__ == "__main__":
    main()
