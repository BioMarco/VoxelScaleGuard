# AGENTS.md

Operating rules for automated agents (and humans acting like them) working in
**VoxelScale Guard**.

This file is binding for anything under this repository root. It is deliberately
specific: the fastest way to damage this project is to repeat work that was
already done, or to state as verified something that was only read.

---

## 1. What this project is

An investigation into, and a contained fix for, a silently wrong **physical voxel
size** in Vesuvius Challenge [`villa`](https://github.com/ScrollPrize/villa)'s
renderer `vc_render_tifxyz`.

Read in this order before touching anything:

1. `DOCS/PROJECT_STATUS.md` — what is done, what is blocked, and the environment
   quirks that will otherwise waste your time.
2. `DOCS/ROOT_CAUSE_ANALYSIS.md` — the cause, at file-and-line resolution.
3. `DOCS/RESULTS.md` §7 — **what was NOT verified.** Read this before writing any
   claim.
4. `DOCS/ARCHITECTURE.md` — the fix's shape, and why the obvious alternatives were
   rejected.

## 2. The one rule that matters most

**Never state as verified something you did not execute.**

This project's entire value is that its claims were checked. Every claim in the
documents is tagged by how it was established:

| Tag | Meaning |
|---|---|
| **[read]** | read from a pinned source, with file and line cited |
| **[exec]** | produced by running code, with the command recorded in `DOCS/RESULTS.md` |
| **[live]** | observed against real published data from the catalog |

If you add a claim, tag it. If you cannot tag it, say "unverified" in the same
sentence. Do not upgrade a **[read]** to an **[exec]**, and do not describe a
derived consequence as a measurement.

Concretely, the following **are currently unverified** and must not be described
otherwise. This list is deliberately short now: it was four items, and three were
closed by execution on 2026-09-16 (`DOCS/CI_VALIDATION.md`). Do not add anything
back to it by implication — if you cannot tag a claim, say "unverified" in the same
sentence.

* the GUI path: `vc_render_tifxyz`'s fix is **not reachable from VC3D**, because
  the enable predicate is deliberately unchanged;
* coverage: **two volumes, one crop, one slice each**. That demonstrates the
  correction; it is not a survey, and no claim about other volumes is supported;
* the "no usable voxel size anywhere" branch is covered by **unit tests only** —
  it is not driven end to end;
* the `.zattrs`/TIFF artifacts come from a **single build configuration**
  (`QuickBuild`, gcc 13.3, Linux) with `--scale 1`. Other presets, compilers and
  scales are unmeasured.

**And one thing this file got wrong, kept because it is instructive.** Until
2026-09-16 this section asserted that "the patched `vc_render_tifxyz` has never
been compiled or run". True at the time — and the first compile then **failed**.
The patch as originally committed could never have built (used variables declared
~60 lines below their use; `DOCS/RESULTS.md` §9.1). Treating "unverified" as a box
to tick is not the same as treating it as a thing to find out.

## 3. Scope discipline

**In scope:** voxel-size resolution and physical-scale correctness in
`villa` tooling, and the tests that pin it.

**Out of scope, do not start without an explicit instruction:**

* `..\Vesuvius` — the previous project (**FitPatch Loop**), suspended by the
  user's decision. Do not read, modify, delete, rename, move or "tidy" anything
  there. Do not claim to have verified or frozen it either; its state is simply
  unknown to this project.
* Rewriting the fix as a Python wrapper, a standalone tool, or a new HTTP/S3
  client. These were considered and rejected on evidence — see
  `DOCS/ARCHITECTURE.md` §6. Re-proposing them without new evidence is churn.
* Changing `--voxel-unit`'s default, or the VC3D GUI predicate. Both are
  documented as deliberate follow-ups in `DOCS/FEASIBILITY.md` §8, not oversights.

## 4. Working with the upstream repository

`villa/` is a **local, read-only-in-spirit** clone of upstream at
`757f70c0140a4cfbbbd44975ef09558444b96980`. Rules:

* **Never push to, or open a pull request against, `ScrollPrize/villa`** without
  explicit authorisation.
* The clone is modified **in place** to hold the patch. That is expected. The
  committed artefact is `patch/vc_render_tifxyz.patch`, not the clone.
* `villa/` is git-ignored here on purpose: it is an upstream checkout with its own
  `.git`, and must not be absorbed into this repository.
* After any change to `villa/`, regenerate the patch and re-verify it:
  ```
  cd villa
  git -c safe.directory='*' diff --output=../patch/vc_render_tifxyz.patch -- volume-cartographer/apps/src/vc_render_tifxyz.cpp
  git -c safe.directory='*' apply --check --reverse ../patch/vc_render_tifxyz.patch   # must exit 0
  ```
  A reverse-apply that succeeds proves the patch is well-formed **and** describes
  the working tree exactly. Do this before claiming a patch is current.
* `villa` is MIT (Copyright (c) 2024 Vesuvius Challenge). Keep it that way. Do not
  copy `villa` code into this repository without its licence header and a note.

## 5. Attribution — non-negotiable

Several people did substantial work on this problem before this project, and three
of their attempts lapsed to process rather than to technical rejection. Credit them
specifically, by name and PR, whenever the corresponding idea is used:

| Contribution | Attribution |
|---|---|
| Consulting the volume's remote voxel size in the renderer | **NicolasHuberty**, PR #1417 |
| `scan.tomo.acquisition.detector.samplePixelSize` handling, `resolveLocalStoreVoxelSize`, `Volume::voxelSize()` semantics | **Bullo27** and the villa maintainers (PRs #1227, #1229, #1454) |
| The VC3D enable-predicate diagnosis | **Bullo27**, PR #1228 |
| The issue reports | **DarthCeltic** (#1403), **Bullo27** (#1226) |

Do not present prior work as this project's discovery. Where this project differs,
say how and show the comparison — `DOCS/FEASIBILITY.md` §7 is the model.

## 6. Verification requirements

### 6.1 Any change to the patch must be accompanied by

1. A regeneration + reverse-apply check of `patch/vc_render_tifxyz.patch` (§4).
2. A build and run of the harness (`harness/build.ps1`, below).
3. A test for the behaviour changed. If you cannot write one, say why in the
   commit message and in `DOCS/RESULTS.md`.

### 6.2 Never weaken a test to make it pass

This has already happened once, in the other direction: two assertions were
**corrected** when execution contradicted them (the reader returns `0` and `-3`
rather than returning nothing), which made the finding stronger. That is the
expected direction of travel. `DOCS/RESULTS.md` §3.2 records it.

If a test fails, the first hypothesis is that the test encodes a wrong belief, not
that the code is wrong. Determine which by executing, then fix whichever is
actually wrong, and record the change.

### 6.3 Requirements of the upstream project

`villa`'s own `AGENTS.md` (at `villa/AGENTS.md`) applies to work *inside* `villa`.
In particular it forbids install/bootstrap commands unless explicitly requested,
requires tests for touched logic, and requires the smallest change that solves the
task. Read it before modifying anything under `villa/`.

### 6.4 Do not add tests that only assert what you just wrote

A test that exercises the fix against a mock of the fix proves nothing. Prefer, in
order:

1. the real upstream translation unit, executed;
2. a byte-verbatim copy of the real code under test (the defect-reproduction tests
   do this deliberately — see `harness/src/vsguard/upstream_read_volume_voxel_size.hpp`);
3. simulated input against real logic.

Never simulate real logic with logic of your own and call it a verification.

## 7. Environment: the traps, and how to avoid them

These cost real time. They are properties of the sandbox, not of the code.

### 7.1 Do not redirect or pipe native command output

```
cmd > file          # FAILS: StandardOutputEncoding is only supported when standard output is redirected
cmd | Select-Object # FAILS the same way
cmd 2>&1            # FAILS the same way
cmd                 # WORKS
```

Use the command's own output option instead — e.g.
`git diff --output=path` — or let it print and read the transcript.

### 7.2 CMake and Ninja cannot be used here

CMake 3.24 (bundled with VS 2022) cannot launch its own subprocesses in this
sandbox; both `ninja.exe` and its compiler probes return `Accesso negato`. Upstream
also requires CMake ≥ 3.28, which is not installed. `harness/CMakeLists.txt` is kept
for normal environments; `harness/build.ps1` invokes `cl.exe` directly and is the
supported path here.

### 7.3 Toolchain specifics

* MSVC toolset **14.34.31933**, Windows SDK **10.0.22000.0**. The SDK version
  matters: paths for `10.0.22621.0` do **not** exist on this machine and produce
  `fatal error C1083: cannot open include file: 'stdio.h'`.
* No vcpkg (`VCPKG_ROOT` unset, no checkout anywhere), no Qt, OpenCV, Ceres, CGAL,
  Docker, MSYS2 or WSL. This is why the patched application cannot be built — it is
  a resource gap, not an oversight. **Do not install these without authorisation**;
  the closure is multi-GB and builds from source.
* `Invoke-WebRequest` fails on TLS here. Node's `fetch` works, which is why the
  download helpers are `.mjs` scripts.

### 7.4 Git ownership

The clone is owned by `BUILTIN/Administrators` (elevated clone), and
`~/.gitconfig` is outside the writable sandbox, so `git config --global --add
safe.directory` fails and plain git commands in `villa/` abort with *detected
dubious ownership*. Invoke git as:

```
git -c safe.directory='*' -C villa <command>
```

### 7.5 Command execution is intermittent

During earlier sessions, external processes sometimes returned **empty output with
no exit code** and later recovered; `pwsh` built-ins kept working throughout. If a
command returns nothing, retry it once before concluding anything about the code.
Never record a silent failure as evidence.

## 8. Building and testing

```powershell
# one-time: pinned upstream sources + two header-only libs (~1.2 MB)
pwsh -File harness/setup.ps1
node harness/fetch_deps.mjs

# build (direct cl.exe; see 7.2)
pwsh -File harness/build.ps1 -Configuration Release

# tests
cd harness/build/Release
./test_upstream_voxel_size_metadata.exe   # upstream's suite, unmodified: must stay 13/13
./test_render_voxel_size.exe              # this project's: must stay 19/19
./probe_render_voxel_size.exe             # before/after over the real documents
```

`harness/setup.ps1` copies upstream files **byte-for-byte**. Never edit the copies
under `harness/src/villa/`; re-run `setup.ps1` to re-sync. Re-run it after changing
the pinned commit.

One exception is deliberate and load-bearing: the file the patch modifies,
`vc_render_tifxyz.cpp`, is taken from **git at the pinned commit**
(`git show <commit>:<path>`), not from `villa/`'s working tree. That clone is
patched in place on purpose, so copying from it would yield the patched file while
calling it pristine — which is what silently happened until a test caught it. The
pristine copy is asserted unpatched by `test_render_voxel_size.exe`.

The same suite also checks the patch as an artefact: that every name the new
resolution block reads is declared above its call site. That check exists because
the patch failed to compile in CI for exactly that reason; it cannot prove the
file compiles, so the CI build remains the real verification.

## 9. Research: how to touch the network

* Read-only GETs of public data are fine: the anonymous Open Data bucket, the
  GitHub REST API.
* **Never read, print or commit credentials, tokens or keys.** If a task seems to
  require one, stop and ask.
* Stay within the sanctioned helpers where possible:
  `research/fetch_volume_metadata.mjs` (catalog probe),
  `harness/fetch_deps.mjs` (build headers).
* Do not download more than 2 GB, or run heavy GPU work, without authorisation.
  Current total footprint: ~1.2 MB of headers, ~12 KB of volume metadata.
* Do not invent data. If a fetch fails, record the failure and stop the work that
  depends on it — see `DOCS/RESEARCH.md` §1 for how an access limitation is
  documented rather than papered over.

## 10. Publishing

* **Do not push to this repository's remote, open a pull request, or publish
  anything anywhere** without explicit authorisation for that specific action.
* Do not modify the remote configuration, force-push, or rewrite published
  history.
* Do not commit: `villa/`, `harness/build/`, `harness/third_party/`,
  `harness/tools/ninja.exe`, or any downloaded dataset. `.gitignore` covers these;
  if you add a new build output, add it there too.

## 11. Documentation conventions

* Documents live in `DOCS/`. Only `README.md` and `AGENTS.md` sit at the root, so
  the repository landing page stays readable. `README.md` links to `DOCS/` with
  the prefix; everything inside `DOCS/` links to its siblings by plain filename.
* Cross-references between documents inside `DOCS/` are by plain filename (e.g.
  `RESULTS.md` §7), because they are all siblings. From `README.md` or `AGENTS.md`,
  prefix with `DOCS/` (e.g. `DOCS/RESULTS.md`).
* Prefer citing **file:line** over prose when referring to upstream code. Line
  numbers are relative to the pinned commit; state the commit whenever you cite
  one, because upstream moves.
* When you discover that a documented claim is wrong, **correct the document and
  record that it was corrected**. Do not quietly delete it. `DOCS/RESEARCH.md` §4
  lists assumptions from the original brief that did not survive contact with the
  source; that section is the model.
* Write the negative results. `DOCS/RESULTS.md` §7 is the most important section of
  the document set: it is what makes the positive claims checkable.

## 12. Definition of done

A task is done when:

1. the change is made at the smallest correct scope;
2. it is verified by **execution**, or the inability to verify it is stated
   explicitly with the reason and what would be needed;
3. tests exist for the behaviour, and the full harness passes;
4. the patch round-trips (§4) if `villa/` was touched;
5. the affected documents are updated, including **new limitations**;
6. nothing unverified has been described as verified.

Compiling is not done. Passing tests is not done. A demonstration on real data —
or an explicit, specific statement of why one was not possible — is done.
