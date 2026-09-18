# Create the PR branch in the fork, containing ONLY the three patched files.
#
# Authorised: fork + branch push. NOT authorised yet: opening the pull request.
# Everything fails closed: if the staged set is not exactly the three expected
# files, nothing is committed and nothing is pushed.

$ErrorActionPreference = 'Stop'

$forkUrl  = 'https://github.com/BioMarco/villa.git'
$patch    = (Resolve-Path "$PSScriptRoot\..\patch\vc_render_tifxyz.patch").Path
$work     = Join-Path $PSScriptRoot 'fork'
$branch   = 'fix/render-voxel-size-from-open-volume'

$expected = @(
    'volume-cartographer/apps/src/vc_render_tifxyz.cpp',
    'volume-cartographer/core/include/vc/core/util/Zarr.hpp',
    'volume-cartographer/core/src/Zarr.cpp'
)

$git = { param([Parameter(ValueFromRemainingArguments = $true)][string[]]$a) & git -c safe.directory='*' @a }
function GitIn([string]$dir, [string[]]$a) {
    & git -c safe.directory='*' -C $dir @a
    if ($LASTEXITCODE -ne 0) { throw "git $($a -join ' ') failed in $dir" }
}

if (Test-Path $work) { Remove-Item -Recurse -Force $work }

Write-Host "[1/6] cloning the fork"
& git clone --quiet $forkUrl $work 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'clone failed' }

Write-Host "[2/6] creating branch '$branch' from the fork's main"
GitIn $work @('fetch', '--quiet', 'origin', 'main')
GitIn $work @('checkout', '--quiet', '-B', $branch, 'origin/main')

# Identity: a fresh clone has none, and the sandbox cannot write ~/.gitconfig. Use
# the identity this project's own commits already carry (read from this repo, not
# invented), so the fork's history is attributed consistently.
$authorName  = (& git -c safe.directory='*' -C $PSScriptRoot\.. config user.name)
$authorEmail = (& git -c safe.directory='*' -C $PSScriptRoot\.. config user.email)
if (-not $authorName -or -not $authorEmail) {
    throw "cannot read the project's git identity; refusing to invent one"
}
Write-Host "      commit identity: $authorName <$authorEmail>"
GitIn $work @('config', 'user.name', $authorName)
GitIn $work @('config', 'user.email', $authorEmail)

Write-Host "[3/6] checking the patch applies, then applying it"
GitIn $work @('apply', '--check', $patch)
GitIn $work @('apply', $patch)

Write-Host "[4/6] staging ONLY the three expected files"
GitIn $work @('add', '--', $expected[0], $expected[1], $expected[2])

# Fail closed: the staged set must be exactly the expected three.
$staged = (GitIn $work @('diff', '--cached', '--name-only')) | Where-Object { $_ -ne '' }
Write-Host "      staged:"
$staged | ForEach-Object { Write-Host "        $_" }

$unexpected = $staged | Where-Object { $expected -notcontains $_ }
$missing    = $expected | Where-Object { $staged -notcontains $_ }
if ($unexpected) { throw "REFUSING TO COMMIT: unexpected staged file(s): $($unexpected -join ', ')" }
if ($missing)    { throw "REFUSING TO COMMIT: expected file(s) not staged: $($missing -join ', ')" }
if ($staged.Count -ne 3) { throw "REFUSING TO COMMIT: expected 3 staged files, got $($staged.Count)" }

# Nothing else may be modified or untracked.
$status = (GitIn $work @('status', '--porcelain')) | Where-Object { $_ -ne '' }
$outside = $status | Where-Object { $l = $_.Substring(3); $expected -notcontains $l }
if ($outside) { throw "REFUSING TO COMMIT: working tree has unrelated changes: $($outside -join '; ')" }

Write-Host "[5/6] committing"
$msg = @'
vc_render_tifxyz: take the voxel size from the volume that is open, not from a local file

The renderer resolved the physical voxel size BEFORE opening the source volume, from
a local file only, using a reader that recognised a single schema: a top-level
`voxelsize` in meta.json or metadata.json. Most published stores instead record
their resolution as `scan.tomo.acquisition.detector.samplePixelSize`, so the reader
resolved nothing and the render continued at a scale of 1.0 declared with
--voxel-unit's default of "nanometer". The declared physical voxel size was wrong by
1000x or more, while the TIFF got no resolution tag at all. The rendered pixels were
always correct.

Resolve it after the volume is open instead, from the volume already being
streamed:

  * `Volume::voxelSize()` -- Volume construction has already fetched and normalised
    the store's own document, so this costs no extra request and cannot disagree
    with the volume actually being rendered;
  * `vc::metadata::resolveLocalStoreVoxelSize()` -- the shared resolver the rest of
    the tree already uses, for a run with no Volume open.

This makes the URL-fragment hazard raised in review of #1417 unreachable by
construction, because no URL is constructed here.

Also:

  * the `.zattrs` number and its unit are now computed together. writeZarrAttrs
    writes `scale` and `axes[*].unit` from separate arguments and never checked
    that they agree, so a converted micrometre number could be written under the
    caller's unit label -- a silent 1000x error;
  * a non-positive base voxel size now means "unknown" and writeZarrAttrs omits the
    multiscales block, rather than publishing a placeholder scale of 1.0. The TIFF
    resolution tag is likewise left unset;
  * --voxel-unit keeps its meaning: it describes the number given to --voxel-size,
    exactly as before. Sizes taken from metadata are declared in micrometres.

CLI behaviour, output formats and rendered pixels are unchanged.

Credit for the approach of consulting the volume's remote voxel size:
NicolasHuberty, #1417. The samplePixelSize schema handling,
resolveLocalStoreVoxelSize and Volume::voxelSize() semantics: Bullo27 and the villa
maintainers (#1227, #1229, #1454). The VC3D enable-predicate diagnosis: Bullo27,
#1228. Issue reports: DarthCeltic (#1403), Bullo27 (#1226).
'@
$msgFile = Join-Path $work '.git\VSG_COMMIT_MSG'
Set-Content -Path $msgFile -Value $msg -Encoding utf8
GitIn $work @('commit', '--quiet', '-F', $msgFile)
Remove-Item $msgFile -Force

Write-Host "[6/6] verifying the commit contains exactly three files"
$show = GitIn $work @('show', '--stat', '--oneline', 'HEAD')
$show | ForEach-Object { Write-Host "      $_" }
$filesInCommit = (GitIn $work @('show', '--pretty=format:', '--name-only', 'HEAD')) | Where-Object { $_ -ne '' }
if ($filesInCommit.Count -ne 3) { throw "COMMIT HAS $($filesInCommit.Count) FILES, expected 3" }
$bad = $filesInCommit | Where-Object { $expected -notcontains $_ }
if ($bad) { throw "COMMIT CONTAINS UNEXPECTED FILES: $($bad -join ', ')" }

Write-Host ""
Write-Host "branch ready locally. NOT pushing (the caller decides)."
Write-Host "  repo   : $work"
Write-Host "  branch : $branch"
Write-Host "  commit : $((GitIn $work @('rev-parse','HEAD')) -join '')"
