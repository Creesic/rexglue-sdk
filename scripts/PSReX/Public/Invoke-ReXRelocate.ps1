<#
.SYNOPSIS
    Clear stale CMake configure caches after moving or copying the repo.
.DESCRIPTION
    CMake caches absolute paths from the machine and directory where configure
    last ran. After copy/paste or drive-letter changes, delete out/build trees
    and re-configure. This script removes CMakeCache.txt directories under
    common ReXGlue output locations.
.PARAMETER RepoRoot
    Root of the rexglue-sdk tree (defaults to Get-ReXRoot).
.PARAMETER WhatIf
    List directories that would be removed without deleting them.
.NOTES
    Copyright (c) 2026 Tom Clay
    Licensed under the BSD 3-Clause License.
#>
function Invoke-ReXRelocate {
    [CmdletBinding()]
    param(
        [string]$RepoRoot,
        [switch]$WhatIf
    )

    $ErrorActionPreference = "Stop"

    if (-not $RepoRoot) { $RepoRoot = Get-ReXRoot }
    Write-Host "=== ReXGlue relocate (stale CMake caches only) ==="
    Write-Host "For full checks use: rex-repair"
    Write-Host ""

    $null = Clear-ReXStaleCMakeCaches -SdkRoot $RepoRoot -WhatIf:$WhatIf
}
