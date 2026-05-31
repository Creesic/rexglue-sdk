<#
.SYNOPSIS
    Internal helpers to detect and fix portable-workspace issues.
.NOTES
    Copyright (c) 2026 Tom Clay
    Licensed under the BSD 3-Clause License.
#>

function Get-ReXSdkRoot {
    param([string]$StartDir)
    if (-not $StartDir) { $StartDir = (Get-Location).Path }
    $dir = [System.IO.Path]::GetFullPath($StartDir)
    while ($true) {
        $marker = Join-Path $dir 'cmake/rexglueConfig.cmake.in'
        if (Test-Path -LiteralPath $marker) { return $dir }
        $parent = Split-Path -Parent $dir
        if (-not $parent -or $parent -eq $dir) { break }
        $dir = $parent
    }
    return $null
}

function Get-ReXTitleProjectRoots {
    param([string]$SdkRoot)
    $roots = [System.Collections.Generic.List[string]]::new()
    foreach ($child in Get-ChildItem -LiteralPath $SdkRoot -Directory -ErrorAction SilentlyContinue) {
        $rexglue = Join-Path $child.FullName 'generated/rexglue.cmake'
        if (Test-Path -LiteralPath $rexglue) {
            $roots.Add($child.FullName)
        }
    }
    return $roots
}

function Get-ReXCMakeBuildRoots {
    param([string]$SdkRoot)
    $list = [System.Collections.Generic.List[string]]::new()
    $sdkBuild = Join-Path $SdkRoot 'out/build'
    if (Test-Path -LiteralPath $sdkBuild) { $list.Add($sdkBuild) }
    foreach ($title in (Get-ReXTitleProjectRoots -SdkRoot $SdkRoot)) {
        $tb = Join-Path $title 'out/build'
        if (Test-Path -LiteralPath $tb) { $list.Add($tb) }
    }
    return $list
}

function Test-ReXStaleCMakeCache {
    param(
        [string]$CacheFile,
        [string]$SdkRoot
    )
    $buildDir = Split-Path -Parent $CacheFile
    $expectedHome = $null
    foreach ($title in (Get-ReXTitleProjectRoots -SdkRoot $SdkRoot)) {
        $presetDir = Join-Path $title 'out/build'
        if ($buildDir.StartsWith($presetDir, [StringComparison]::OrdinalIgnoreCase)) {
            $expectedHome = $title
            break
        }
    }
    if (-not $expectedHome) {
        $sdkPreset = Join-Path $SdkRoot 'out/build'
        if ($buildDir.StartsWith($sdkPreset, [StringComparison]::OrdinalIgnoreCase)) {
            $expectedHome = $SdkRoot
        }
    }

    $lines = Get-Content -LiteralPath $CacheFile -ErrorAction SilentlyContinue
    if (-not $lines) { return $true }

    $cachedHome = $null
    $rexsdkDir = $null
    foreach ($line in $lines) {
        if ($line -match '^CMAKE_HOME_DIRECTORY:INTERNAL=(.+)$') {
            $cachedHome = $Matches[1].Trim()
        }
        if ($line -match '^REXSDK_DIR:.*=(.+)$') {
            $rexsdkDir = $Matches[1].Trim()
        }
    }

    if ($cachedHome) {
        $normCached = [System.IO.Path]::GetFullPath($cachedHome)
        if (-not (Test-Path -LiteralPath $normCached)) { return $true }
        if ($expectedHome) {
            $normExpected = [System.IO.Path]::GetFullPath($expectedHome)
            if ($normCached -ne $normExpected) { return $true }
        }
        if (-not $normCached.StartsWith($SdkRoot, [StringComparison]::OrdinalIgnoreCase)) {
            return $true
        }
    }

    if ($rexsdkDir -and $rexsdkDir -notmatch '^\s*$') {
        if (-not (Test-Path -LiteralPath $rexsdkDir)) { return $true }
        $normSdk = [System.IO.Path]::GetFullPath($rexsdkDir)
        $normRoot = [System.IO.Path]::GetFullPath($SdkRoot)
        if ($normSdk -ne $normRoot) { return $true }
    }

    return $false
}

function Clear-ReXStaleCMakeCaches {
    param(
        [string]$SdkRoot,
        [switch]$WhatIf
    )
    $removed = @()
    foreach ($buildRoot in (Get-ReXCMakeBuildRoots -SdkRoot $SdkRoot)) {
        Get-ChildItem -LiteralPath $buildRoot -Directory -ErrorAction SilentlyContinue | ForEach-Object {
            $cache = Join-Path $_.FullName 'CMakeCache.txt'
            if (-not (Test-Path -LiteralPath $cache)) { return }
            if (-not (Test-ReXStaleCMakeCache -CacheFile $cache -SdkRoot $SdkRoot)) { return }
            if ($WhatIf) {
                $removed += $_.FullName
                Write-Host "[WhatIf] Would remove stale configure dir: $($_.FullName)"
            } else {
                Remove-Item -LiteralPath $_.FullName -Recurse -Force
                $removed += $_.FullName
                Write-Host "[fixed] Removed stale configure dir: $($_.FullName)"
            }
        }
    }
    return $removed
}

function Repair-ReXCMakePresetsFile {
    param(
        [string]$PresetsPath,
        [string]$SdkRoot,
        [switch]$WhatIf
    )
    if (-not (Test-Path -LiteralPath $PresetsPath)) { return @() }
    $fixed = @()
    $raw = Get-Content -LiteralPath $PresetsPath -Raw
    $json = $raw | ConvertFrom-Json
    $changed = $false

    foreach ($preset in $json.configurePresets) {
        if (-not $preset.cacheVariables) { continue }
        $cv = $preset.cacheVariables
        if ($cv.PSObject.Properties.Name -contains 'REXSDK_DIR') {
            $val = [string]$cv.REXSDK_DIR
            if ($val -and [System.IO.Path]::IsPathRooted($val)) {
                $cv.PSObject.Properties.Remove('REXSDK_DIR')
                $changed = $true
                $fixed += "Removed absolute REXSDK_DIR from preset '$($preset.name)' in $(Split-Path -Leaf $PresetsPath)"
            }
        }
        if ($preset.name -match 'windows-amd64-base') {
            $want = '${sourceDir}/../out/install/win-amd64'
            if ($cv.CMAKE_PREFIX_PATH -ne $want) {
                $cv | Add-Member -NotePropertyName 'CMAKE_PREFIX_PATH' -NotePropertyValue $want -Force
                $changed = $true
                $fixed += "Set CMAKE_PREFIX_PATH on '$($preset.name)' in $(Split-Path -Leaf $PresetsPath)"
            }
        }
    }

    if ($changed -and -not $WhatIf) {
        $utf8NoBom = New-Object System.Text.UTF8Encoding $false
        [System.IO.File]::WriteAllText($PresetsPath, ($json | ConvertTo-Json -Depth 20), $utf8NoBom)
        foreach ($msg in $fixed) { Write-Host "[fixed] $msg" }
    } elseif ($changed -and $WhatIf) {
        foreach ($msg in $fixed) { Write-Host "[WhatIf] $msg" }
    }
    return $fixed
}

function Repair-ReXTomlPathsInProject {
    param(
        [string]$ProjectRoot,
        [switch]$WhatIf
    )
    $fixed = @()
    $tomls = Get-ChildItem -LiteralPath $ProjectRoot -Filter '*.toml' -File -ErrorAction SilentlyContinue
    foreach ($toml in $tomls) {
        $lines = @(Get-Content -LiteralPath $toml.FullName -ErrorAction SilentlyContinue)
        if ($lines.Count -eq 0) { continue }
        $newLines = [System.Collections.Generic.List[string]]::new()
        $fileChanged = $false
        foreach ($line in $lines) {
            $updated = $line
            if ($line -match '^(?<key>file_path|game_root)\s*=\s*"(?<val>[^"]+)"\s*$') {
                $key = $Matches.key
                $val = $Matches.val -replace '\\\\', '\'
                if ([System.IO.Path]::IsPathRooted($val)) {
                    $candidate = $val
                    if (Test-Path -LiteralPath $candidate) {
                        $rel = [System.IO.Path]::GetRelativePath($ProjectRoot, $candidate)
                        $rel = ($rel -replace '\\', '/')
                        $updated = "$key = `"$rel`""
                        $fileChanged = $true
                    } elseif ($val -match '[/\\]assets[/\\]default\.xex$') {
                        $updated = 'file_path = "assets/default.xex"'
                        $fileChanged = $true
                    }
                }
            }
            $newLines.Add($updated)
        }
        if ($fileChanged) {
            $msg = "Relativized paths in $(Split-Path -Leaf $toml.FullName)"
            if ($WhatIf) {
                Write-Host "[WhatIf] $msg"
            } else {
                Set-Content -LiteralPath $toml.FullName -Value $newLines -Encoding utf8
                Write-Host "[fixed] $msg"
            }
            $fixed += $msg
        }
    }
    return $fixed
}

function Repair-ReXGeneratedRexglueCmake {
    param(
        [string]$CmakePath,
        [switch]$WhatIf
    )
    if (-not (Test-Path -LiteralPath $CmakePath)) { return @() }
    $content = Get-Content -LiteralPath $CmakePath -Raw
    if ($content -match 'rexglueConfig\.cmake\.in') { return @() }

    $portableBlock = @'
# Find SDK (portable: explicit cache, parent monorepo checkout, then install prefix)
set(REXSDK_DIR "" CACHE PATH "Path to rexglue-sdk source tree")
set(_rexglue_sdk_root "")
if(REXSDK_DIR)
    set(_rexglue_sdk_root "${REXSDK_DIR}")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/rexglueConfig.cmake.in")
    get_filename_component(_rexglue_sdk_root "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
endif()
if(_rexglue_sdk_root)
    add_subdirectory("${_rexglue_sdk_root}" rexglue-sdk)
    message(STATUS "Using ReXGlue SDK from source tree: ${_rexglue_sdk_root}")
else()
'@

    $legacyPattern = '(?s)# Find SDK\r?\nset\(REXSDK_DIR.*?if\(REXSDK_DIR\)\r?\n\s+add_subdirectory\("\$\{REXSDK_DIR\}".*?\r?\nelse\(\)\r?\n'
    if ($content -notmatch $legacyPattern) {
        return @("Outdated generated/rexglue.cmake at $CmakePath - run 'rexglue migrate' or regenerate from SDK template")
    }

    $fixed = @()
    $newContent = [regex]::Replace($content, $legacyPattern, $portableBlock)
    if ($newContent -eq $content) { return $fixed }

    $msg = "Patched portable SDK discovery in $(Split-Path -Leaf (Split-Path -Parent $CmakePath))/rexglue.cmake"
    if ($WhatIf) {
        Write-Host "[WhatIf] $msg"
    } else {
        $utf8NoBom = New-Object System.Text.UTF8Encoding $false
        [System.IO.File]::WriteAllText($CmakePath, $newContent, $utf8NoBom)
        Write-Host "[fixed] $msg"
    }
    $fixed += $msg
    return $fixed
}

function Ensure-ReXCMakeUserPresets {
    param(
        [string]$SdkRoot,
        [switch]$WhatIf
    )
    $dst = Join-Path $SdkRoot 'CMakeUserPresets.json'
    if (Test-Path -LiteralPath $dst) { return @() }
    $src = Join-Path $SdkRoot 'CMakeUserPresets.json.example'
    if (-not (Test-Path -LiteralPath $src)) { return @() }
    $msg = 'Created CMakeUserPresets.json from example (edit compiler paths for this machine)'
    if ($WhatIf) {
        Write-Host "[WhatIf] $msg"
    } else {
        Copy-Item -LiteralPath $src -Destination $dst
        Write-Host "[fixed] $msg"
    }
    return @($msg)
}

function Sync-ReXTitleRuntimeDll {
    param(
        [string]$SdkRoot,
        [string]$Preset = 'win-amd64',
        [switch]$WhatIf
    )
    $src = Join-Path $SdkRoot "out/install/$Preset/bin/rexruntimerd.dll"
    if (-not (Test-Path -LiteralPath $src)) { return @() }
    $synced = @()
    foreach ($title in (Get-ReXTitleProjectRoots -SdkRoot $SdkRoot)) {
        $buildRoot = Join-Path $title 'out/build'
        if (-not (Test-Path -LiteralPath $buildRoot)) { continue }
        Get-ChildItem -LiteralPath $buildRoot -Directory -ErrorAction SilentlyContinue | ForEach-Object {
            $dst = Join-Path $_.FullName 'rexruntimerd.dll'
            $copy = $false
            if (-not (Test-Path -LiteralPath $dst)) {
                $copy = $true
            } else {
                $srcTime = (Get-Item -LiteralPath $src).LastWriteTimeUtc
                $dstTime = (Get-Item -LiteralPath $dst).LastWriteTimeUtc
                if ($srcTime -gt $dstTime) { $copy = $true }
            }
            if (-not $copy) { return }
            $msg = "Synced rexruntimerd.dll -> $($_.FullName)"
            if ($WhatIf) {
                Write-Host "[WhatIf] $msg"
            } else {
                Copy-Item -LiteralPath $src -Destination $dst -Force
                Write-Host "[fixed] $msg"
            }
            $synced += $msg
        }
    }
    return $synced
}

function Invoke-ReXRepairWorkspaceCore {
    [CmdletBinding()]
    param(
        [string]$SdkRoot,
        [string]$WorkingDirectory,
        [switch]$WhatIf,
        [switch]$SkipDllSync
    )

    if (-not $SdkRoot) {
        $SdkRoot = Get-ReXSdkRoot -StartDir $WorkingDirectory
    }
    if (-not $SdkRoot) {
        $SdkRoot = Get-ReXRoot
    }
    $SdkRoot = [System.IO.Path]::GetFullPath($SdkRoot)

    $report = [ordered]@{
        SdkRoot       = $SdkRoot
        FixesApplied  = [System.Collections.Generic.List[string]]::new()
        Warnings      = [System.Collections.Generic.List[string]]::new()
        CachesRemoved = @()
    }

    Write-Host "=== ReXGlue workspace repair ==="
    Write-Host "SDK root: $($report.SdkRoot)"
    Write-Host ""

    $report.CachesRemoved = @(Clear-ReXStaleCMakeCaches -SdkRoot $SdkRoot -WhatIf:$WhatIf)
    foreach ($c in $report.CachesRemoved) {
        if (-not $WhatIf) { $report.FixesApplied.Add("Removed stale cache: $c") }
    }

    $presets = @(Join-Path $SdkRoot 'CMakePresets.json')
    foreach ($title in (Get-ReXTitleProjectRoots -SdkRoot $SdkRoot)) {
        $presets += (Join-Path $title 'CMakePresets.json')
    }
    foreach ($p in ($presets | Select-Object -Unique)) {
        $f = Repair-ReXCMakePresetsFile -PresetsPath $p -SdkRoot $SdkRoot -WhatIf:$WhatIf
        foreach ($x in $f) { if (-not $WhatIf) { $report.FixesApplied.Add($x) } }
    }

    foreach ($title in (Get-ReXTitleProjectRoots -SdkRoot $SdkRoot)) {
        $f = Repair-ReXTomlPathsInProject -ProjectRoot $title -WhatIf:$WhatIf
        foreach ($x in $f) { if (-not $WhatIf) { $report.FixesApplied.Add($x) } }
        $gen = Join-Path $title 'generated/rexglue.cmake'
        $f2 = Repair-ReXGeneratedRexglueCmake -CmakePath $gen -WhatIf:$WhatIf
        foreach ($x in $f2) {
            if ($x -match '^Patched') {
                if (-not $WhatIf) { $report.FixesApplied.Add($x) }
            } else {
                $report.Warnings.Add($x)
            }
        }
    }

    $f3 = Ensure-ReXCMakeUserPresets -SdkRoot $SdkRoot -WhatIf:$WhatIf
    foreach ($x in $f3) { if (-not $WhatIf) { $report.FixesApplied.Add($x) } }

    if (-not $SkipDllSync) {
        $preset = if ($env:REXGLUE_PRESET) { $env:REXGLUE_PRESET } else { Get-ReXPreset }
        $f4 = Sync-ReXTitleRuntimeDll -SdkRoot $SdkRoot -Preset $preset -WhatIf:$WhatIf
        foreach ($x in $f4) { if (-not $WhatIf) { $report.FixesApplied.Add($x) } }
    }

    $fixCount = $report.FixesApplied.Count + $(if ($WhatIf) { $report.CachesRemoved.Count } else { 0 })
    Write-Host ""
    if ($fixCount -eq 0 -and $report.Warnings.Count -eq 0) {
        Write-Host "No portable-workspace issues detected."
    } elseif ($WhatIf) {
        Write-Host "WhatIf complete. Re-run without -WhatIf to apply fixes."
    } else {
        Write-Host "Applied $($report.FixesApplied.Count) fix(es)."
        if ($report.Warnings.Count -gt 0) {
            Write-Host "Warnings:"
            foreach ($w in $report.Warnings) { Write-Host "  - $w" }
        }
    }
    Write-Host ""

    return [pscustomobject]$report
}
