# RESUME — how to pick this project up in a fresh chat

Written to be sufficient on its own. A new session should be able to read this
file plus `AGENTS.md` and continue without re-deriving anything.

**Repository:** <https://github.com/BioMarco/VoxelScaleGuard>
**Workspace:** `C:\Users\marco\Documents\DeepSeek\VoxelScaleGuard`
**State at handoff:** `main` @ `9cfcaf9`, clean, in sync with `origin/main`

---

## 1. First: verify the state you inherited

Do not trust this document alone. Run these. All are read-only.

```powershell
cd C:\Users\marco\Documents\DeepSeek\VoxelScaleGuard

# 1. repository clean and in sync with the remote
pwsh -File tools\git.ps1 status --short          # expect: no output
pwsh -File tools\git.ps1 log --oneline -3        # expect: 9cfcaf9, 6de80df, 4a77205
pwsh -File tools\git.ps1 rev-parse HEAD origin/main   # expect: the same hash twice

# 2. the harness still builds and passes (this is the control that the pinned
#    upstream code is intact)
pwsh -File harness/setup.ps1
node harness/fetch_deps.mjs
pwsh -File harness/build.ps1 -Configuration Release
cd harness\build\Release
.\test_upstream_voxel_size_metadata.exe            # expect: 13 cases / 54 assertions, all pass
.\test_render_voxel_size.exe                       # expect: 27 cases / 208 assertions, all pass
.\probe_render_voxel_size.exe                      # expect: 3 divergences out of 4 volumes
cd ..\..\..

# 3. the patch still round-trips against the local villa clone
#    NB: `-C villa` changes the working directory first, so the patch path must be
#    relative to villa/. This command is issued from the workspace root.
git -c safe.directory='*' -C villa apply --check --reverse ../patch/vc_render_tifxyz.patch
# expect exit 0
```

If step 3 fails, the `villa` clone and the patch have diverged. Re-read
`AGENTS.md` §4 before regenerating anything.

## 2. What this project is, in one paragraph

`vc_render_tifxyz` (in Vesuvius Challenge
[`villa`](https://github.com/ScrollPrize/villa)) attaches a physical voxel size to
everything it renders — into OME-Zarr `.zattrs` and TIFF resolution tags. It
obtains that number from a **local** file, using a private reader that recognises
only a top-level `voxelsize` key, **while the process has already opened the
source volume remotely and is holding the correct value**. For three of four
published volumes probed, it therefore falls back to a scale of `1.0` declared as
**nanometres**, making the declared physical voxel size wrong by **×2400, ×8640 or
×45532**. The rendered pixels are correct; every physical number attached to them
is not. The fix is **three files, +250/−65**; it was written, then **compiled and
run in CI on two real published volumes**, and the rendered pixels were verified
byte-identical (`RESULTS.md` §9).

*This paragraph said "one file, +176/−63 … has never been compiled or run" until
2026-09-18. Both halves were stale: the patch grew two Zarr files, and it has been
compiled and run since 2026-09-16. It is the same staleness that produced the
regeneration-command defect in `RESULTS.md` §12.1, kept visible here as a reminder
that the summaries drift faster than the evidence.*

## 3. Read these, in this order

| Document | Why |
|---|---|
| `DOCS/INDEX.md` | Reading order and the evidence map |
| `DOCS/PROJECT_STATUS.md` | State, gaps, environment notes, activity log |
| `DOCS/ROOT_CAUSE_ANALYSIS.md` | The cause at file-and-line resolution; every claim tagged `[read]`/`[exec]`/`[live]` |
| `DOCS/RESULTS.md` | Everything executed. **§7 is what was NOT executed — read it before writing any claim** |
| `DOCS/FEASIBILITY.md` | The GO decision, what is out of scope, and why |
| `DOCS/ARCHITECTURE.md` | The fix's design and the alternatives rejected on evidence |
| `DOCS/TEST_PLAN.md` | Executed vs blocked; what would falsify each claim |

`AGENTS.md` at the root is binding: verification rules, attribution, scope
boundaries, environment traps.

## 4. What is done

* **Diagnosis, verified.** Five interacting defects, established by reading the
  pinned revision *and* by executing its real code against live catalog metadata.
  Also: the `vc_grow_seg_from_seed` half of issue #1403 is **already fixed
  upstream** — not by us, and we claim no credit.
* **Both review concerns on the lapsed PR #1417 reproduced as executable tests.**
  The URL-fragment hazard is real in `joinRemoteUrlPath` and reachable via the
  GUI's `remoteLocator()`; the invalid-local-metadata concern is real and *worse*
  than the review described (the reader returns `0.0` and `-3.0` as measurements).
* **The patch**, `patch/vc_render_tifxyz.patch`, **three files, +250/−65**, applies
  exactly to `villa` @ `757f70c0140a4cfbbbd44975ef09558444b96980`.
  *This line said "one file" until 2026-09-18; the patch grew a second and third
  file on 2026-09-17 (`core/src/Zarr.cpp`, `core/include/vc/core/util/Zarr.hpp`) and
  the summary was not updated. The same stale belief was in `AGENTS.md` §4's
  regeneration command, where it was not cosmetic: see `RESULTS.md` §12.1.*
* **The harness**, which compiles the pinned revision's real
  `VoxelSizeMetadata.cpp` / `RemoteUrl.cpp` / `Json.cpp` byte-for-byte, plus a
  verbatim copy of the pre-patch reader, plus tests: 27 cases / 208 assertions, with
  upstream's own 13-case suite compiled unmodified as the control.
* **The before/after demonstration on four real published volumes**, with a
  deliberate control (the one legacy-shaped volume that already worked, and which
  is the volume upstream's only live-S3 test pins) — and from 2026-09-16, **two real
  compiled binaries rendering two real volumes in CI** (`RESULTS.md` §9).
* **Fifteen documents**, two evidence figures and a public repository. The
  documentation set gained `LICENSING_PROPOSAL.md` and
  `raw_metadata/PROVENANCE.md` on 2026-09-18.

## 5. The next step — **this section is history; the gap it describes is closed**

> **Closed 2026-09-16.** The patched binary now compiles and runs, and the
> before/after was captured from two real binaries on real published volumes. The
> route was **not** the vcpkg closure and **not** the prebuilt Windows package
> described below: it was a GitHub Actions workflow on a free `ubuntu-24.04` runner
> using the public apt package list. Full record in `CI_VALIDATION.md` and
> `RESULTS.md` §9.
>
> The text below is kept because it documents what was believed and why, and
> because one of its assumptions turned out badly wrong: the patch it describes as
> ready to build **did not compile**. See `RESULTS.md` §9.1.
>
> What genuinely remains is the **GUI path** (§7.4) and broader coverage. Neither
> needs the vcpkg closure.

**The blocking gap was that the patched binary had never been compiled or run.**
That is now closed.

The full build needs the vcpkg closure (Qt, OpenCV, Ceres, CGAL), which is
unauthorised and multi-GB. But reconnaissance found a way to get most of the
value without it:

> **A prebuilt VC3D Windows package is published, built from precisely the commit
> this project pins.**

From the GitHub release tagged `latest` (published 2026-09-15):

```
VC3D-757f70c-2026-09-15-win64.zip    148.4 MB     built from commit 757f70c
VC3D-757f70c-2026-09-15-win64.exe    105.2 MB
```

`757f70c` is the exact pinned revision. And the package is **self-contained and
includes the CLI tools**: `.github/workflows/vc3d-windows.yml` smoke-tests
`$bin\vc_tifxyz_trim.exe` where `$bin` is the directory holding `VC3D.exe`, so the
`vc_*` executables ship next to it with their full DLL and Qt-plugin closure.

**Consequence:** the *before* side of the demonstration can be produced today,
from the real shipped binary, with no build at all. 148 MB is well inside the
2 GB limit, and it is a public release artefact — but **ask before downloading**,
since it is a substantial download and the standing instruction is to get
authorisation for heavy ones.

Suggested procedure:

```powershell
# 1. download the asset built from the pinned commit (ask first)
#    https://github.com/ScrollPrize/villa/releases/download/latest/VC3D-757f70c-2026-09-15-win64.zip
# 2. extract into a scratch directory INSIDE the workspace, e.g. scratch/vc3d-win64/
#    (workspace-write only; do not install into Program Files)
# 3. locate the bin dir
Get-ChildItem scratch -Recurse -Filter VC3D.exe | Select-Object -First 1
# 4. confirm the CLI tools are there and run the shipped renderer's --help
& "$bin\vc_render_tifxyz.exe" --help
# 5. capture the BEFORE transcript on the volume from the report
#    (needs a tifxyz segment; see "still needed" below)
```

What this buys:

* the **real, shipped** behaviour, not a reproduction — the strongest possible
  form of the "before/after logs" the Progress Prize rubric asks for;
* confirmation of the exact log line and of `TIFFTAG_XRESOLUTION` being absent;
* a check on one thing currently only **[read]**: that the `vc_*` CLI tools really
  do ship in the Windows bundle.

What it does **not** buy: the *after* side. Producing the fixed binary still needs
a build. Options, in order of cost:

1. **MSYS2 UCRT64 + the `ci-windows-mingw` preset**, which is what upstream CI
   uses. Needs MSYS2 plus the `vc3d-deps` archive that CI restores with
   `oras pull ghcr.io/scrollprize/vc3d-deps/windows:sha-d84dfa07…` — i.e. a
   prebuilt dependency bundle, *not* a from-source vcpkg build. This is much
   cheaper than the vcpkg route and is likely the right choice. **Requires
   authorisation** (install + download).
2. The `vc3d-deps` **Linux container** with `vc3d-deps` and the
   `ci-release-*` presets. Needs Docker or WSL, neither present.
3. `windows-msvc` + vcpkg from source. Most expensive; avoid.

**Also still needed for the artifact-level test:** a tifxyz segment. None is
present locally. Options: take one from the public Open Data catalog, or grow a
small one — the latter needs the tools, which is circular. Prefer a published
segment.

### Measured on 2026-09-16 — read this before spending anything

Reconnaissance in the continuation session changed the picture on both routes.
Full detail in `DOCS/RESULTS.md` §8.4; the load-bearing points:

* **CMake and Ninja cannot execute a compiler here.** Both run and report their
  versions (`cmake 3.24.202208181-MSVC_2`, `ninja 1.11.0`), but CMake fails with
  `Accesso negato` when it probes `ninja.exe`, and `ninja` **hangs** when a build
  rule spawns a process. So a CMake-driven build — which is what `ci-windows-mingw`
  is — is not reachable from this sandbox, and **installing a newer CMake does not
  change that.**
* **The `vc3d-deps` bundle is not anonymously readable.** An anonymous
  `ghcr.io/token` request for `repository:scrollprize/vc3d-deps:pull` returns
  **HTTP 403** [live] (`node research/recon_ghcr_bundles.mjs`). CI authenticates
  with `${{ github.token }}`. Its size and contents therefore cannot even be
  measured from here, and obtaining them is a credentials question this project
  must not answer. **Do not plan around this route without resolving that first.**
* **The target's link closure is smaller than the project's configure closure.**
  Ceres, CGAL and Qt are *not* linked by `vc_render_tifxyz`, but
  `find_package(Ceres REQUIRED)` is unconditional and `VC_BUILD_APPS` (which always
  adds the Qt GUI) is ON by default, so the project cannot be configured without
  them. What the binary actually needs is OpenCV, libtiff, Boost `program_options`,
  curl, zlib, blosc, zstd, lz4, a `vc_delta3d` codec, plus Eigen and
  nlohmann-json headers. Full audit in `research/vc_render_tifxyz_build_analysis.md`.
* **What is still cheap and still worth doing:** the *before* side, from the
  prebuilt Windows package (148.4 MB). Its existence was re-confirmed [live] on
  2026-09-16 — see below.

### Acceptance criteria for "the next step is done"

1. The patched `vc_render_tifxyz.cpp` **compiles** with no new warnings, clean
   under `-Werror` if the strict preset is affordable.
2. `vc_render_tifxyz --help` still lists `--voxel-size` and `--voxel-unit`
   unchanged.
3. On `s3://vesuvius-challenge-open-data/PHerc0009B/volumes/20250521125136-8.640um-1.2m-116keV-masked.zarr`
   the log line reads `Voxel size (remote volume metadata): 8.64 micrometer`
   where the shipped binary printed `Voxel size: 1.0 (no metadata found…)`.
4. `.zattrs` for `-g 0 --scale 1` carries `unit: micrometer` and
   `scale: [1, 8.64, 8.64]`.
5. The TIFF carries `XResolution` ≈ `25400 / 8.64 ≈ 2939.8` px/inch where the
   shipped binary set no resolution tag.
6. **Regression:** on the legacy `PHerc0172` volume, the rendered pixels are
   byte-identical to the shipped binary's and it resolves `7.91`.
7. `ctest` (or the project's suite) green, plus the three live-S3 tests.

Record all of it in `DOCS/RESULTS.md`, replacing §7's entries as they are closed.
Do not delete §7 — mark the closed items.

## 6. Decisions already made — do not silently reverse these

Each was decided on evidence, recorded in `DOCS/ARCHITECTURE.md` §6 and
`DOCS/FEASIBILITY.md`. Reversing one is allowed, but only with new evidence, and
say so.

| Decision | Reason |
|---|---|
| Read `Volume::voxelSize()` from the volume **already opened**, instead of adding a new `remoteVolumeVoxelSize()` fetcher (PR #1417's design) | Zero extra requests; cannot disagree with the volume being streamed; makes the URL-fragment hazard unreachable **by construction** rather than guarded |
| Resolve **after** the volume is open | This ordering *is* the fix — resolving earlier is why the private reader existed |
| Delete the private reader; reuse `vc::metadata::resolveLocalStoreVoxelSize` | It is the shared resolver the rest of the tree already agrees on. Keeping a third opinion is how the drift happened |
| Do **not** change `--voxel-unit`'s default (PR #1313's fix) | Separate blast radius, separately lapsed. Deriving the emitted unit from the value's *source* achieves correct output without it |
| Do **not** change the VC3D GUI predicate | Changes GUI behaviour; its enable-predicate has a lapsed history (#1228). Documented as a follow-up in `DOCS/FEASIBILITY.md` §8 and `DOCS/PR_DRAFT.md` |
| Keep `villa/` git-ignored as a separate clone | It is an upstream checkout with its own `.git`; the committed artefact is `patch/` |
| `.gitattributes` pins `* -text` | Line-ending normalisation would corrupt the patch (git apply is EOL-sensitive) and break `harness/setup.ps1`'s byte-for-byte guarantee |

## 7. Environment: the traps, verified by experience

Full list in `AGENTS.md` §7. The ones that will cost you an hour:

* **Never redirect or pipe a native command.** `cmd > f`, `cmd | x`, `cmd 2>&1`
  all fail with `StandardOutputEncoding is only supported when standard output is
  redirected`. Use the tool's own output option (`git diff --output=…`,
  `node script.mjs --out …`).
* **Git needs a transient flag, and push needs an escalation.** Both this repo and
  `villa/` are owned by `BUILTIN/Administrators`, and `~/.gitconfig` is outside
  the sandbox, so use `tools/git.ps1` (which passes `-c safe.directory='*'`).
  Separately, `git push` needs a stdio pipe that the sandbox denies — every push
  here required a one-shot `danger-full-access` escalation. Ask for it; the
  transfer itself is an ordinary authenticated push via Git Credential Manager.
* **CMake and Ninja are unusable here.** CMake 3.24 cannot launch its own
  subprocesses; upstream needs ≥ 3.28. `harness/build.ps1` invokes `cl.exe`
  directly. Keep it that way.
* **Windows SDK is 10.0.22000.0.** The `10.0.22621.0` paths do not exist here;
  using them yields `fatal error C1083: cannot open include file: 'stdio.h'`.
* **`Invoke-WebRequest` fails TLS.** Node's `fetch` works — that is why the
  download helpers are `.mjs`.
* **Command execution is intermittent.** An external process may return empty
  output with no exit code and then work on retry. Retry once; never record a
  silent failure as evidence.

## 8. What not to do

* Do not touch `..\Vesuvius` (**FitPatch Loop**, suspended by the user). Do not
  read, modify, move, delete or "tidy" it, and do not claim to have verified or
  frozen it.
* Do not push to, or open a pull request against, `ScrollPrize/villa`.
* Do not describe the patch as working. It is **logic-verified, not
  binary-verified**. That sentence, or an equivalent, must survive every edit
  until criterion 1 of §5 is met.
* Do not claim the `vc_grow_seg_from_seed` fix. It is upstream's.
* Do not weaken a test to make it pass. If a test fails, first check whether the
  test encodes a wrong belief — that is how two assertions here were *corrected*,
  making the finding stronger (`DOCS/RESULTS.md` §3.2).
* Do not install heavy components, download more than 2 GB, or use paid services
  without asking.

## 9. Open follow-ups, none of them started

| Item | Where it is documented |
|---|---|
| VC3D GUI predicate: pass the size whenever it is known, not only when rebased | `DOCS/FEASIBILITY.md` §8, `DOCS/PR_DRAFT.md` (exact edit included) |
| Point `core/test/test_volume_live_s3.cpp` at a modern `metadata.json` volume as well as `PHerc0172` | `DOCS/TEST_PLAN.md` §4.5 — the test that would have caught this |
| `vc_zarr_to_tiff.cpp:69-86` has the same schema gap (local-only, top-level key only) | `DOCS/FEASIBILITY.md` §8.3 |
| Open the PR to `villa`; submit for a Progress Prize | `DOCS/PR_DRAFT.md`, `DOCS/SUBMISSION_DRAFT.md`, `DOCS/PROGRESS_PRIZE_CHECKLIST.md` §4.1. **Deadline re-confirmed [live] 2026-09-18: 11:59pm Pacific, 30 Sep 2026** — 12 days out. Earlier revisions of `RESUME.md`, `README.md`, `PRIZE_REQUIREMENTS.md` and `SUBMISSION_DRAFT.md` wrongly said it had passed; corrected in place and recorded in `DOCS/RESULTS.md` §8 |
| Decide the repository licence, and add the third-party Open Data attribution | **Done 2026-09-18.** `LICENSE` (MIT, original work only), `LICENSE-GPL-3.0.txt` (verbatim upstream GPL text), `NOTICE.md` (authoritative path-by-path map, including the two uncertainties that remain open), `DATA_ATTRIBUTION.md` (the data, both dataset citations, the transformations), GPL headers on the four derived files. `RESULTS.md` §14 |
| Send, or not, the prepared question to the prize organisers | `DOCS/PROGRESS_PRIZE_QUESTION.md` — written in English, **not sent**. Only the author can decide |
| Keep the patch artefact honest | `AGENTS.md` §4's regeneration command now names all three paths, and CI asserts the touched-file count. Both exist because the old command silently produced a 2/3 patch (`RESULTS.md` §12.1) |

## 10. Conventions

* Documentation lives in `DOCS/`; only `README.md` and `AGENTS.md` are at the root.
  Cross-references inside `DOCS/` use plain filenames; from the root, prefix with
  `DOCS/`.
* Cite upstream code as **file:line**, and always name the commit, because
  upstream moves.
* Tag new claims `[read]` / `[exec]` / `[live]`. If you cannot tag one, write
  "unverified" in the same sentence.
* Tag the patch files: `[read]` only when the pinned revision is clean. **Do not
  run `git checkout` on `villa/` today** unless you first confirm with the user —
  opening a new session mid-project risks discarding the only in-place copy of the
  change. Recover by regenerating from `patch/vc_render_tifxyz.patch` if it
  happens.

---

## 11. The continuation prompt

Copy the fenced block into a new chat that has this workspace open.

```text
Riprendi il progetto VoxelScale Guard. Workspace:
C:\Users\marco\Documents\DeepSeek\VoxelScaleGuard

Leggi prima, in quest'ordine:
  1. DOCS/RESUME.md            <- punto di partenza: stato, prossimo passo, decisioni
  2. AGENTS.md                 <- regole vincolanti: verifiche, attribuzione, scope, trappole
  3. DOCS/RESULTS.md §7        <- cosa NON è stato verificato
Poi, se ti serve il dettaglio: DOCS/ROOT_CAUSE_ANALYSIS.md, DOCS/FEASIBILITY.md,
DOCS/ARCHITECTURE.md, DOCS/TEST_PLAN.md, DOCS/INDEX.md.

Prima di qualunque cosa, esegui le verifiche di DOCS/RESUME.md §1 (sono read-only)
e riportami l'esito: repository pulito e allineato, i due binari di test che
passano (13/13 e 27/27), il probe che mostra 3 divergenze su 4 volumi, e la patch
che fa round-trip con `git apply --check --reverse`.

Stato al 2026-09-16 (seconda sessione): diagnosi verificata; **la patch è stata
compilata ed eseguita** su runner GitHub Actions, dal commit fissato, contro volumi
pubblicati reali. La scala fisica dichiarata diventa corretta (8.64 micrometri dove
il binario distribuito dichiarava 1 nanometro) e i pixel renderizzati sono
byte-identici. Vedi DOCS/CI_VALIDATION.md e DOCS/RESULTS.md §9.

ATTENZIONE — il dato più importante: la patch **come era stata committata non
compilava**. Il primo tentativo di compilazione è fallito su un errore di uso prima
della dichiarazione (RESULTS.md §9.1). È corretta, e l'harness ora ha un test per
quella classe di difetto. Non presentare la storia come se fosse andata liscia.

Obiettivo di una prossima sessione: il percorso GUI (RESULTS.md §7.4) e copertura
più ampia (due volumi, un crop, una slice finora). Nessuno dei due richiede la
closure vcpkg.

Non serve più scaricare il pacchetto Windows precompilato per ottenere il "prima":
il baseline compilato dallo stesso commit lo fornisce già.

Chiedimi l'autorizzazione prima di:
  - scaricare il pacchetto Windows (~148 MB) o qualunque cosa pesante;
  - installare MSYS2 o la closure vcpkg/Qt/OpenCV (multi-GB);
  - aprire una PR o pubblicare qualunque cosa.

Regole non negoziabili:
  - non toccare ..\Vesuvius (FitPatch Loop, sospeso dall'utente);
  - non modificare né aprire PR contro ScrollPrize/villa;
  - non dichiarare verificato ciò che non hai eseguito; tagga ogni affermazione
    [read] / [exec] / [live] come da AGENTS.md §2;
  - attribuisci il lavoro precedente (NicolasHuberty #1417, Bullo27 #1227/#1228,
    DarthCeltic #1403) come da AGENTS.md §5;
  - non attenuare un test per farlo passare;
  - le decisioni già prese sono in DOCS/RESUME.md §6: non ribaltarle senza nuove
    evidenze, e se lo fai dillo esplicitamente.

Alla fine riporta solo risultati effettivamente ottenuti, e cosa resta bloccato.
```

## 12. Why this file exists

A handoff that only says "read the docs" loses the things that were expensive to
find and cheap to forget: that pushes work from this machine but `cmake` cannot
spawn subprocesses here, that neither of upstream's prebuilt dependency bundles is
anonymously readable, which alternative designs were already rejected and on what
grounds, and — now — that the patch this project spent two sessions describing as
"logic-verified" turned out not to compile the first time anything tried it. Those
are in §5–§7 and in `RESULTS.md` §9. `DOCS/RESULTS.md` §7 remains the authoritative
list of what is unverified, though most of it is now closed; if the two ever
disagree, `RESULTS.md` wins.
