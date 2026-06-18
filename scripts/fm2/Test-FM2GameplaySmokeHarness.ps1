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

Assert-Match -Text $harness `
    -Pattern '\[ValidateSet\("auto",\s*"sendinput",\s*"postmessage",\s*"both"\)\]\s*\r?\n\s*\[string\]\$InputMethod\s*=\s*"postmessage"' `
    -Message "Invoke-FM2GameplaySmoke.ps1 must default to targeted FM2 window messages."

Assert-Match -Text $harness `
    -Pattern 'SendMessageTimeout' `
    -Message "Harness targeted window input must synchronously reach FM2's WndProc."

Assert-Match -Text $harness `
    -Pattern 'MNK_INPUT_KEY event=down vk=32' `
    -Message "Harness must verify Space down events from FM2's MnK trace."

Assert-Match -Text $harness `
    -Pattern 'MNK_INPUT_KEY event=up vk=32' `
    -Message "Harness must verify Space up events from FM2's MnK trace."

Assert-Match -Text $harness `
    -Pattern 'MNK_INPUT_STATE \.\*a=1' `
    -Message "Harness must verify synthesized A-button state from FM2's MnK trace."

Write-Host "FM2 gameplay smoke harness checks passed."
