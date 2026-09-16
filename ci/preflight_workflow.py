#!/usr/bin/env python3
"""Pre-flight checks for the renderer-validation workflow.

Run locally before pushing. Catches the two failure modes that waste a CI run:
the YAML not parsing, and embedded shell here-docs/quoting being mangled.

Usage: python ci/preflight_workflow.py
"""

import subprocess
import sys
import tempfile
from pathlib import Path

try:
    import yaml
except Exception as exc:  # noqa: BLE001
    print(f"PyYAML is required: {exc}\n  python -m pip install pyyaml")
    sys.exit(2)

ROOT = Path(__file__).resolve().parent.parent
WORKFLOW = ROOT / ".github" / "workflows" / "renderer-validation.yml"
BASH = Path(r"C:\Program Files\Git\bin\bash.exe")

failures = []


def check(name, ok, detail=""):
    print(f"{'PASS' if ok else 'FAIL'}  {name}" + (f"  -- {detail}" if detail else ""))
    if not ok:
        failures.append(name)


print("=" * 70)
print("workflow pre-flight")
print("=" * 70)

# 1. YAML parses, and GitHub's required top-level keys are present.
#    This check has already earned its place: a colon inside an inlined shell
#    string once made the whole workflow unparseable, and it was pushed before
#    anyone noticed. Run this before every push.
try:
    doc = yaml.safe_load(WORKFLOW.read_text())
except Exception as exc:  # noqa: BLE001
    check("YAML parses", False, str(exc))
    print("\nCannot continue without a parseable workflow."
          "\nDo NOT push this: GitHub will reject the run.")
    sys.exit(1)

check("YAML parses", True)
# PyYAML resolves the bare key `on:` to the boolean True (YAML 1.1), which is
# exactly what GitHub's own parser does too. Accept either spelling.
triggers = doc.get("on", doc.get(True))
check("has triggers ('on')", triggers is not None,
      ", ".join(triggers) if isinstance(triggers, dict) else str(triggers))
check("has 'jobs'", "jobs" in doc)

jobs = doc.get("jobs", {})
check("jobs non-empty", bool(jobs), ", ".join(jobs))

for job_name, job in jobs.items():
    steps = job.get("steps", [])
    check(f"job '{job_name}' has runs-on", "runs-on" in job, str(job.get("runs-on")))
    check(f"job '{job_name}' has steps", bool(steps), f"{len(steps)} step(s)")
    for i, step in enumerate(steps, 1):
        if "run" not in step:
            check(f"  step {i} ({step.get('name', step.get('uses', '?'))}) uses:", "uses" in step)
            continue
        script = step["run"]
        # shellcheck the bash body with Git-for-Windows' bash if present.
        if BASH.exists():
            with tempfile.NamedTemporaryFile("w", suffix=".sh", delete=False,
                                             encoding="utf-8", newline="\n") as fh:
                fh.write("set -euo pipefail\n" + script)
                tmp = fh.name
            proc = subprocess.run([str(BASH), "-n", tmp], capture_output=True, text=True)
            check(f"  step {i} ({step.get('name', '?')}) bash -n",
                  proc.returncode == 0, proc.stderr.strip()[:200])

# 2. The paths the workflow depends on really exist in the repo.
for rel in ("patch/vc_render_tifxyz.patch", "ci/compare_render_outputs.py"):
    check(f"exists: {rel}", (ROOT / rel).is_file())

# 3. The patch still round-trips against the local villa clone (AGENTS.md section 4).
patch = ROOT / "patch" / "vc_render_tifxyz.patch"
proc = subprocess.run(
    ["git", "-c", "safe.directory=*", "-C", str(ROOT / "villa"),
     "apply", "--check", "--reverse", str(patch)],
    capture_output=True, text=True,
)
check("patch reverse-applies against villa/", proc.returncode == 0, proc.stderr.strip()[:200])

# 4. The report tools' own self-tests. A broken reporter inside CI wastes a full
#    dependency install and build, and can invert the answer.
for name, script in (("compare_render_outputs", "ci/selftest_compare.py"),
                     ("check_physical_size", "ci/selftest_physical_size.py")):
    if not (ROOT / script).is_file():
        check(f"self-test present: {script}", False)
        continue
    proc = subprocess.run([sys.executable, str(ROOT / script)],
                          capture_output=True, text=True, cwd=str(ROOT))
    ok = proc.returncode == 0 and "SELFTEST OK" in proc.stdout
    detail = "" if ok else (proc.stdout or proc.stderr).strip().split("\n")[-1][:200]
    check(f"self-test passes: {name}", ok, detail)

# 4. Report the run/uses set so a reviewer can spot anything unexpected.
uses = [s["uses"] for j in jobs.values() for s in j.get("steps", []) if "uses" in s]
print("\nactions used:")
for u in uses:
    print("   " + u)

print()
if failures:
    print(f"PRE-FLIGHT FAILED: {len(failures)} check(s): {failures}")
    sys.exit(1)
print("PRE-FLIGHT OK")
