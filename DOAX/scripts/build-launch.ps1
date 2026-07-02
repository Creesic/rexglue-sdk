# Build doax from the CLI (no Visual Studio) and optionally launch it.
param(
    [string]$Preset = "win-amd64-relwithdebinfo",
    [switch]$Codegen,
    [switch]$Launch,
    [switch]$NoBuild,
    [string[]]$GameArgs = @("--game_data_root")
)

$ErrorActionPreference = "Stop"
$doaxRoot = Split-Path $PSScriptRoot -Parent
$sdkRoot = Split-Path $doaxRoot -Parent
$buildDir = Join-Path $doaxRoot "out\build\$Preset"
$exe = Join-Path $buildDir "doax.exe"
$assets = (Resolve-Path -LiteralPath (Join-Path $doaxRoot "assets")).Path
$rexglue = Join-Path $sdkRoot "out\install\win-amd64\bin\rexglue.exe"

if ($Codegen) {
    if (-not (Test-Path $rexglue)) {
        throw "rexglue not found at $rexglue (build/install the SDK first)"
    }
    Write-Host "Running codegen..." -ForegroundColor Cyan
    Push-Location $doaxRoot
    try {
        & $rexglue codegen doax_manifest.toml
    } finally {
        Pop-Location
    }
}

if (-not $NoBuild) {
    # cmake --preset reads CMakePresets.json from the CURRENT directory, so this
    # must run from $doaxRoot (where DOAX/CMakePresets.json lives) regardless of
    # where the launcher/shortcut was started from.
    Push-Location $doaxRoot
    try {
        Write-Host "Configuring preset $Preset (in $doaxRoot)..." -ForegroundColor Cyan
        cmake --preset $Preset
        if ($LASTEXITCODE -ne 0) { throw "cmake --preset $Preset FAILED (exit $LASTEXITCODE)" }
        Write-Host "Building doax..." -ForegroundColor Cyan
        cmake --build $buildDir --target doax -j
        # cmake is a native exe: $ErrorActionPreference does NOT catch its exit code.
        # Without this check a failed build silently launches stale binaries.
        if ($LASTEXITCODE -ne 0) { throw "Build FAILED (exit $LASTEXITCODE) - refusing to launch stale binaries" }
    } finally {
        Pop-Location
    }
}

if (-not (Test-Path $exe)) {
    throw "doax.exe not found at $exe"
}

# --- Guarantee the runtime DLL next to doax.exe is the freshly built one ---------
# rexruntimerd.dll builds into the SDK output tree and must be colocated with
# doax.exe (the game's working dir). A missed copy, a failed build, or editing a
# source after the build silently leaves STALE code running. Verify by hash and
# reconcile; abort rather than launch stale code.
$gameDll = Join-Path $buildDir "rexruntimerd.dll"
$dllRoots = @((Join-Path $sdkRoot "out"), $buildDir) | Where-Object { Test-Path $_ }
$builtDlls = @()
foreach ($root in $dllRoots) {
    $builtDlls += Get-ChildItem -Path $root -Recurse -Filter "rexruntimerd.dll" -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -ne $gameDll -and $_.FullName -notmatch '[\\/]install[\\/]' }
}
$fresh = $builtDlls | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if ($fresh) {
    $freshHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $fresh.FullName).Hash
    if (Test-Path $gameDll) { $gameHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $gameDll).Hash } else { $gameHash = "<missing>" }
    if ($freshHash -ne $gameHash) {
        Write-Host "Refreshing stale runtime DLL next to doax.exe:" -ForegroundColor Yellow
        Write-Host "  from $($fresh.FullName) [$($fresh.LastWriteTime)]" -ForegroundColor Yellow
        Copy-Item -LiteralPath $fresh.FullName -Destination $gameDll -Force
        $gameHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $gameDll).Hash
    }
    if ($freshHash -ne $gameHash) {
        throw "Could not colocate fresh rexruntimerd.dll next to doax.exe (hash mismatch persists)"
    }
    Write-Host "Runtime DLL verified: $gameDll" -ForegroundColor Green
    Write-Host "  sha256 $($freshHash.Substring(0,16))...  built $($fresh.LastWriteTime)" -ForegroundColor Gray
} else {
    Write-Warning "No freshly built rexruntimerd.dll found to verify against $gameDll"
}

# Catch the classic trap: the freshest BUILT dll is older than the newest SDK
# source => the build did not include your latest edits (wrong tree / incremental
# miss / edited after build). Compare the build OUTPUT's time (not the just-copied
# game dll, whose mtime is now). Fatal on a build run; warning under -NoBuild.
$srcDirs = @((Join-Path $sdkRoot "src"), (Join-Path $sdkRoot "include")) | Where-Object { Test-Path $_ }
$newestSrc = Get-ChildItem -Path $srcDirs -Recurse -Include *.cpp,*.h,*.hlsl,*.hlsli,*.inc -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending | Select-Object -First 1
if ($newestSrc -and $fresh) {
    if ($fresh.LastWriteTime -lt $newestSrc.LastWriteTime) {
        $msg = "STALE BUILD: newest runtime DLL ($($fresh.LastWriteTime)) is older than source $($newestSrc.Name) ($($newestSrc.LastWriteTime)). Your latest edits are NOT compiled in."
        if ($NoBuild) { Write-Warning $msg } else { throw "$msg Rebuild (and check for compile errors) before launching." }
    }
}

Write-Host "Built: $exe ($(Get-Item $exe).LastWriteTime)" -ForegroundColor Green

if (-not $Launch) {
    Write-Host "Pass -Launch to start the game, or run:" -ForegroundColor Gray
    Write-Host "  Set-Location '$buildDir'; .\doax.exe --game_data_root '$assets'" -ForegroundColor Gray
    exit 0
}

$launchArgs = @()
if ($GameArgs.Count -eq 1 -and $GameArgs[0] -eq "--game_data_root") {
    $launchArgs = @("--game_data_root", $assets)
} else {
    $launchArgs = $GameArgs
}

Write-Host "Launching doax from $buildDir" -ForegroundColor Cyan
Write-Host "  args: $($launchArgs -join ' ')" -ForegroundColor Gray
Push-Location $buildDir
try {
    & $exe @launchArgs
} finally {
    Pop-Location
}
