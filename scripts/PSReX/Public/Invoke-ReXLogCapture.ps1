<#
.SYNOPSIS
    Interactive FM2 log capture with keyboard start/stop triggers.
.DESCRIPTION
    Tails a live log file and writes a short snippet file only while capture is
    enabled. Use keyboard controls in the same console:
      - R: start/stop recording
      - M: write a marker line
      - Q: quit
.NOTES
    Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
    All rights reserved.
    Licensed under the BSD 3-Clause License.
    See LICENSE file in the project root for full license text.
#>
function Invoke-ReXLogCapture {
    [CmdletBinding()]
    param(
        [string]$LogPath = "C:\\temp\\fm2-clean.log",
        [string]$OutputPath,
        [string[]]$IncludePattern = @(
            "CP stats frame=",
            "FM2_PROD_PERSEC",
            "FM2_637F8_CALLER_PERSEC",
            "FM2_63768_LR_PERSEC",
            "ResolvePath\\(",
            "WAIT_REG_MEM"
        ),
        [switch]$NoFilter,
        [ValidateRange(5, 1000)]
        [int]$PollMs = 25
    )

    if (-not $OutputPath) {
        $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
        $OutputPath = "C:\\temp\\fm2-snippet-$stamp.log"
    }

    if ([Console]::IsInputRedirected) {
        throw "Invoke-ReXLogCapture requires an interactive console for keypress controls."
    }

    if (-not (Test-Path -LiteralPath $LogPath)) {
        New-Item -Path $LogPath -ItemType File -Force | Out-Null
    }

    $outputDir = Split-Path -Parent $OutputPath
    if ($outputDir -and -not (Test-Path -LiteralPath $outputDir)) {
        New-Item -Path $outputDir -ItemType Directory -Force | Out-Null
    }

    "[CAPTURE] Created: $OutputPath" | Set-Content -LiteralPath $OutputPath -Encoding utf8
    "[CAPTURE] Controls: R=start/stop, M=marker, Q=quit" | Add-Content -LiteralPath $OutputPath -Encoding utf8

    $stream = $null
    $reader = $null
    $recording = $false
    $shouldQuit = $false

    try {
        $stream = [System.IO.FileStream]::new(
            $LogPath,
            [System.IO.FileMode]::Open,
            [System.IO.FileAccess]::Read,
            [System.IO.FileShare]::ReadWrite
        )
        $reader = [System.IO.StreamReader]::new($stream)
        $reader.BaseStream.Seek(0, [System.IO.SeekOrigin]::End) | Out-Null
        $reader.DiscardBufferedData()

        Write-Host "Watching $LogPath"
        Write-Host "Output   $OutputPath"
        Write-Host "Press R to start/stop capture, M for marker, Q to quit."

        while (-not $shouldQuit) {
            while ([Console]::KeyAvailable) {
                $key = [Console]::ReadKey($true)
                $ch = [char]::ToUpperInvariant($key.KeyChar)

                switch ($ch) {
                    'R' {
                        $recording = -not $recording
                        $state = if ($recording) { "START" } else { "STOP" }
                        $line = "[CAPTURE] $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss.fff') $state"
                        Add-Content -LiteralPath $OutputPath -Value $line -Encoding utf8
                        Write-Host $line
                    }
                    'M' {
                        $line = "[CAPTURE] $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss.fff') MARK"
                        Add-Content -LiteralPath $OutputPath -Value $line -Encoding utf8
                        Write-Host $line
                    }
                    'Q' {
                        if ($recording) {
                            $recording = $false
                            $line = "[CAPTURE] $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss.fff') STOP"
                            Add-Content -LiteralPath $OutputPath -Value $line -Encoding utf8
                            Write-Host $line
                        }
                        $shouldQuit = $true
                    }
                }
            }

            while (($line = $reader.ReadLine()) -ne $null) {
                if (-not $recording) {
                    continue
                }

                $match = $NoFilter
                if (-not $match) {
                    foreach ($pattern in $IncludePattern) {
                        if ($line -match $pattern) {
                            $match = $true
                            break
                        }
                    }
                }

                if ($match) {
                    Add-Content -LiteralPath $OutputPath -Value $line -Encoding utf8
                }
            }

            Start-Sleep -Milliseconds $PollMs
        }
    } finally {
        if ($reader) { $reader.Dispose() }
        if ($stream) { $stream.Dispose() }
    }
}
