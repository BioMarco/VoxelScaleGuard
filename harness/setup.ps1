# =============================================================================
#  VoxelScale Guard - pinned setup for the reproducer harness.
#
#  Nothing in this directory is the deliverable. This is scaffolding whose only
#  job is to let us execute the *real* upstream translation units outside a full
#  volume-cartographer build.
#
#  Every file copied from villa is copied byte-for-byte from a pinned commit and
#  is never edited here. If upstream changes, bump $VillaCommit and re-run.
#
#  Usage:  pwsh -File harness/setup.ps1
# =============================================================================
[CmdletBinding()]
param(
    [string]$VillaCommit = '757f70c0140a4cfbbbd44975ef09558444b96980'
)

$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $here

$villaRaw = "https://raw.githubusercontent.com/ScrollPrize/villa/$VillaCommit"
$villaDir = Join-Path $here '..\villa'
$villaDir = [System.IO.Path]::GetFullPath($villaDir)

Write-Host "[setup] pinned villa commit: $VillaCommit"

# --- 1. Translation units and headers reused verbatim from villa -------------
#
# Three sources of truth, and the distinction matters:
#
#   * Most files come from the pinned commit. Byte-for-byte, unmodified.
#
#   * The files the PATCH modifies come from git at the pinned commit, never
#     from a working tree: the clone under villa/ is deliberately modified in
#     place to hold the patch (AGENTS.md section 4), so copying from it would
#     yield the PATCHED file while calling it pristine. That silently made the
#     before/after comparison compare the patch with itself, and
#     tests/test_render_voxel_size.cpp now asserts the pristine copy really is
#     unpatched.
#
#   * A few files are ALSO modified by the proposed PR, ahead of the patch. Those
#     come from the PR branch (tools/fork, branch
#     fix/render-voxel-size-from-open-volume), because the pinned commit predates
#     them and they are the change under test. They are listed explicitly with
#     FromPrBranch so that "which of these differ from upstream, and why" is
#     answerable from this file alone. Each is pinned by the commit hash printed
#     below, and the harness records that hash.
$pairs = @(
    @('volume-cartographer/utils/src/Json.cpp',
      'src/villa/Json.cpp'),
    @('volume-cartographer/utils/include/utils/Json.hpp',
      'src/villa/utils/include/utils/Json.hpp'),
    @('volume-cartographer/core/src/RemoteUrl.cpp',
      'src/villa/vc/core/util/RemoteUrl.cpp'),
    @('volume-cartographer/core/include/vc/core/util/RemoteUrl.hpp',
      'src/villa/vc/core/util/RemoteUrl.hpp'),
    @{ Rel = 'volume-cartographer/apps/src/vc_render_tifxyz.cpp'
       Dest = 'src/villa/apps/src/vc_render_tifxyz.cpp'
       FromGit = $true }
    # Changed by the PR as well as by the patch. The PR restores the legacy
    # `metadata.json -> scan.voxelsize` schema and makes resolveLocalStoreVoxelSize()
    # continue past a candidate file that yields no usable size; the tests for both
    # travel with it.
    @{ Rel = 'volume-cartographer/core/src/VoxelSizeMetadata.cpp'
       Dest = 'src/villa/vc/core/util/VoxelSizeMetadata.cpp'
       FromPrBranch = $true }
    @{ Rel = 'volume-cartographer/core/include/vc/core/util/VoxelSizeMetadata.hpp'
       Dest = 'src/villa/vc/core/util/VoxelSizeMetadata.hpp'
       FromPrBranch = $true }
    @{ Rel = 'volume-cartographer/core/test/test_voxel_size_metadata.cpp'
       Dest = 'src/villa/test_voxel_size_metadata.cpp'
       FromPrBranch = $true }
)

$haveLocal = Test-Path (Join-Path $villaDir 'volume-cartographer/CMakeLists.txt')
Write-Host "[setup] source: $(if ($haveLocal) { "local clone at $villaDir" } else { $villaRaw })"

# --- PR-branch source: tools/fork at the branch the proposal targets ---------
# Resolved once, and printed, so a harness run says which revision of the
# proposed change it exercised rather than leaving that implicit.
$prDir = [System.IO.Path]::GetFullPath((Join-Path $here '..\tools\fork'))
$prCommit = $null
if (Test-Path (Join-Path $prDir 'volume-cartographer/CMakeLists.txt')) {
    $prCommit = (& git -c safe.directory='*' -C $prDir rev-parse HEAD 2>$null)
    if ($LASTEXITCODE -ne 0 -or -not $prCommit) {
        throw "tools/fork is present but not a git checkout; cannot record the PR revision"
    }
    $prCommit = $prCommit.Trim()
    $prBranch = (& git -c safe.directory='*' -C $prDir rev-parse --abbrev-ref HEAD).Trim()
    Write-Host "[setup] PR branch: $prBranch @ $($prCommit.Substring(0,7))"
} else {
    throw ("tools/fork is missing. The harness needs it for the files the proposed " +
           "change modifies ahead of the patch. Clone BioMarco/villa there at " +
           "fix/render-voxel-size-from-open-volume.")
}

foreach ($pair in $pairs) {
    $rel = if ($pair -is [hashtable]) { $pair.Rel } else { $pair[0] }
    $destRel = if ($pair -is [hashtable]) { $pair.Dest } else { $pair[1] }
    $dest = Join-Path $here $destRel
    $destDir = Split-Path -Parent $dest
    if (-not (Test-Path $destDir)) { New-Item -ItemType Directory -Force -Path $destDir | Out-Null }

    if ($pair -is [hashtable] -and $pair.FromPrBranch) {
        # From the PR branch's working tree: these files ARE the proposed change,
        # so they must not come from the pinned commit.
        $src = Join-Path $prDir ($rel -replace '/', '\')
        if (-not (Test-Path $src)) { throw "missing PR-branch file: $src" }
        Copy-Item -Force $src $dest
        Write-Host ("  {0,-72} -> {1}   [PR @ {2}]" -f $rel, $destRel, $prCommit.Substring(0, 7))
        continue
    }

    if ($haveLocal) {
        $src = Join-Path $villaDir ($rel -replace '/', '\')
        if (-not (Test-Path $src)) { throw "missing upstream file: $src" }
        if ($pair -is [hashtable] -and $pair.FromGit) {
            # `git show <commit>:<path>` returns the committed blob, so this is
            # the pinned revision regardless of what the working tree holds. The
            # redirect writes raw bytes: no newline translation, no BOM.
            & git -c safe.directory='*' -C $villaDir show "${VillaCommit}:${rel}" > $dest
            if ($LASTEXITCODE -ne 0) { throw "git show failed for $rel at $VillaCommit" }
            Write-Host ("  {0,-72} -> {1}   [git @ {2}]" -f $rel, $destRel, $VillaCommit.Substring(0, 7))
            continue
        }
        Copy-Item -Force $src $dest
    } else {
        $url = "$villaRaw/$rel"
        Invoke-WebRequest -UseBasicParsing -Uri $url -OutFile $dest
    }
    Write-Host ("  {0,-72} -> {1}" -f $rel, $destRel)
}

# Record the PR revision next to the copies it produced, so a harness run and its
# uploaded artefacts can be tied to a specific commit of the proposed change.
Set-Content -Path (Join-Path $here 'PR_REVISION.txt') -Value $prCommit -NoNewline

# --- 2. Header-only dependencies, fetched at build time ---------------------
$deps = @(
    @('third_party/nlohmann/json.hpp',
      'https://raw.githubusercontent.com/nlohmann/json/v3.11.3/single_include/nlohmann/json.hpp'),
    @('third_party/doctest/doctest/doctest.h',
      'https://raw.githubusercontent.com/doctest/doctest/v2.4.11/doctest/doctest.h')
)
foreach ($dep in $deps) {
    $dest = Join-Path $here $dep[0]
    if (Test-Path $dest) { Write-Host "  present: $($dep[0])"; continue }
    $destDir = Split-Path -Parent $dest
    if (-not (Test-Path $destDir)) { New-Item -ItemType Directory -Force -Path $destDir | Out-Null }
    Write-Host "[setup] fetching $($dep[0])"
    Invoke-WebRequest -UseBasicParsing -Uri $dep[1] -OutFile $dest
}

Write-Host '[setup] OK'
