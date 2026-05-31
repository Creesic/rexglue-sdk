<#
.SYNOPSIS
    Configure the rexglue-sdk CMake project.
.NOTES
    Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
    All rights reserved.
    Licensed under the BSD 3-Clause License.
    See LICENSE file in the project root for full license text.
#>
function Invoke-ReXConfigure {
    [CmdletBinding()]
    param(
        [switch]$NoRepair
    )

    $preset = Get-ReXPreset
    $root = Get-ReXRoot
    Write-Host "=== Configuring with preset: $preset ==="

    $cmd = { cmake --preset $using:preset }
    if ($NoRepair -or -not (Test-ReXAutoRepairEnabled)) {
        & $cmd
        if ($LASTEXITCODE -ne 0) { throw "cmake configure failed (exit $LASTEXITCODE)" }
        return
    }

    Invoke-ReXCmakeWithRepair -CmakeCommand $cmd -SdkRoot $root -FailureLabel 'cmake configure'
}
