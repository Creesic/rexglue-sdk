<#
.SYNOPSIS
    Detect and fix portable-workspace issues (paths, caches, presets, TOML).
.DESCRIPTION
    Scans the SDK tree and title projects for common copy/paste breakage:
    stale CMake caches, absolute REXSDK_DIR in presets, absolute file_path in
    TOML, outdated generated/rexglue.cmake, missing CMakeUserPresets.json, and
    stale rexruntimerd.dll copies.

    Configure/build commands call this automatically on failure unless
    REXGLUE_SKIP_AUTO_REPAIR=1 is set.
.PARAMETER SdkRoot
    SDK repository root (auto-detected when omitted).
.PARAMETER WhatIf
    Report fixes without applying them.
.PARAMETER SkipDllSync
    Do not copy rexruntimerd.dll into title out/build directories.
.NOTES
    Copyright (c) 2026 Tom Clay
    Licensed under the BSD 3-Clause License.
#>
function Invoke-ReXRepairWorkspace {
    [CmdletBinding()]
    param(
        [string]$SdkRoot,
        [switch]$WhatIf,
        [switch]$SkipDllSync
    )

    Invoke-ReXRepairWorkspaceCore -SdkRoot $SdkRoot -WhatIf:$WhatIf -SkipDllSync:$SkipDllSync
}
