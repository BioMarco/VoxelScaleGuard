# =============================================================================
#  VoxelScale Guard - build the reproducer harness with the local MSVC toolchain.
#
#  Discovered on this machine, all already present:
#    MSVC        14.34.31933        (VS 2022 Community)
#    Windows SDK 10.0.22000.0
#    CMake 3.24 + Ninja 1.11        (bundled with VS 2022)
#
#  CMake and Ninja are NOT used. In this environment CMake cannot launch its own
#  subprocesses ("Accesso negato" for both ninja.exe and, from inside CMake, any
#  probe), so the compiler is invoked directly. This is a harness limitation, not
#  a property of the code under test; CMakeLists.txt is kept for anyone building
#  in a normal environment.
#
#  No download, no install: everything used here is already present.
#
#  Usage:  pwsh -File harness/build.ps1 [-Configuration Release|Debug]
# =============================================================================
[CmdletBinding()]
param(
    [ValidateSet('Release', 'Debug')]
    [string]$Configuration = 'Release'
)

$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $here

$vs  = 'C:\Program Files\Microsoft Visual Studio\2022\Community'
$sdk = 'C:\Program Files (x86)\Windows Kits\10'

$msvcVersion = (Get-ChildItem "$vs\VC\Tools\MSVC" -Directory |
                Sort-Object Name -Descending | Select-Object -First 1).Name
if (-not $msvcVersion) { throw "no MSVC toolset found under $vs\VC\Tools\MSVC" }
$sdkVersion = (Get-ChildItem "$sdk\Include" -Directory |
               Where-Object { Test-Path (Join-Path $_.FullName 'ucrt\stdio.h') } |
               Sort-Object Name -Descending | Select-Object -First 1).Name
if (-not $sdkVersion) { throw "no usable Windows SDK found under $sdk\Include" }

$binDir = "$vs\VC\Tools\MSVC\$msvcVersion\bin\Hostx64\x64"
$cl = "$binDir\cl.exe"
if (-not (Test-Path $cl)) { throw "missing compiler: $cl" }

Write-Host "[build] MSVC $msvcVersion / Windows SDK $sdkVersion / $Configuration"

$env:Path = "$binDir;" + $env:Path
$env:INCLUDE = "$vs\VC\Tools\MSVC\$msvcVersion\include;" +
               "$sdk\Include\$sdkVersion\ucrt;" +
               "$sdk\Include\$sdkVersion\um;" +
               "$sdk\Include\$sdkVersion\shared;" +
               "$sdk\Include\$sdkVersion\winrt;" +
               "$sdk\Include\$sdkVersion\cppwinrt"
$env:LIB = "$vs\VC\Tools\MSVC\$msvcVersion\lib\x64;" +
           "$sdk\Lib\$sdkVersion\ucrt\x64;" +
           "$sdk\Lib\$sdkVersion\um\x64"

$out = Join-Path $here "build\$Configuration"
$objDir = Join-Path $out 'obj'
New-Item -ItemType Directory -Force -Path $out | Out-Null
New-Item -ItemType Directory -Force -Path $objDir | Out-Null

$includeArgs = @(
    '/I', 'src',
    '/I', 'src\villa',
    '/I', 'src\villa\utils\include',
    '/I', 'third_party',
    '/I', 'third_party\doctest'
)

$commonArgs = @(
    '/nologo', '/std:c++20', '/EHsc', '/permissive-', '/utf-8',
    '/W4', '/wd4100'           # unreferenced formal parameter (stub signatures)
)
$commonArgs += if ($Configuration -eq 'Debug') {
    @('/Od', '/MDd', '/D_DEBUG')
} else {
    @('/O2', '/MD', '/DNDEBUG')
}

$sources = @(
    'src\villa\Json.cpp',
    'src\villa\vc\core\util\VoxelSizeMetadata.cpp',
    'src\villa\vc\core\util\RemoteUrl.cpp',
    'src\vsguard\render_voxel_size_resolution.cpp'
)

$targets = @(
    @{ Name = 'test_upstream_voxel_size_metadata'; Main = 'src\villa\test_voxel_size_metadata.cpp' },
    @{ Name = 'test_render_voxel_size';           Main = 'tests\test_render_voxel_size.cpp' },
    @{ Name = 'probe_render_voxel_size';          Main = 'tools\probe_render_voxel_size.cpp' }
)

$failed = @()
foreach ($target in $targets) {
    $targetSources = @($target.Main) + $sources
    $objects = @()
    Write-Host "[build] compiling $($target.Name)"
    foreach ($source in $targetSources) {
        $obj = Join-Path $objDir ((Split-Path -Leaf $source) -replace '\.cpp$', '.obj')
        $objects += $obj
        & $cl @commonArgs @includeArgs '/c' $source ('/Fo:' + $obj)
        if ($LASTEXITCODE -ne 0) { $failed += "$($target.Name):$source"; break }
    }
    if ($failed.Count -gt 0) { continue }

    Write-Host "[build] linking $($target.Name)"
    & $cl @commonArgs ('/Fe:' + (Join-Path $out ($target.Name + '.exe'))) @objects '/link' '/INCREMENTAL:NO'
    if ($LASTEXITCODE -ne 0) { $failed += "$($target.Name):link" }
}

if ($failed.Count -gt 0) { throw "build failed: $($failed -join ', ')" }
Write-Host "[build] OK -> $out"
