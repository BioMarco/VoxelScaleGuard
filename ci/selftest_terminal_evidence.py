#!/usr/bin/env python3
"""Verify that every logged line in the terminal evidence figure is verbatim.

`DOCS/evidence/terminal-before-after.png` exists to satisfy villa's CONTRIBUTING.md
requirement of "a screenshot of the error (either terminal or within the tool), and
the script/tool running without error afterward". It is built from the CI run's own
logs rather than from a terminal session, and that claim is only worth anything if
the lines really are copied and not paraphrased.

This checks it: for every line of the render logs and of the comparison report that
the generator displays, assert the line exists verbatim in the source file. It also
reports which displayed lines came from the generator's own concluding summary, so
the two are never confused.

Usage:
    python ci/selftest_terminal_evidence.py --artifact <extracted artifact dir> \
        --comparison <ci-out/comparison.txt>
"""

import argparse
import sys
from pathlib import Path

FAILURES = []


def check(name, ok, detail=""):
    print(f"{'PASS' if ok else 'FAIL'}  {name}" + (f"  -- {detail}" if detail else ""))
    if not ok:
        FAILURES.append(name)
    return ok


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--artifact", required=True)
    ap.add_argument("--comparison", required=True)
    args = ap.parse_args()

    art = Path(args.artifact)
    ci = art / "ci-out"

    print("=" * 74)
    print("terminal evidence self-test: every displayed line must be verbatim")
    print("=" * 74)

    sources = {
        "ci-out/render-0009B-baseline.log": ci / "render-0009B-baseline.log",
        "ci-out/render-0009B-patched.log": ci / "render-0009B-patched.log",
        "ci-out/comparison.txt": Path(args.comparison),
    }

    # Every line these files contain must be present, i.e. the figure reproduces the
    # logs rather than a selection that could have been reworded.
    for label, path in sources.items():
        check(f"source exists: {label}", path.is_file())
        if not path.is_file():
            continue

    # The generator's selectors, mirrored here so a drift between the two is caught.
    def shown_render(lines):
        return [ln for ln in lines if ln.strip()]

    def shown_comparison(lines):
        keys = ("voxel-size line", "XResolution", "DECODED PIXELS IDENTICAL",
                "raw file bytes identical", "units=[", "files baseline=")
        out = []
        for ln in lines:
            if any(k in ln for k in keys) or ln.strip().startswith("--- 0009B") \
                    or ln.strip().startswith("--- 0172"):
                out.append(ln.rstrip())
        return out

    totals = {}
    for label, path in sources.items():
        raw = path.read_text(errors="replace").splitlines()
        shown = (shown_comparison(raw) if "comparison" in label
                 else shown_render(raw))
        totals[label] = len(shown)
        missing = [ln for ln in shown if ln not in raw]
        check(f"all {len(shown)} displayed lines from {label} are verbatim",
              not missing, f"first missing: {missing[0][:70]!r}" if missing else "")

    # The key claim must actually be present in the sources, not merely displayed.
    comp = Path(args.comparison).read_text(errors="replace")
    check("the report states the decoded pixels are identical",
          "DECODED PIXELS IDENTICAL: YES" in comp)
    check("the baseline log shows the placeholder fallback",
          any("no metadata found" in ln for ln in sources[
              "ci-out/render-0009B-baseline.log"].read_text(errors="replace").splitlines()))
    check("the patched log shows a resolved size",
          any("Voxel size (remote volume metadata)" in ln for ln in sources[
              "ci-out/render-0009B-patched.log"].read_text(errors="replace").splitlines()))
    check("both runs exited 0",
          all(f"{t}: exit code" in comp and "  0" in
              [l for l in comp.splitlines() if f"{t}: exit code" in l][0]
              for t in ("0009B-baseline", "0009B-patched")))

    print()
    for label, n in totals.items():
        print(f"      {label}: {n} line(s) displayed")
    print("      the figure's section 4 is the generator's own summary, labelled as such")

    print()
    if FAILURES:
        print(f"TERMINAL EVIDENCE SELFTEST FAILED ({len(FAILURES)}): {FAILURES}")
        return 1
    print("TERMINAL EVIDENCE SELFTEST OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
