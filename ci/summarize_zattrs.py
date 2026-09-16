#!/usr/bin/env python3
"""Summarise a rendered .zattrs as one line: units and the first dataset's scale.

Used by the renderer-validation workflow's edge-case step. Kept as a file rather
than inlined in the workflow so the workflow stays valid YAML and this stays
readable and testable.

Usage: python ci/summarize_zattrs.py <path to .zattrs>
Prints exactly one line, always, so the caller can paste it verbatim.
"""

import json
import sys
from pathlib import Path

path = Path(sys.argv[1] if len(sys.argv) > 1 else "")
if not path.is_file():
    print("(no .zattrs written)")
    sys.exit(0)

try:
    doc = json.loads(path.read_text())
except Exception as exc:  # noqa: BLE001
    print(f"(unparseable .zattrs: {exc})")
    sys.exit(0)

mults = doc.get("multiscales") or [{}]
first = mults[0]
units = ",".join(str(a.get("unit")) for a in first.get("axes", []))
datasets = first.get("datasets") or [{}]
scale = None
for ct in datasets[0].get("coordinateTransformations", []) or []:
    if ct.get("type") == "scale":
        scale = ct.get("scale")

print(f"units={units} first-dataset-scale={scale} (levels={len(mults[0].get('datasets', []))})")
