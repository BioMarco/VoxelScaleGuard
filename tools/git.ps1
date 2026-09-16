# =============================================================================
#  VoxelScale Guard - git wrapper.
#
#  Why this exists: this workspace is owned by BUILTIN/Administrators (it was
#  created by an elevated process), while the current user is marco. Git refuses
#  to operate on such a repository ("detected dubious ownership"), and the usual
#  fix -- `git config --global --add safe.directory ...` -- cannot be applied
#  because ~/.gitconfig is outside the sandbox.
#
#  So every git invocation here passes a transient safe.directory. This script
#  exists only to keep that flag in one place, and to spell out why.
#
#  Usage:  pwsh -File tools/git.ps1 status
#          pwsh -File tools/git.ps1 add -A
#          pwsh -File tools/git.ps1 commit -m "message"
#          pwsh -File tools/git.ps1 push -u origin main
# =============================================================================
[CmdletBinding()]
param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$GitArgs
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)

if (-not $GitArgs) {
    Write-Host "usage: pwsh -File tools/git.ps1 <git args...>"
    exit 2
}

# `*` trusts any directory for this single invocation only. It is transient: it
# changes no file and persists nothing.
& git -c safe.directory='*' -C $root @GitArgs
exit $LASTEXITCODE
