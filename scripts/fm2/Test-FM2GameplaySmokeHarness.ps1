<#
.SYNOPSIS
    Static regression checks for Invoke-FM2GameplaySmoke.ps1.
#>
[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$scriptRoot = Split-Path -Parent $PSCommandPath
$harnessPath = Join-Path $scriptRoot "Invoke-FM2GameplaySmoke.ps1"
$harness = Get-Content -LiteralPath $harnessPath -Raw

function Assert-Match {
    param(
        [Parameter(Mandatory)]
        [string]$Text,
        [Parameter(Mandatory)]
        [string]$Pattern,
        [Parameter(Mandatory)]
        [string]$Message
    )

    if ($Text -notmatch $Pattern) {
        throw $Message
    }
}

function Assert-NotMatch {
    param(
        [Parameter(Mandatory)]
        [string]$Text,
        [Parameter(Mandatory)]
        [string]$Pattern,
        [Parameter(Mandatory)]
        [string]$Message
    )

    if ($Text -match $Pattern) {
        throw $Message
    }
}

Assert-Match -Text $harness `
    -Pattern 'Invoke-AutomationGamepadATap' `
    -Message "Invoke-FM2GameplaySmoke.ps1 must drive gamepad A taps."

Assert-Match -Text $harness `
    -Pattern 'REX_AUTOMATION_GAMEPAD_FILE' `
    -Message "Harness must pass the automation gamepad state file to FM2."

Assert-Match -Text $harness `
    -Pattern 'buttons=0x\{0:X4\}' `
    -Message "Harness must write XInput button masks to the automation state file."

Assert-Match -Text $harness `
    -Pattern '\$xInputGamepadA\s*=\s*0x1000' `
    -Message "Harness must drive the Xbox 360 A button mask."

Assert-Match -Text $harness `
    -Pattern 'Write-AutomationGamepadState[\s\S]+for\s*\(\s*\$attempt' `
    -Message "Harness must retry automation gamepad state writes while FM2 polls the file."

Assert-NotMatch -Text $harness `
    -Pattern 'TapSpace|VK_SPACE|SendInput|PostMessage|SendMessageTimeout' `
    -Message "Harness must not keep the old Space/keyboard injection path."

Assert-NotMatch -Text $harness `
    -Pattern 'REX_MNK_MODE|REX_MNK_TRACE_INPUT|--mnk_mode|MNK_INPUT_KEY' `
    -Message "Harness must not depend on MnK mode to press A."

Write-Host "FM2 gameplay smoke harness checks passed."
