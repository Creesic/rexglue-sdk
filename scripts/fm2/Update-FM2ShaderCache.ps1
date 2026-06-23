param(
    [string]$ReOdysseyRoot = "",
    [string]$BuildDir = "",
    [string]$ClangCl = "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin\clang-cl.exe",
    [int]$Jobs = 8,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
if ([string]::IsNullOrWhiteSpace($ReOdysseyRoot)) {
    $ReOdysseyRoot = Join-Path (Split-Path -Parent $repoRoot) "ReOdyssey"
}
if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path $repoRoot ".cache\xenosrecomp-clangcl-build"
}

$xenosSource = Join-Path $ReOdysseyRoot "thirdparty\XenosRecomp"
$shaderCommon = Join-Path $xenosSource "XenosRecomp\shader_common.h"
$assetDir = Join-Path $repoRoot "FM2\assets"
$generatedDir = Join-Path $repoRoot "FM2\generated"
$candidate = Join-Path $repoRoot ".cache\fm2_shader_cache_candidate.cpp"
$output = Join-Path $generatedDir "shader_cache.cpp"
$xenosExe = Join-Path $BuildDir "XenosRecomp\XenosRecomp.exe"

if (-not (Test-Path -LiteralPath $xenosSource)) {
    throw "XenosRecomp source not found: $xenosSource"
}
if (-not (Test-Path -LiteralPath $shaderCommon)) {
    throw "XenosRecomp shader_common.h not found: $shaderCommon"
}
if (-not (Test-Path -LiteralPath $assetDir)) {
    throw "FM2 assets directory not found: $assetDir"
}
if (-not (Test-Path -LiteralPath $ClangCl)) {
    throw "clang-cl not found: $ClangCl"
}

New-Item -ItemType Directory -Force -Path (Split-Path -Parent $candidate) | Out-Null
New-Item -ItemType Directory -Force -Path $generatedDir | Out-Null

if (-not $SkipBuild) {
    & cmake -S $xenosSource `
        -B $BuildDir `
        -G Ninja `
        -DCMAKE_BUILD_TYPE=RelWithDebInfo `
        -DCMAKE_OSX_ARCHITECTURES=AMD64 `
        "-DCMAKE_C_COMPILER=$ClangCl" `
        "-DCMAKE_CXX_COMPILER=$ClangCl"
    if ($LASTEXITCODE -ne 0) {
        throw "XenosRecomp configure failed with exit code $LASTEXITCODE"
    }

    & cmake --build $BuildDir --target XenosRecomp --config RelWithDebInfo
    if ($LASTEXITCODE -ne 0) {
        throw "XenosRecomp build failed with exit code $LASTEXITCODE"
    }
}

if (-not (Test-Path -LiteralPath $xenosExe)) {
    throw "XenosRecomp executable not found: $xenosExe"
}

Push-Location $repoRoot
try {
    & $xenosExe "FM2\assets" $candidate $shaderCommon -j $Jobs
    if ($LASTEXITCODE -ne 0) {
        throw "XenosRecomp shader-cache generation failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}

$cacheText = Get-Content -LiteralPath $candidate -Raw
$cacheText = $cacheText -replace '("[^"]*") \},', '$1, nullptr },'
Set-Content -LiteralPath $candidate -Value $cacheText -NoNewline -Encoding utf8

Copy-Item -LiteralPath $candidate -Destination $output -Force
(Get-Item -LiteralPath $output).LastWriteTime = Get-Date

$entryLine = Select-String -Path $output -Pattern "g_shaderCacheEntryCount\s*=\s*(\d+)" |
    Select-Object -First 1
if ($entryLine -and $entryLine.Matches.Count -gt 0) {
    $entryCount = $entryLine.Matches[0].Groups[1].Value
    Write-Host "Updated FM2 shader cache: $output ($entryCount entries)"
} else {
    Write-Host "Updated FM2 shader cache: $output"
}
