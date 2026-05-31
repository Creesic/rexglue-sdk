<#
.SYNOPSIS
    Run cmake and optionally repair the workspace then retry once.
.NOTES
    Copyright (c) 2026 Tom Clay
    Licensed under the BSD 3-Clause License.
#>

function Test-ReXAutoRepairEnabled {
    return $env:REXGLUE_SKIP_AUTO_REPAIR -ne '1'
}

function Invoke-ReXCmakeWithRepair {
    param(
        [Parameter(Mandatory)]
        [scriptblock]$CmakeCommand,
        [string]$SdkRoot,
        [string]$FailureLabel = 'cmake'
    )

    & $CmakeCommand
    if ($LASTEXITCODE -eq 0) { return }

    if (-not (Test-ReXAutoRepairEnabled)) {
        throw "$FailureLabel failed (exit $LASTEXITCODE); auto-repair disabled (REXGLUE_SKIP_AUTO_REPAIR=1)"
    }

    Write-Host ""
    Write-Host "=== $FailureLabel failed - running workspace repair and retrying once ===" -ForegroundColor Yellow
    Write-Host ""

    $report = Invoke-ReXRepairWorkspaceCore -SdkRoot $SdkRoot
    $hadFixes = ($report.FixesApplied.Count -gt 0) -or ($report.CachesRemoved.Count -gt 0)

    if (-not $hadFixes -and $report.Warnings.Count -eq 0) {
        throw "$FailureLabel failed (exit $LASTEXITCODE); repair found nothing to fix"
    }

    & $CmakeCommand
    if ($LASTEXITCODE -ne 0) {
        throw "$FailureLabel failed again after repair (exit $LASTEXITCODE)"
    }
}
