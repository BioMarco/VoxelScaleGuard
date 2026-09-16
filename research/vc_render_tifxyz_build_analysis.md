# Build analysis: `vc_render_tifxyz` in `villa` @ `757f70c0140a4cfbbbd44975ef09558444b96980`

Read-only inspection only. No build, install, or network access was performed.
Every claim below is tagged `[read]` with `file:line` and a quote, or is marked
"not determinable from the local checkout".

Paths are relative to `villa/` unless absolute.

---

## 1. Target definition

**`[read]` `volume-cartographer/apps/CMakeLists.txt:17-18`**

```cmake
add_executable(vc_render_tifxyz src/vc_render_tifxyz.cpp)
target_link_libraries(vc_render_tifxyz vc_core vc_flattening Boost::program_options TIFF::TIFF)
```

Direct link line: `vc_core`, `vc_flattening`, `Boost::program_options`, `TIFF::TIFF`
(plain signature, so these are *link* dependencies; `vc_core`/`vc_flattening`
propagate their `PUBLIC` deps transitively).

No `target_include_directories`, no target-specific `find_package`. `vc_core`
grants one include path to it: `core/CMakeLists.txt:58`
`target_include_directories(vc_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../apps/src)`
— which is why `#include "RenderPrefetch.hpp"` resolves
(`apps/src/vc_render_tifxyz.cpp:2`).

### 1.1 First-party transitive closure

Edge list, all `[read]`:

| Target | Definition | `PUBLIC` deps | `PRIVATE` deps |
|---|---|---|---|
| `vc_core` | `core/CMakeLists.txt:3` | `opencv_core opencv_imgproc opencv_imgcodecs ${VC_OPENCV_CALIB_TARGETS} opencv_video nlohmann_json::nlohmann_json OpenMP::OpenMP_CXX TIFF::TIFF OpenABF CURL::libcurl` (`:60-72`); `utils` `utils_c3d_codec` `vc_delta3d::vc_delta3d` (`:75-79`); `ws2_32` on WIN32 (`:87-89`) | `Blosc::blosc` `libbacktrace` (`:80-83`) |
| `vc_flattening` | `core/CMakeLists.txt:151` | `vc_core OpenABF` (`:152`) | — |
| `utils` (INTERFACE-ish static lib) | `utils/CMakeLists.txt:1` | `nlohmann_json::nlohmann_json CURL::libcurl zstd::libzstd LZ4::lz4 ZLIB::ZLIB` (`:6-12`) | — |
| `utils_c3d_codec` | `utils/CMakeLists.txt:16` | `c3d` (`:21`) | — |
| `OpenABF` | `libs/OpenABF/CMakeLists.txt:5` | `Eigen3::Eigen` (`:9`) | — |
| `c3d` | `libs/c3d/CMakeLists.txt:3` | — (compiles `c3d.c`; links `m` when not MSVC, `:43-45`) | — |
| `vc_delta3d::vc_delta3d` | fetched, `CMakeLists.txt:686-697` | external | external |

**Closure reached by `vc_render_tifxyz`:** `vc_flattening` → `vc_core` → `utils`,
`utils_c3d_codec` → `c3d`, `OpenABF`, `vc_delta3d::vc_delta3d`, plus every
`PUBLIC` dep of `vc_core`/`utils`.

**Do NOT enter the closure:** `vc_inpaint`, `vc_lasagna`, `vc_atlas`,
`vc_fiber_tracer`, `vc_tracer`. Evidence: they appear as link deps only of *other*
`add_executable` lines — e.g. `apps/CMakeLists.txt:24`
`target_link_libraries(vc_grow_seg_from_seed vc_core vc_tracer Boost::program_options)`
and `:33` `target_link_libraries(vc_tifxyz2obj vc_core vc_inpaint)` — and none of
`vc_core`/`vc_flattening`/`utils` link them
(`core/CMakeLists.txt:151-152`, `:60-83`).

### 1.2 CMake-level caveat: the *project* still needs everything

`[read]` `volume-cartographer/CMakeLists.txt:774-776`

```cmake
add_subdirectory(utils)
add_subdirectory(core)
add_subdirectory(apps)
```

`[read]` `volume-cartographer/apps/CMakeLists.txt:1-8`

```cmake
if(NOT VC_BUILD_APPS)
    return()
endif
...
add_subdirectory(VC3D)
```

`VC3D` is added **unconditionally** once `VC_BUILD_APPS=ON` (`option(... ON)` by
default, `CMakeLists.txt:158`; only forced `OFF` under `VC_ENABLE_COVERAGE`,
`:155-159`). There is no CMake switch that builds the `vc_*` CLI tools while
skipping the Qt GUI. So under the project's own build system, `VC_BUILD_APPS=ON`
both requires Qt (`CMakeLists.txt:517-524`) and requires CGAL
(`CMakeLists.txt:665-667`).

---

## 2. Third-party dependencies actually needed

### 2.1 Dependency table

| Dependency | Kind | Where required | Vendored? |
|---|---|---|---|
| **OpenCV** | **prebuilt binary library** (system / vcpkg), shared | `CMakeLists.txt:531` `find_package(OpenCV REQUIRED)`; `core/CMakeLists.txt:62-66` links `opencv_core opencv_imgproc opencv_imgcodecs ${VC_OPENCV_CALIB_TARGETS} opencv_video`; `ViewController` PCH also lists `opencv2/*.hpp` (`core/CMakeLists.txt:128-130`) | external |
| **libtiff** | **prebuilt binary library** | `CMakeLists.txt:576` `find_package(TIFF REQUIRED)`; link line `apps/CMakeLists.txt:18` `TIFF::TIFF`; `vc_core` `core/CMakeLists.txt:69` | external |
| **Boost** | **prebuilt binary library**, component `program_options` only | `CMakeLists.txt:523` `find_package(Boost CONFIG REQUIRED COMPONENTS program_options)`; `apps/CMakeLists.txt:18`; source `apps/src/vc_render_tifxyz.cpp:23` `#include <boost/program_options.hpp>` | external |
| **Eigen** | **header-only** | `CMakeLists.txt:530` `find_package(Eigen3 REQUIRED)`; `libs/OpenABF/CMakeLists.txt:9` `target_link_libraries(OpenABF INTERFACE Eigen3::Eigen)`; headers used at `libs/OpenABF/include/OpenABF/OpenABF.hpp:1974` `<Eigen/SparseLU>` and `core/src/ABFFlattening.cpp:6` `<Eigen/Core>` | external |
| **nlohmann-json** | **header-only** | `CMakeLists.txt:574`; `core/CMakeLists.txt:67`; `utils/CMakeLists.txt:7`; source `apps/src/vc_render_tifxyz.cpp:15` `#include "utils/Json.hpp"` | external |
| **curl** | **prebuilt binary library** | `CMakeLists.txt:575` `find_package(CURL REQUIRED)`; `core/CMakeLists.txt:71` `CURL::libcurl`; `utils/CMakeLists.txt:8`; `utils/CMakeLists.txt:13` `target_compile_definitions(utils PUBLIC UTILS_HAS_CURL=1)` | external |
| **zlib** | **prebuilt binary library** | `CMakeLists.txt:577` `find_package(ZLIB REQUIRED)`; `utils/CMakeLists.txt:11` `ZLIB::ZLIB`; used in `core/src/VcDataset.cpp:20` `#include <zlib.h>` | external |
| **blosc** (Blosc1) | **prebuilt binary library** | located by hand, `CMakeLists.txt:595-609`: `find_path(BLOSC_INCLUDE_DIR NAMES blosc.h ...)`, `find_library(BLOSC_LIBRARY NAMES blosc ...)`, fatal at `:601-603` if absent, wrapped as imported `Blosc::blosc`; consumed PRIVATE by `vc_core` (`core/CMakeLists.txt:81`) and in `core/src/VcDataset.cpp:17` `#include <blosc.h>` | external |
| **zstd** | **prebuilt binary library** | `find_package(zstd CONFIG REQUIRED)` on MSVC (`CMakeLists.txt:643`) or `pkg_check_modules(zstd REQUIRED IMPORTED_TARGET libzstd)` elsewhere (`:656`); `utils/CMakeLists.txt:9` | external |
| **lz4** | **prebuilt binary library** | `find_package(lz4 CONFIG REQUIRED)` (`CMakeLists.txt:644`) or `pkg_check_modules(lz4 REQUIRED IMPORTED_TARGET liblz4)` (`:657`); `utils/CMakeLists.txt:10` | external |
| **OpenABF** | **header-only** | vendored; `libs/OpenABF/CMakeLists.txt:5-9` is `add_library(OpenABF INTERFACE)` + include dir + Eigen. Only content is `libs/OpenABF/include/OpenABF/OpenABF.hpp` | **vendored in-tree** (`libs/OpenABF/`) |
| **`c3d`** | **built from source by the project** | `libs/c3d/CMakeLists.txt:3` `add_library(c3d c3d.c)`; vendored `libs/c3d/` | **vendored in-tree** |
| **`vc_delta3d`** | **built from source by CMake, fetched over the network** | `CMakeLists.txt:686-697`: `find_package(vc_delta3d CONFIG QUIET)`, else `FetchContent_Declare(vc_delta3d GIT_REPOSITORY https://github.com/ScrollPrize/vc-delta3d.git GIT_TAG v0.1.0)` + `FetchContent_MakeAvailable`. Public dep of `vc_core` (`core/CMakeLists.txt:78`). Genuinely needed: `core/src/VcDataset.cpp:23` includes `vc/core/util/CacheCompression.hpp`, which at `core/include/vc/core/util/CacheCompression.hpp:3` does `#include <vc_delta3d/codec.hpp>` | **external, fetched** |
| **libbacktrace** | built from source by CMake (`ExternalProject_Add`), **Linux-only** | `CMakeLists.txt:724-750`, guarded `if(NOT VC_LIBBACKTRACE AND NOT WIN32)`. Private dep of `vc_core` (`core/CMakeLists.txt:82`) | fetched (skipped on Windows) |
| **OpenMP** | **prebuilt/shims**: MSVC & Clang use an in-tree no-op stub | `CMakeLists.txt:699-712` — for Clang/MSVC: `include_directories(${CMAKE_SOURCE_DIR}/core/openmp_stub)`, `add_library(openmp_stub INTERFACE)`, aliased to `OpenMP::OpenMP_CXX`. Otherwise `find_package(OpenMP REQUIRED)` (`:711`). Source `apps/src/vc_render_tifxyz.cpp:36` `#include <omp.h>` | **vendored in-tree** on MSVC |
| **libigl** | **header-only, fetched** | `CMakeLists.txt:760-762` `if(VC_BUILD_FLATBOI OR VC_BUILD_APPS) include(cmake/FetchLibigl.cmake)`; `cmake/FetchLibigl.cmake:10-16` fetches `https://github.com/libigl/libigl.git` @ `ae8f959e...` | fetched external; **not** linked by `vc_render_tifxyz` |
| **Ceres** | prebuilt binary library | `CMakeLists.txt:529` `find_package(Ceres REQUIRED)` — **unconditional** | external |
| **CGAL** | prebuilt binary/header library | `CMakeLists.txt:665-667` `if(VC_BUILD_APPS) find_package(CGAL REQUIRED)` | external |
| **Qt6** | prebuilt binary library | `CMakeLists.txt:517-518`: `if(VC_BUILD_UI_TRACER OR VC_BUILD_APPS) find_package(Qt6 QUIET REQUIRED COMPONENTS Widgets Gui Core Network Concurrent OpenGLWidgets)` | external |

### 2.2 Qt vs CLI

- `VC3D` links Qt directly: `apps/VC3D/CMakeLists.txt:365-384` includes
  `Qt6::Core Qt6::Concurrent Qt6::Gui Qt6::Widgets Qt6::Network Qt6::OpenGLWidgets`.
- **`vc_render_tifxyz` does not link Qt.** Its link line is
  `apps/CMakeLists.txt:18`. No Qt header is reached through `vc_core` either: the
  only Qt header under `core/` is `core/include/vc/ui/VCCollection.hpp:3`
  `#include <QObject>`, which is a UI-shim header, and `vc_render_tifxyz.cpp` does
  not include it.
- **However** Qt is a hard *configure-time* requirement of the project whenever
  `VC_BUILD_APPS=ON` (`CMakeLists.txt:517-518`), and `VC3D` is added
  unconditionally by `apps/CMakeLists.txt:8`. So in the project's own build, Qt is
  required to get to `vc_render_tifxyz` even though the binary does not link it.
- Qt is also invoked at configure time by `qt6_wrap_cpp` in
  `apps/CMakeLists.txt:177` and `apps/VC3D/CMakeLists.txt:7`, both for VC3D-related
  targets.

### 2.3 Vendored vs external summary

**Vendored in-tree under `libs/`** (present locally, no download):
`libs/OpenABF/` (header-only), `libs/c3d/` (C source), `libs/cc3d/`,
`libs/djikstra3d/`, `libs/edt/`, `libs/flatboi/`, `libs/libigl_changes/`,
`libs/ECL-MaxFlow/` (the latter two/three are not used by `vc_render_tifxyz`).
`villa/.gitmodules` is empty — there are **no git submodules**.

**External (must be obtained):** OpenCV, libtiff, Boost (`program_options`),
Eigen, nlohmann-json, curl, zlib, blosc, zstd, lz4, Ceres, CGAL, Qt6, plus
network fetches of `vc-delta3d` and `libigl`.

**No `add_subdirectory` of any third-party source tree other than the in-tree
`libs/` and the fetched `vc-delta3d`/`libigl`/`pastix`.** Full inventory of
`find_package` / `FetchContent` / `pkg_check_modules` / `ExternalProject_Add` /
`find_path` / `find_library` in every `CMakeLists.txt`:
`CMakeLists.txt:175, 518, 523, 529, 530, 531, 556, 574, 575, 576, 577, 582, 584,
595, 598, 618, 622, 643, 644, 656, 657, 666, 669, 680, 686, 692, 696, 711, 716,
734, 755, 756, 764, 774, 775, 776, 778, 905, 906, 907`;
`apps/CMakeLists.txt:8, 46, 67`; `apps/VC3D/CMakeLists.txt:348, 350, 360`;
`python/CMakeLists.txt:1, 14`; `libs/flatboi/CMakeLists.txt:12, 86`;
`libs/libigl_changes/CMakeLists.txt:70, 75, 83, 154`;
`core/test/CMakeLists.txt:10, 500, 891`;
`apps/VC3D/test/CMakeLists.txt:1`; `apps/VC3D/agent_bridge/test/CMakeLists.txt:3`.

The Windows/MSVC dependency manifest is `volume-cartographer/vcpkg.json:7-33`
(`qtbase`, `boost-program-options`, `boost-graph`, `boost-geometry`,
`ceres[eigensparse]`, `eigen3`, `opencv4[contrib]`, `nlohmann-json`, `curl`,
`tiff`, `zlib`, `blosc`, `zstd`, `lz4`, `cgal`).

---

## 3. Is a minimal build conceivable?

### 3.1 Link time: neither Ceres, CGAL, Qt nor blosc is needed by this binary

**Ceres — not in the link closure.** Ceres appears only as:
- `core/CMakeLists.txt:155` `target_link_libraries(vc_inpaint PUBLIC vc_core PRIVATE Ceres::ceres)`
- `core/CMakeLists.txt:189` `target_link_libraries(vc_lasagna PUBLIC vc_core PRIVATE Ceres::ceres)`
- `core/CMakeLists.txt:196` `target_link_libraries(vc_atlas PUBLIC vc_core vc_lasagna PRIVATE Ceres::ceres)`
- `core/CMakeLists.txt:216` `target_link_libraries(vc_tracer PUBLIC vc_core vc_flattening Ceres::ceres)`
None of `vc_core`, `vc_flattening`, `utils`, `OpenABF`, `c3d` links Ceres
(`core/CMakeLists.txt:60-83, 151-152`; `utils/CMakeLists.txt:6-21`).

**CGAL — not in the link closure.** Only consumer in the whole app set is
`apps/CMakeLists.txt:102` (`vc_add_ignore_label ... CGAL::CGAL`); the only CGAL
includes are `apps/src/alpha_wrap.cpp:3-7`.

**Qt — not in the link closure** (§2.2), though required to configure.

**blosc — yes, needed** (the one third-party that is *not* on the target's own
link line but is a real dependency): `vc_core` links it PRIVATE
(`core/CMakeLists.txt:81`) and `core/src/VcDataset.cpp:17` includes `<blosc.h>`.
With `BUILD_SHARED_LIBS ON` (`CMakeLists.txt:16`), `vc_core` is a shared library,
so blosc is satisfied at *vc_core's* link, not at the executable's — but it must
be present to link the closure at all.

**But the project's own build still requires Ceres and CGAL.** `CMakeLists.txt:529`
`find_package(Ceres REQUIRED)` is unconditional and outside any `if()`.
`CMakeLists.txt:665-667` requires CGAL whenever `VC_BUILD_APPS` is ON. So
"minimal" cannot mean "run `cmake -S . -B build && ninja vc_render_tifxyz` with
fewer deps" — CMake configuration fails without Ceres, CGAL, Qt6, Eigen3, OpenCV,
nlohmann_json, CURL, TIFF, ZLIB, blosc, zstd, lz4, and (unless the tree is
elided) `vc-delta3d` from GitHub.

### 3.2 What a hand-rolled build of this one executable would genuinely need

Determined from the target's declared link deps plus the reachable closures above
(all `[read]`):

**Must link:** `vc_core`, `vc_flattening` (i.e. `core/src/*.cpp` + `core/src/render/*.cpp`
+ `apps/src/vc_merge_tifxyz_grid.cpp` per `core/CMakeLists.txt:3-54`, plus
`core/src/ABFFlattening.cpp`), the vendored `c3d` C source
(`libs/c3d/CMakeLists.txt:3`), `utils`/`utils_c3d_codec` sources
(`utils/CMakeLists.txt:1, 16`), and the fetched `vc_delta3d` codec.

**Must link, third-party:** OpenCV (`opencv_core`, `opencv_imgproc`,
`opencv_imgcodecs`, `opencv_calib3d`, `opencv_video` — `core/CMakeLists.txt:62-66`),
libtiff, Boost `program_options`, libcurl, zlib, blosc, zstd, lz4, and on Windows
`ws2_32` (`core/CMakeLists.txt:87-89`).

**Must be present as headers only:** Eigen (through OpenABF), nlohmann-json.

**Not needed at all for this binary:** Ceres, CGAL, Qt6, libigl, PaStiX/flatboi,
avahi, mimalloc, libbacktrace (Linux-only anyway), CUDA/ECL-MaxFlow, AMGX,
Python/nanobind.

**Not determinable from the local checkout:** whether the OpenCV build pulled in
by a given package manager drags further link-time transitive libs (e.g. a
TBB-backed OpenCV 5 parallel backend) into this binary; and whether `vc_delta3d`
v0.1.0's own CMake introduces additional third-party deps. Neither the OpenCV
package nor the `vc-delta3d` source is present locally.

### 3.3 Practical consequence

A "minimal build" is conceivable only as **not-CMake**: invoking `cl.exe`
directly over the ~55 `vc_core`/`vc_flattening`/`utils`/`c3d` translation units
plus `apps/src/vc_render_tifxyz.cpp`, with hand-specified `/I` paths to OpenCV,
libtiff, Boost, Eigen, nlohmann-json, curl, zlib, blosc, zstd, lz4 and a
hand-written `vc_delta3d` codec file. It is **not** achievable by configuring the
CMake project with a reduced dependency set, because `CMakeLists.txt:529`,
`:517-518` and `:665-667` are unconditional or tied to `VC_BUILD_APPS`, and
`apps/CMakeLists.txt:8` always adds the Qt GUI.

---

## 4. CI recipe: Windows

`[read]` `villa/.github/workflows/vc3d-windows.yml`.

Runner and shell:

- `:43` `runs-on: windows-latest`
- `:42` job name `... (MSYS2 UCRT64)`
- `:51` `shell: C:\msys64\usr\bin\bash.exe -leo pipefail {0}` — i.e. the
  **runner image's pre-installed `C:\msys64`**, which the workflow assumes exists.
- `:52` `working-directory: volume-cartographer`

Steps that obtain dependencies, quoted verbatim:

`:58-62`
```yaml
      - uses: actions/checkout@v4
        with:
          repository: ScrollPrize/vc3d-deps
          ref: d84dfa0745dd2debbcec2126117878b241ffdb7f
          path: vc3d-deps
```

`:64-75`
```yaml
      - uses: oras-project/setup-oras@v1

      - name: Restore prebuilt MSYS2 dependencies
        shell: pwsh
        working-directory: ${{ github.workspace }}
        env:
          GHCR_TOKEN: ${{ github.token }}
        run: |
          $env:GHCR_TOKEN | oras login ghcr.io -u '${{ github.actor }}' --password-stdin
          oras pull ghcr.io/scrollprize/vc3d-deps/windows:sha-d84dfa0745dd2debbcec2126117878b241ffdb7f -o $env:RUNNER_TEMP
          & "$env:GITHUB_WORKSPACE\vc3d-deps\windows\restore.ps1" `
            -Archive "$env:RUNNER_TEMP\vc3d-windows-ucrt64.7z"
```

Then configure/build (`:85-92`):
```yaml
      - name: Configure
        run: >-
          cmake --preset ci-windows-mingw
          -DVC_USE_SCCACHE=ON
          -DSCCACHE_PROGRAM="${SCCACHE_PATH}"

      - name: Build
        run: ninja -C build/ci-windows-mingw
```

**What is fetched from
`ghcr.io/scrollprize/vc3d-deps/windows:sha-d84dfa0745dd2debbcec2126117878b241ffdb7f`:**
a single ORAS artifact pulled into `$RUNNER_TEMP`, which `restore.ps1` consumes as
`-Archive "$env:RUNNER_TEMP\vc3d-windows-ucrt64.7z"`. The file name states it is a
7z archive, named for the UCRT64 MSYS2 environment. **What is inside that archive
is not determinable from the local checkout** — the artifact is not present, and
nothing in the repo enumerates its contents. The only corroborating statement
in-tree is `CMakePresets.json:147`: `"Dependencies come from mingw-w64 pacman
packages"`.

**`vc3d-deps/windows/restore.ps1`: the repository is NOT checked out locally.**
Verified absent: neither `villa/vc3d-deps` nor `<workspace>/vc3d-deps` exists.
Only its *invocation* is visible (above): it takes `-Archive <path to
vc3d-windows-ucrt64.7z>`. Its behaviour — where it unpacks to, whether it runs
`pacman -U`, whether it verifies a checksum — **is not determinable from the local
checkout**. I will not guess.

**Which MSYS2 packages CI relies on:** **not determinable from the local
checkout.** The Windows workflow never runs `pacman`. There is no
`mingw-w64-*` package list anywhere in the repo (a repo-wide grep for
`mingw-w64|pacman -S|msys2` matches only `CMakePresets.json:147` and
`volume-cartographer/README.md:42`, both prose). `volume-cartographer/README.md:42`
says:

> "To build natively from source on Windows, use an MSYS2 UCRT64 shell (see
> [.github/workflows/vc3d-windows.yml](../.github/workflows/vc3d-windows.yml) for
> the package list), then:"

— but that workflow file **contains no package list**; it points at the
`vc3d-deps` archive instead. The README's pointer is therefore stale/incorrect as
of this commit. The packages are implicitly whatever the `vc3d-deps` image
contains. The in-tree `vcpkg.json:7-33` manifest gives the *logical* set for the
MSVC route (Qt base, Boost program_options/graph/geometry, Ceres[eigensparse],
Eigen3, OpenCV4[contrib], nlohmann-json, curl, tiff, zlib, blosc, zstd, lz4,
CGAL) but that is the vcpkg path, not the MSYS2/pacman path.

One more MSYS2 fact CI depends on: `cmake/WindowsPackaging.cmake:20` reads
`set(_vc_mingw_bin "$ENV{MSYSTEM_PREFIX}/bin")` and warns at `:21-23` if
`MSYSTEM_PREFIX` is unset; `:36-38` looks for `windeployqt` under
`"$ENV{MSYSTEM_PREFIX}/bin"` / `"$ENV{MSYSTEM_PREFIX}/share/qt6/bin"`. So the
UCRT64 prefix must contain the full DLL closure plus Qt6's `windeployqt`.

### 4.1 CI test/toolchain facts worth carrying

- `:94-96` installs Python 3.12 for the MCP tests; `:104-105` put
  `C:\msys64\ucrt64\bin` on `PATH` and set
  `QT_PLUGIN_PATH = "C:\msys64\ucrt64\share\qt6\plugins"`.
- `:201-206` the Windows smoke test asserts the packaged CLI tools load:
  `# Console CLI tool: loads the vc_core/OpenCV/Ceres DLL closure.` then
  `& "$bin\vc_tifxyz_trim.exe"` with `if ($trimExitCode -notin 0,1) { throw ... }`.
  Note this is upstream's own words that the CLI closure includes Ceres — but
  `vc_tifxyz_trim` (`apps/CMakeLists.txt:120-121`) links `vc_core opencv_core
  opencv_imgcodecs TIFF::TIFF`, i.e. same shape as `vc_render_tifxyz`; the Ceres
  DLL is in the *packaged tree* because VC3D pulls it in, not because
  `vc_tifxyz_trim` needs it.

---

## 5. Linux route

`villa/.github/workflows/vc3d-linux.yml` does **not** install build dependencies
with apt. It builds inside Docker images:

- `:59` `runs-on: ubuntu-24.04`
- `:68-73` `docker/login-action@v3` against `ghcr.io`
- `:84-89`
  ```yaml
          DOCKER_BUILDKIT=1 docker build \
            -o type=local,dest=dist \
            --build-arg VC_GIT_SHA1="$git_sha1" \
            --build-arg VC_GIT_COMMIT_DATE="$git_date" \
            -f scripts/Dockerfile.appimage \
            ..
  ```
- PRs get a no-op (`:48-54`): `Native compile coverage is provided by the
  required vc3d-ci Linux job.`

The actual native Linux compile/test gate is `villa/.github/workflows/vc3d-ci.yml`,
which runs every build **inside a prebuilt dependency container**:

- `:137-141` (and again at `:187-191`, `:237`, `:306`, `:355`, `:406`)
  ```yaml
        container:
          image: ghcr.io/scrollprize/vc3d-deps/linux:sha-0c371b1d472c5281b703d65517e980d945da693f
          credentials:
            username: ${{ github.actor }}
            password: ${{ secrets.GITHUB_TOKEN }}
  ```
- The only inline apt install in that file is valgrind, `:256-260`:
  `# valgrind entry in scripts/install_build_deps.sh` /
  `apt-get install -y --no-install-recommends valgrind`.
- The container's contents are defined by
  `volume-cartographer/scripts/install_build_deps.sh`, which is what the builder
  image is built from (`volume-cartographer/Dockerfile:12-19`
  `# Build deps live in scripts/install_build_deps.sh — the single source of
  truth`). Its package list, `scripts/install_build_deps.sh:12-25`:
  ```
      build-essential clang lld llvm flang-21 libclang-rt-21-dev mold git cmake ninja-build ccache pkg-config \
      qt6-base-dev \
      libboost-system-dev libboost-program-options-dev \
      libceres-dev libsuitesparse-dev \
      libopencv-dev libopencv-contrib-dev \
      libcgal-dev libmpfr-dev libgmp-dev \
      libblosc-dev libzstd-dev libcurl4-openssl-dev \
      nlohmann-json3-dev libavahi-client-dev \
      liblz4-dev libtiff-dev \
      zlib1g-dev gfortran libopenblas-dev liblapack-dev liblapacke-dev libomp-dev \
      libscotch-dev libscotchmetis-dev libhwloc-dev \
      file bzip2 wget jq valgrind \
      python3 python3-venv
  ```
  plus AWS CLI v2 from `https://awscli.amazonaws.com/...` (`:29-34`).

**Would that route need a Linux machine or container?** Yes. Every Linux build
path in CI is either a Docker build (`vc3d-linux.yml:84`) or a job running inside
a Linux container (`vc3d-ci.yml:137`). There is no Linux-native path that does not
need a Linux host, a container runtime, or a WSL/VM. On this Windows machine there
is no Docker, no WSL distro and no MSYS2, so the Linux route is unavailable
without obtaining one of those.

---

## 6. Docker route

`villa/volume-cartographer/Dockerfile` (41 lines):

- `:1` `# syntax=docker/dockerfile:1.7`
- `:3` `FROM ubuntu:26.04 AS builder`
- `:15-19`
  ```dockerfile
  COPY scripts/install_build_deps.sh /tmp/install_build_deps.sh
  RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
      --mount=type=cache,target=/var/lib/apt,sharing=locked \
      bash /tmp/install_build_deps.sh \
   && rm -f /tmp/install_build_deps.sh
  ```
- `:21-28` a `build` stage that runs `cmake --preset ci-release-gcc`,
  `cmake --build --preset ci-release-gcc` and
  `cmake --install build/ci-release-gcc --prefix /usr/local --component vc_runtime`
  — i.e. **this image does install the CLI tools**, because `vc_runtime` is the
  install component registered for every `EXECUTABLE` and `SHARED_LIBRARY` in
  `core`, `utils`, `libs/c3d` and (when `VC_BUILD_APPS`) `apps`/`apps/VC3D`
  (`CMakeLists.txt:792-832`). `vc_render_tifxyz` is one of them.
- `:30` `FROM build AS runtime`; `:32-36` writes a `vc3d` wrapper script;
  `:38-39` copies `docker_s3_entrypoint.sh`.
- `:10` `LABEL org.opencontainers.image.licenses="GPL-3.0"`.

A published image also exists:
`volume-cartographer/README.md:18-22`
> "Due to a complex set of dependencies, it is *highly* recommended to use the
> docker image ... `docker pull ghcr.io/scrollprize/villa/volume-cartographer:edge`"

`villa/volume-cartographer/build_from_src_debian.sh` (132 lines) is the non-Docker
Debian route. It is explicitly Linux-only: `:19-22`
`if [[ "$(uname -s)" != Linux ]] ...; echo "build_from_src_debian.sh requires a
Debian-family Linux distribution."; exit 1`, and `:26-29` requires
`ID`/`ID_LIKE` to contain `debian`. It refuses to install in agent mode unless
`AGENTS_ALLOW_INSTALL=1` (`:9-12`). Its install list, `:63-94`:
`build-essential ca-certificates gfortran git libavahi-client-dev libblosc-dev
libboost-program-options-dev libboost-system-dev libcgal-dev libceres-dev
libcurl4-openssl-dev libgmp-dev libhwloc-dev liblapack-dev liblapacke-dev
liblz4-dev libmpfr-dev libopenblas-dev libopencv-contrib-dev libopencv-dev
libscotch-dev libscotchmetis-dev libsuitesparse-dev libtiff-dev libzstd-dev
ninja-build nlohmann-json3-dev pkg-config python3 qt6-base-dev zlib1g-dev`.
It then configures with `-DVC_BUILD_APPS=ON -DVC_BUILD_FLATBOI=ON`
(`:107-108`) and requires CMake ≥ 3.28 (`:52-61`).

**Could a Docker image be used to build this?** Yes, in principle — the
`Dockerfile` builds and installs the full `vc_runtime` tree, and the
`ghcr.io/scrollprize/villa/volume-cartographer:edge` image is a published runtime
image. But note the prerequisites:

- A Docker daemon is required. `[read]` the machine facts given in the brief state
  there is no Docker here, so this route is not available on this machine.
- The base image is `ubuntu:26.04` (`Dockerfile:3`), and the Linux CI builder image
  is `ghcr.io/scrollprize/villa/volume-cartographer:builder-ubuntu-26.04`
  (`scripts/Dockerfile.appimage:23`).
- **Roughly how large the dependency install is: not determinable from the local
  checkout.** The Dockerfile gives no size figure, and the apt package set mixes
  Qt6, OpenCV (+contrib), CGAL, Ceres/SuiteSparse, Scotch/METIS, OpenBLAS/LAPACK,
  LLVM/Clang/Flang and AWS CLI — but no byte count appears anywhere in the tree.
  `volume-cartographer/README.md:61-63` describes the analogous vcpkg closure as
  *"The first configure builds the dependency closure (Qt, OpenCV, Ceres, CGAL,
  ...) from source via vcpkg — this takes a while"* without a size. Any GB figure
  would be a guess; I am not giving one.

---

## 7. Gaps: files/artefacts NOT present locally

- `vc3d-deps` repository (either `villa/vc3d-deps` or `<workspace>/vc3d-deps`) —
  **absent**. Therefore `vc3d-deps/windows/restore.ps1` and its contents are
  **not determinable from the local checkout**.
- The ORAS artefact `vc3d-windows-ucrt64.7z` — **absent**; contents not
  determinable.
- Any MSYS2/pacman package list for Windows — **not present anywhere in the repo**.
- No `build/` directory and no CMake cache exist in
  `volume-cartographer/` (only `build_from_src_debian.sh` matches `build*`), so
  there is no local record of a previous configure.
- `libigl` and `vc-delta3d` sources — **absent** (both are network `FetchContent`).
