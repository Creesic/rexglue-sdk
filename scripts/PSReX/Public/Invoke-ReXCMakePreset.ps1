<#
.SYNOPSIS
    Run cmake --preset (or --build) from the current directory with auto-repair on failure.
.PARAMETER Preset
    Configure preset name (required unless -BuildOnly).
.PARAMETER BuildOnly
    Only build; use with -Preset as the configure preset name to locate out/build/<preset>.
.PARAMETER Config
    Build configuration (Debug, Release, RelWithDebInfo). Default: RelWithDebInfo.
.PARAMETER Target
    Optional cmake --target name.
.PARAMETER NoRepair
    Do not run workspace repair and retry on failure.
.NOTES
    Copyright (c) 2026 Tom Clay
    Licensed under the BSD 3-Clause License.
#>
function Invoke-ReXCMakePreset {
    [CmdletBinding()]
    param(
        [string]$Preset,
        [switch]$BuildOnly,
        [ValidateSet('Debug', 'Release', 'RelWithDebInfo')]
        [string]$Config = 'RelWithDebInfo',
        [string]$Target,
        [switch]$NoRepair
    )

    $cwd = (Get-Location).Path
    $sdkRoot = Get-ReXSdkRoot -StartDir $cwd
    if (-not $sdkRoot) { $sdkRoot = Get-ReXRoot }

    if ($BuildOnly) {
        if (-not $Preset) { throw '-Preset is required with -BuildOnly (configure preset directory name)' }
        $buildDir = Join-Path $cwd "out/build/$Preset"
        $args = @('--build', $buildDir, '--config', $Config)
        if ($Target) { $args += @('--target', $Target) }
        $label = "cmake build ($Preset)"
        $cmd = { & cmake @using:args }
    } else {
        if (-not $Preset) { throw '-Preset is required' }
        $label = "cmake configure ($Preset)"
        $cmd = { cmake --preset $using:Preset }
    }

    if ($NoRepair -or -not (Test-ReXAutoRepairEnabled)) {
        & $cmd
        if ($LASTEXITCODE -ne 0) { throw "$label failed (exit $LASTEXITCODE)" }
        return
    }

    Invoke-ReXCmakeWithRepair -CmakeCommand $cmd -SdkRoot $sdkRoot -FailureLabel $label
}
