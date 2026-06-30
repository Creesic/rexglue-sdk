# Run DOAX in bootstrap mode, wait for the session to end, report discoveries,
# then merge + codegen + build only when new addresses were recorded.
param(
    [int]$MaxRounds = 20,
    [string]$Preset = "win-amd64-relwithdebinfo",
    [switch]$Timed,
    [int]$RunSeconds = 180,
    [switch]$NoPause,
    [switch]$ForceCodegen
)

$ErrorActionPreference = "Stop"
$doaxRoot = Split-Path $PSScriptRoot -Parent
$sdkRoot = Split-Path $doaxRoot -Parent
$buildDir = Join-Path $doaxRoot "out\build\$Preset"
$manifest = Join-Path $doaxRoot "doax_manifest.toml"
$exe = Join-Path $buildDir "doax.exe"
$initCpp = Join-Path $doaxRoot "generated\default\doax_init.cpp"
$rexglueCandidates = @(
    (Join-Path $sdkRoot "out\win-amd64\rexglue.exe"),
    (Join-Path $sdkRoot "out\install\win-amd64\bin\rexglue.exe"),
    (Join-Path $sdkRoot "out\win-amd64\RelWithDebInfo\rexglue.exe")
)
$rexglue = $rexglueCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
$assets = (Resolve-Path -LiteralPath (Join-Path $doaxRoot "assets")).Path
$discovered = Join-Path $doaxRoot "bootstrap_discovered.toml"
$logDir = Join-Path $doaxRoot "logs"
$logFile = Join-Path $logDir ("bootstrap-loop-{0:yyyyMMdd-HHmmss}.log" -f (Get-Date))

$script:ExitCode = 0
$script:ExitMessage = "Finished."

function Write-Log {
    param([string]$Message, [string]$Color = "Gray")
    $line = "[{0}] {1}" -f (Get-Date -Format "HH:mm:ss"), $Message
    Write-Host $line -ForegroundColor $Color
    Add-Content -Path $logFile -Value $line -Encoding UTF8
}

function Get-CommandLineText {
    param($Item)
    if ($Item -is [System.Management.Automation.ErrorRecord]) {
        if ($null -ne $Item.Exception -and $Item.Exception.Message) {
            return $Item.Exception.Message
        }
        return $Item.ToString()
    }
    return "$Item"
}

function Get-UniqueAddresses {
    param([string]$Path)
    $seen = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    if (-not (Test-Path $Path)) {
        return $seen
    }
    foreach ($line in [System.IO.File]::ReadLines($Path)) {
        if ($line -match '^\s*(0x[0-9A-Fa-f]+)\s*=') {
            [void]$seen.Add($Matches[1].ToUpperInvariant())
        }
    }
    return $seen
}

function Get-StubCount {
    param([string]$Path)
    return (Get-UniqueAddresses $Path).Count
}

function Repair-BootstrapDiscoveredToml {
    param([string]$Path)
    if (-not (Test-Path $Path)) { return 0 }

    $addresses = [System.Collections.Generic.SortedSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $lineCount = 0
    foreach ($line in [System.IO.File]::ReadLines($Path)) {
        if ($line -match '^\s*(0x[0-9A-Fa-f]+)\s*=') {
            $lineCount++
            [void]$addresses.Add($Matches[1].ToUpperInvariant())
        }
    }

    if ($lineCount -le $addresses.Count) {
        return 0
    }

    $tmp = "$Path.tmp"
    $lines = @(
        "# Auto-discovered guest functions (bootstrap mode)",
        "[functions]"
    )
    foreach ($addr in $addresses) {
        $lines += "$addr = {}"
    }
    Set-Content -Path $tmp -Value $lines -Encoding UTF8
    Move-Item -LiteralPath $tmp -Destination $Path -Force
    return ($lineCount - $addresses.Count)
}

function Test-CodegenOutputComplete {
    return (Test-Path $initCpp) -and (Test-Path (Join-Path $doaxRoot "generated\default\doax_register.cpp"))
}

function Test-ShouldCodegenAfterSession {
    param(
        [int]$NewRuntime,
        [string[]]$NewAddresses
    )

    if ($ForceCodegen) {
        Write-Log "Codegen forced by -ForceCodegen" "Yellow"
        return $true
    }

    if ($NewRuntime -gt 0) {
        return $true
    }

    return $false
}

function Invoke-ExternalCommand {
    param(
        [string]$Label,
        [string]$LogName,
        [scriptblock]$Command
    )

    Write-Log $Label "Cyan"
    $cmdLog = Join-Path $logDir ("{0}-{1:yyyyMMdd-HHmmss}.log" -f $LogName, (Get-Date))

    $prevEap = $ErrorActionPreference
    $exitCode = 0

    Push-Location $doaxRoot
    try {
        $ErrorActionPreference = 'Continue'
        $output = & $Command 2>&1
        $exitCode = $LASTEXITCODE
        $output | Tee-Object -FilePath $cmdLog | ForEach-Object {
            $text = Get-CommandLineText $_
            if ($text -match '(?i)\berror\b|\bfailed\b|\bfatal\b|ANALYSIS ERRORS') {
                Write-Host $text -ForegroundColor Red
            }
            elseif ($text -match '(?i)\bwarn\b|continuing due to --force|Bootstrap merge') {
                Write-Host $text -ForegroundColor Yellow
            }
        }
    }
    finally {
        $ErrorActionPreference = $prevEap
        Pop-Location
    }

    if ($exitCode -ne 0) {
        Write-Log "$Label failed (exit $exitCode). Full output: $cmdLog" "Red"
        throw "$Label failed with exit code $exitCode"
    }

    Write-Log "$Label succeeded. Full output: $cmdLog" "Green"
}

function Copy-RuntimeDll {
    $runtimeDll = Join-Path $sdkRoot "out\win-amd64\RelWithDebInfo\rexruntimerd.dll"
    if (-not (Test-Path $runtimeDll)) {
        $runtimeDll = Join-Path $sdkRoot "out\win-amd64\rexruntimerd.dll"
    }
    if (-not (Test-Path $runtimeDll)) {
        $runtimeDll = Join-Path $sdkRoot "out\install\win-amd64\bin\rexruntimerd.dll"
    }
    if (Test-Path $runtimeDll) {
        Copy-Item -LiteralPath $runtimeDll -Destination (Join-Path $buildDir "rexruntimerd.dll") -Force
        Write-Log "Copied runtime: $runtimeDll" "Gray"
    }
    else {
        Write-Log "rexruntimerd.dll not found; skipping copy" "Yellow"
    }
}

function Wait-ForBootstrapContinue {
    param([System.Diagnostics.Process]$Process)

    if ($Timed) {
        Write-Log "Running doax in bootstrap mode for $RunSeconds seconds (-Timed)..." "Cyan"
        Wait-Process -Id $Process.Id -Timeout $RunSeconds -ErrorAction SilentlyContinue
        if (-not $Process.HasExited) {
            Stop-Process -Id $Process.Id -Force -ErrorAction SilentlyContinue
            Write-Log "Stopped doax after timeout" "Yellow"
        }
        else {
            Write-Log "doax exited on its own (exit $($Process.ExitCode))" "Yellow"
        }
        return
    }

    Write-Host ""
    Write-Host "doax is running in bootstrap mode (pid $($Process.Id))." -ForegroundColor Cyan
    Write-Host "Explore in the game window. When finished, return to THIS console and press any key" -ForegroundColor Cyan
    Write-Host "to end the bootstrap session and continue the loop (or close the game window)." -ForegroundColor Cyan
    Write-Host ""

    while (-not $Process.HasExited) {
        $keyAvailable = $false
        try {
            $keyAvailable = [Console]::KeyAvailable
        } catch [System.InvalidOperationException] {
            # No interactive console (e.g. redirected stdin); wait for process to exit naturally.
            Wait-Process -Id $Process.Id -ErrorAction SilentlyContinue
            break
        }
        if ($keyAvailable) {
            $null = [Console]::ReadKey($true)
            break
        }
        Start-Sleep -Milliseconds 100
        try { $Process.Refresh() } catch { break }
    }

    if (-not $Process.HasExited) {
        Write-Log "Stopping doax after console keypress..." "Yellow"
        Stop-Process -Id $Process.Id -Force -ErrorAction SilentlyContinue
        Wait-Process -Id $Process.Id -ErrorAction SilentlyContinue
    }
    else {
        Write-Log "doax exited on its own (exit $($Process.ExitCode))" "Gray"
    }
}

function Invoke-BootstrapRun {
    if (-not (Test-Path $exe)) {
        throw "Missing executable: $exe (build doax first)"
    }

    $runArgs = @(
        "--game_data_root", $assets,
        "--bootstrap_unregistered_functions=1",
        "--bootstrap_functions_log=$discovered"
    )

    Write-Log "Launching doax in bootstrap mode..." "Cyan"
    Write-Log "  exe: $exe" "Gray"
    Write-Log "  cwd: $buildDir" "Gray"

    $proc = Start-Process -FilePath $exe -ArgumentList $runArgs -PassThru -WorkingDirectory $buildDir
    if (-not $proc) {
        throw "Start-Process failed for $exe"
    }

    Start-Sleep -Milliseconds 500
    if ($proc.HasExited) {
        throw @"
doax exited immediately (exit $($proc.ExitCode)).
Check that rexruntimerd.dll is beside doax.exe, assets exist under $assets,
and run doax.exe manually from $buildDir for details.
"@
    }

    Wait-ForBootstrapContinue -Process $proc
}

function Wait-ForUser {
    if ($NoPause) { return }
    Write-Host ""
    Read-Host "Press Enter to close"
}

try {
    New-Item -ItemType Directory -Force -Path $logDir | Out-Null
    Write-Log "DOAX bootstrap loop starting (max $MaxRounds rounds)" "Cyan"
    Write-Log "Flow: run game -> report discoveries -> merge/codegen/build only if new addresses" "Gray"
    Write-Log "Log file: $logFile"
    if ($rexglue) {
        Write-Log "Using rexglue: $rexglue" "Gray"
    }

    if (-not $rexglue) {
        throw @"
rexglue.exe not found. Build the SDK from the repo root first:
  cd $sdkRoot
  cmake --preset win-amd64
  cmake --build --preset win-amd64-relwithdebinfo --target rexglue
"@
    }

    if (-not (Test-CodegenOutputComplete)) {
        Write-Log "Warning: generated output is missing or incomplete; bootstrap run may fail until you codegen once manually or pass -ForceCodegen after discoveries" "Yellow"
    }

    $manifestStubs = Get-StubCount $manifest
    $round = 0

    while ($round -lt $MaxRounds) {
        $round++
        Write-Log "=== Round $round / $MaxRounds ===" "Cyan"

        if (-not (Test-Path $exe)) {
            Write-Log "Building doax (link only; no codegen yet)" "Yellow"
            Invoke-ExternalCommand -Label "Build doax" -LogName "build" -Command {
                cmake --build --preset $Preset --target doax
            }
            Copy-RuntimeDll
        }

        $beforeDiscovered = Get-UniqueAddresses $discovered
        Copy-RuntimeDll
        Invoke-BootstrapRun

        $removedDupes = Repair-BootstrapDiscoveredToml $discovered
        if ($removedDupes -gt 0) {
            Write-Log "Compacted bootstrap_discovered.toml ($removedDupes duplicate line(s) removed)" "Yellow"
        }

        $afterDiscovered = Get-UniqueAddresses $discovered
        $newAddresses = @($afterDiscovered | Where-Object { -not $beforeDiscovered.Contains($_) })
        $newRuntime = $newAddresses.Count

        Write-Log "Session finished: $newRuntime new unique address(es) in bootstrap_discovered.toml" $(if ($newRuntime -gt 0) { "Green" } else { "Gray" })
        if ($newRuntime -gt 0) {
            foreach ($addr in $newAddresses) {
                Write-Log "  + $addr" "Green"
            }
        }

        if (-not (Test-ShouldCodegenAfterSession -NewRuntime $newRuntime -NewAddresses $newAddresses)) {
            Write-Log "No new discoveries; skipping manifest merge, codegen, and build" "Green"
            if ($newRuntime -eq 0) {
                Write-Log "Bootstrap loop complete" "Green"
            }
            break
        }

        $manifestStubsBefore = Get-StubCount $manifest
        Write-Log "Merging discoveries into doax_manifest.toml and running codegen ($manifestStubsBefore manifest stub(s) before)" "Cyan"

        Invoke-ExternalCommand -Label "Codegen" -LogName "codegen" -Command {
            if ($ForceCodegen) {
                & $rexglue -f codegen $manifest
            }
            else {
                & $rexglue codegen $manifest
            }
        }

        $manifestStubsAfter = Get-StubCount $manifest
        $addedToManifest = $manifestStubsAfter - $manifestStubsBefore
        Write-Log "Manifest now has $manifestStubsAfter stub(s) ($addedToManifest newly merged this round)" $(if ($addedToManifest -gt 0) { "Green" } else { "Gray" })

        Invoke-ExternalCommand -Label "Build doax" -LogName "build" -Command {
            cmake --build --preset $Preset --target doax
        }

        Copy-RuntimeDll
        $manifestStubs = $manifestStubsAfter
    }
}
catch {
    $script:ExitCode = 1
    $script:ExitMessage = $_.Exception.Message
    Write-Host ""
    Write-Host "ERROR: $($script:ExitMessage)" -ForegroundColor Red
    Write-Host "Session log: $logFile" -ForegroundColor Yellow
    if ($MyInvocation.MyCommand.Path) {
        Write-Host "Tip: re-run from a terminal or use bootstrap-codegen-loop.cmd" -ForegroundColor Gray
    }
}
finally {
    if ($script:ExitCode -eq 0) {
        Write-Log $script:ExitMessage "Green"
    }
    Wait-ForUser
    if ($script:ExitCode -ne 0) {
        exit $script:ExitCode
    }
}
