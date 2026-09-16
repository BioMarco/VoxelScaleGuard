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
$pairs = @(
    @('volume-cartographer/utils/src/Json.cpp',
      'src/villa/Json.cpp'),
    @('volume-cartographer/utils/include/utils/Json.hpp',
      'src/villa/utils/include/utils/Json.hpp'),
    @('volume-cartographer/core/src/VoxelSizeMetadata.cpp',
      'src/villa/vc/core/util/VoxelSizeMetadata.cpp'),
    @('volume-cartographer/core/include/vc/core/util/VoxelSizeMetadata.hpp',
      'src/villa/vc/core/util/VoxelSizeMetadata.hpp'),
    @('volume-cartographer/core/src/RemoteUrl.cpp',
      'src/villa/vc/core/util/RemoteUrl.cpp'),
    @('volume-cartographer/core/include/vc/core/util/RemoteUrl.hpp',
      'src/villa/vc/core/util/RemoteUrl.hpp'),
    @('volume-cartographer/core/test/test_voxel_size_metadata.cpp',
      'src/villa/test_voxel_size_metadata.cpp'),
    # =====================================================================
    # The files the patch modifies MUST come from git at the pinned commit,
    # never from the working tree.
    #
    # The clone under villa/ is deliberately modified in place to hold the
    # patch (AGENTS.md section 4). Copying from its working tree therefore
    # yields the PATCHED file while labelling it pristine -- which silently
    # made the before/after comparison in the harness compare the patch with
    # itself. tests/test_render_voxel_size.cpp asserts the pristine copy really
    # is unpatched, which is how this was caught.
    # =====================================================================
    @{ Rel = 'volume-cartographer/apps/src/vc_render_tifxyz.cpp'
       Dest = 'src/villa/apps/src/vc_render_tifxyz.cpp'
       FromGit = $true }
)

$haveLocal = Test-Path (Join-Path $villaDir 'volume-cartographer/CMakeLists.txt')
Write-Host "[setup] source: $(if ($haveLocal) { "local clone at $villaDir" } else { $villaRaw })"

foreach ($pair in $pairs) {
    $rel = if ($pair -is [hashtable]) { $pair.Rel } else { $pair[0] }
    $destRel = if ($pair -is [hashtable]) { $pair.Dest } else { $pair[1] }
    $dest = Join-Path $here $destRel
    $destDir = Split-Path -Parent $dest
    if (-not (Test-Path $destDir)) { New-Item -ItemType Directory -Force -Path $destDir | Out-Null }

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
