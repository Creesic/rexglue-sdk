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
    Write-Host "Configuring preset $Preset..." -ForegroundColor Cyan
    cmake --preset $Preset
    Write-Host "Building doax..." -ForegroundColor Cyan
    cmake --build $buildDir --target doax -j
}

if (-not (Test-Path $exe)) {
    throw "doax.exe not found at $exe"
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
