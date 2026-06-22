<#
.SYNOPSIS
    Launches FM2, drives the intro with gamepad A input, then stops FM2.
.DESCRIPTION
    This smoke harness is for repeatable FM2 Plume trace runs. It starts the
    FM2 executable with automation gamepad input enabled, waits for startup,
    sends A-button taps for a fixed duration, waits briefly in gameplay, then
    kills the FM2 process.
#>
[CmdletBinding()]
param(
    [string]$Fm2Exe,
    [string]$WorkingDirectory,
    [string]$LogPath = "C:\temp\fm2-clean.log",
    [bool]$ResetLog = $true,
    [ValidateRange(0, 300)]
    [int]$StartupDelaySeconds = 10,
    [ValidateRange(1, 300)]
    [int]$InputDurationSeconds = 20,
    [ValidateRange(1, 20)]
    [int]$PressesPerSecond = 2,
    [ValidateRange(1, 1000)]
    [int]$KeyHoldMilliseconds = 120,
    [string]$AutomationGamepadPath = (Join-Path $env:TEMP "rex-fm2-automation-gamepad-state.txt"),
    [bool]$VerifyInput = $true,
    [string]$ReplayScreenshotPath,
    [bool]$VerifyReplayScreenshotNonDark = $false,
    [string]$ReplayScreenshotWindowTitlePart = "FM2 Plume Debug Replay",
    [ValidateRange(1, 1000000)]
    [int]$ReplayScreenshotMinNonDarkPixels = 1024,
    [ValidateRange(0, 765)]
    [int]$ReplayScreenshotDarkLuminanceThreshold = 48,
    [ValidateRange(0, 300)]
    [int]$PostInputDelaySeconds = 10,
    [string[]]$Fm2Args = @(
        "--fm2_plume_mode", "shadow",
        "--fm2_plume_trace_packets",
        "--fm2_plume_trace_log_interval", "120",
        "--fm2_plume_trace_direct_decode",
        "--fm2_plume_trace_direct_decode_limit", "8",
        "--fm2_plume_trace_direct_decode_record_limit", "31",
        "--fm2_plume_trace_direct_buffer_bytes", "64",
        "--fm2_plume_trace_direct_state_bytes", "64",
        "--fm2_plume_trace_direct_shader_bytes", "1024"
    )
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Add-ReplayWindowCaptureType {
    if ("ReXGlue.FM2Smoke.ReplayWindowCaptureV1" -as [type]) {
        return
    }

    Add-Type -AssemblyName System.Drawing
    Add-Type -ReferencedAssemblies @("System.Drawing.dll") -TypeDefinition @"
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

namespace ReXGlue.FM2Smoke {
    public sealed class ReplayWindowCaptureResult {
        public bool Ok;
        public string Status;
        public int Width;
        public int Height;
        public int SampledPixels;
        public int NonDarkPixels;
        public double AverageLuminance;
    }

    public static class ReplayWindowCaptureV1 {
        private delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

        [StructLayout(LayoutKind.Sequential)]
        private struct RECT {
            public int left;
            public int top;
            public int right;
            public int bottom;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct POINT {
            public int x;
            public int y;
        }

        [DllImport("user32.dll")]
        private static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);

        [DllImport("user32.dll")]
        private static extern bool IsWindowVisible(IntPtr hWnd);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern int GetWindowTextW(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern int GetWindowTextLengthW(IntPtr hWnd);

        [DllImport("user32.dll")]
        private static extern bool GetClientRect(IntPtr hWnd, out RECT lpRect);

        [DllImport("user32.dll")]
        private static extern bool ClientToScreen(IntPtr hWnd, ref POINT lpPoint);

        [DllImport("user32.dll")]
        private static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

        [DllImport("user32.dll")]
        private static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter,
            int x, int y, int cx, int cy, uint uFlags);

        private static readonly IntPtr HWND_TOP = IntPtr.Zero;
        private const int SW_RESTORE = 9;
        private const uint SWP_NOACTIVATE = 0x0010;
        private const uint SWP_SHOWWINDOW = 0x0040;

        public static IntPtr FindVisibleWindowByTitlePart(string titlePart) {
            IntPtr found = IntPtr.Zero;
            EnumWindows(delegate(IntPtr windowHandle, IntPtr lParam) {
                if (!IsWindowVisible(windowHandle)) {
                    return true;
                }

                int length = GetWindowTextLengthW(windowHandle);
                if (length <= 0) {
                    return true;
                }

                StringBuilder title = new StringBuilder(length + 1);
                GetWindowTextW(windowHandle, title, title.Capacity);
                if (title.ToString().IndexOf(titlePart, StringComparison.OrdinalIgnoreCase) >= 0) {
                    found = windowHandle;
                    return false;
                }
                return true;
            }, IntPtr.Zero);
            return found;
        }

        public static ReplayWindowCaptureResult CaptureClientToPng(
            IntPtr windowHandle, string path, int darkLuminanceThreshold) {
            ReplayWindowCaptureResult result = new ReplayWindowCaptureResult();
            if (windowHandle == IntPtr.Zero) {
                result.Status = "missing-window";
                return result;
            }

            RECT rect;
            if (!GetClientRect(windowHandle, out rect)) {
                result.Status = "client-rect-failed";
                return result;
            }

            int width = rect.right - rect.left;
            int height = rect.bottom - rect.top;
            result.Width = width;
            result.Height = height;
            if (width <= 0 || height <= 0) {
                result.Status = "empty-client";
                return result;
            }

            ShowWindow(windowHandle, SW_RESTORE);
            SetWindowPos(windowHandle, HWND_TOP, 50, 50, width, height,
                         SWP_SHOWWINDOW | SWP_NOACTIVATE);
            Thread.Sleep(750);

            POINT origin = new POINT { x = 0, y = 0 };
            ClientToScreen(windowHandle, ref origin);

            using (Bitmap bitmap = new Bitmap(width, height, PixelFormat.Format32bppArgb)) {
                using (Graphics graphics = Graphics.FromImage(bitmap)) {
                    graphics.CopyFromScreen(origin.x, origin.y, 0, 0, new Size(width, height));
                }

                long luminanceSum = 0;
                int nonDarkPixels = 0;
                int sampledPixels = 0;
                int stepY = Math.Max(1, height / 180);
                int stepX = Math.Max(1, width / 320);
                for (int y = 0; y < height; y += stepY) {
                    for (int x = 0; x < width; x += stepX) {
                        Color color = bitmap.GetPixel(x, y);
                        int luminance = color.R + color.G + color.B;
                        luminanceSum += luminance;
                        ++sampledPixels;
                        if (luminance > darkLuminanceThreshold) {
                            ++nonDarkPixels;
                        }
                    }
                }

                string directory = Path.GetDirectoryName(path);
                if (!String.IsNullOrEmpty(directory)) {
                    Directory.CreateDirectory(directory);
                }
                bitmap.Save(path, ImageFormat.Png);

                result.Ok = true;
                result.Status = "ok";
                result.SampledPixels = sampledPixels;
                result.NonDarkPixels = nonDarkPixels;
                result.AverageLuminance = sampledPixels != 0
                    ? (double)luminanceSum / sampledPixels
                    : 0.0;
                return result;
            }
        }
    }
}
"@
}

function Write-AutomationGamepadState {
    param(
        [Parameter(Mandatory)]
        [string]$Path,
        [Parameter(Mandatory)]
        [int]$Packet,
        [Parameter(Mandatory)]
        [int]$Buttons,
        [ValidateRange(0, 255)]
        [int]$LeftTrigger = 0,
        [ValidateRange(0, 255)]
        [int]$RightTrigger = 0,
        [ValidateRange(-32768, 32767)]
        [int]$ThumbLX = 0,
        [ValidateRange(-32768, 32767)]
        [int]$ThumbLY = 0,
        [ValidateRange(-32768, 32767)]
        [int]$ThumbRX = 0,
        [ValidateRange(-32768, 32767)]
        [int]$ThumbRY = 0
    )

    $directory = Split-Path -Parent $Path
    if ($directory -and -not (Test-Path -LiteralPath $directory)) {
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
    }

    $content = @(
        "packet=$Packet",
        ("buttons=0x{0:X4}" -f $Buttons),
        "left_trigger=$LeftTrigger",
        "right_trigger=$RightTrigger",
        "thumb_lx=$ThumbLX",
        "thumb_ly=$ThumbLY",
        "thumb_rx=$ThumbRX",
        "thumb_ry=$ThumbRY"
    ) -join "`n"

    $tempPath = "$Path.tmp"
    Set-Content -LiteralPath $tempPath -Value ($content + "`n") -Encoding ascii
    for ($attempt = 1; $attempt -le 20; ++$attempt) {
        try {
            Move-Item -LiteralPath $tempPath -Destination $Path -Force -ErrorAction Stop
            return
        } catch {
            if ($attempt -eq 20) {
                try {
                    Set-Content -LiteralPath $Path -Value ($content + "`n") -Encoding ascii -ErrorAction Stop
                    Remove-Item -LiteralPath $tempPath -Force -ErrorAction SilentlyContinue
                    return
                } catch {
                    throw
                }
            }
            Start-Sleep -Milliseconds 10
        }
    }
}

function Invoke-AutomationGamepadATap {
    param(
        [Parameter(Mandatory)]
        [string]$Path,
        [Parameter(Mandatory)]
        [int]$Packet,
        [Parameter(Mandatory)]
        [int]$HoldMilliseconds
    )

    $xInputGamepadA = 0x1000
    Write-AutomationGamepadState -Path $Path -Packet $Packet -Buttons $xInputGamepadA
    Start-Sleep -Milliseconds ([Math]::Max(1, $HoldMilliseconds))

    $releasePacket = $Packet + 1
    Write-AutomationGamepadState -Path $Path -Packet $releasePacket -Buttons 0
    return ($releasePacket + 1)
}

function Invoke-ReplayScreenshotCapture {
    param(
        [Parameter(Mandatory)]
        [string]$Path,
        [Parameter(Mandatory)]
        [string]$WindowTitlePart,
        [Parameter(Mandatory)]
        [int]$DarkLuminanceThreshold,
        [Parameter(Mandatory)]
        [int]$MinimumNonDarkPixels,
        [Parameter(Mandatory)]
        [bool]$VerifyNonDark
    )

    Add-ReplayWindowCaptureType
    $windowHandle = [ReXGlue.FM2Smoke.ReplayWindowCaptureV1]::FindVisibleWindowByTitlePart(
        $WindowTitlePart)
    $capture = [ReXGlue.FM2Smoke.ReplayWindowCaptureV1]::CaptureClientToPng(
        $windowHandle, $Path, $DarkLuminanceThreshold)

    Write-Host "Replay screenshot: status=$($capture.Status) hwnd=0x$($windowHandle.ToString('X')) path=$Path width=$($capture.Width) height=$($capture.Height) sampled=$($capture.SampledPixels) nonDark=$($capture.NonDarkPixels) avgLum=$([Math]::Round($capture.AverageLuminance, 2))"
    if ($VerifyNonDark) {
        if (-not $capture.Ok) {
            throw "Replay screenshot capture failed: $($capture.Status)"
        }
        if ($capture.NonDarkPixels -lt $MinimumNonDarkPixels) {
            throw "Replay screenshot did not contain enough non-dark sampled pixels. Expected at least $MinimumNonDarkPixels, got $($capture.NonDarkPixels)."
        }
    }
}

function Find-RecentFM2AppLogs {
    param(
        [Parameter(Mandatory)]
        [string]$Fm2Executable,
        [Parameter(Mandatory)]
        [datetime]$Since
    )

    $exeDir = Split-Path -Parent $Fm2Executable
    $logDir = Join-Path $exeDir "logs"
    if (-not (Test-Path -LiteralPath $logDir)) {
        return @()
    }

    @(Get-ChildItem -LiteralPath $logDir -Filter "fm2*.log" -File |
        Where-Object { $_.LastWriteTime -ge $Since.AddSeconds(-5) } |
        Sort-Object LastWriteTime -Descending)
}

$scriptRoot = Split-Path -Parent $PSCommandPath
$repoRoot = Resolve-Path -LiteralPath (Join-Path $scriptRoot "..\..")

if (-not $Fm2Exe) {
    $Fm2Exe = Join-Path $repoRoot "FM2\out\build\win-amd64-relwithdebinfo\fm2.exe"
}
if (-not $WorkingDirectory) {
    $WorkingDirectory = Join-Path $repoRoot "FM2"
}

$Fm2Exe = [System.IO.Path]::GetFullPath($Fm2Exe)
$WorkingDirectory = [System.IO.Path]::GetFullPath($WorkingDirectory)
$AutomationGamepadPath = [System.IO.Path]::GetFullPath($AutomationGamepadPath)

if (-not (Test-Path -LiteralPath $Fm2Exe)) {
    throw "FM2 executable not found: $Fm2Exe"
}
if (-not (Test-Path -LiteralPath $WorkingDirectory)) {
    throw "Working directory not found: $WorkingDirectory"
}

if ($ResetLog) {
    $logDir = Split-Path -Parent $LogPath
    if ($logDir -and -not (Test-Path -LiteralPath $logDir)) {
        New-Item -ItemType Directory -Path $logDir -Force | Out-Null
    }
    "" | Set-Content -LiteralPath $LogPath -Encoding utf8
}

Write-Host "Input: automation gamepad A"
Write-Host "Automation gamepad file: $AutomationGamepadPath"
Write-Host "Verify input: $VerifyInput"

$process = $null
$runStartTime = Get-Date
try {
    $effectiveFm2Args = @($Fm2Args)

    Write-Host "Launching: $Fm2Exe"
    Write-Host "Working directory: $WorkingDirectory"
    Write-Host "Arguments: $($effectiveFm2Args -join ' ')"

    $previousAutomationGamepadFile = $env:REX_AUTOMATION_GAMEPAD_FILE
    Write-AutomationGamepadState -Path $AutomationGamepadPath -Packet 0 -Buttons 0
    $env:REX_AUTOMATION_GAMEPAD_FILE = $AutomationGamepadPath
    Write-Host "Child environment: REX_AUTOMATION_GAMEPAD_FILE=$AutomationGamepadPath"

    try {
        $process = Start-Process -FilePath $Fm2Exe -WorkingDirectory $WorkingDirectory `
            -ArgumentList $effectiveFm2Args -PassThru
    } finally {
        if ($null -eq $previousAutomationGamepadFile) {
            Remove-Item Env:\REX_AUTOMATION_GAMEPAD_FILE -ErrorAction SilentlyContinue
        } else {
            $env:REX_AUTOMATION_GAMEPAD_FILE = $previousAutomationGamepadFile
        }
    }
    Write-Host "Started fm2.exe pid=$($process.Id)"

    Write-Host "Waiting $StartupDelaySeconds seconds for startup..."
    Start-Sleep -Seconds $StartupDelaySeconds

    $tapCount = $InputDurationSeconds * $PressesPerSecond
    $tapIntervalMs = [Math]::Max(1, [int](1000 / $PressesPerSecond))
    $tapLabel = "Gamepad A"
    Write-Host "Sending $tapLabel $tapCount times over $InputDurationSeconds seconds, hold=${KeyHoldMilliseconds}ms..."

    $tapOkCount = 0
    $automationPacket = 1
    for ($tap = 1; $tap -le $tapCount; ++$tap) {
        $tapStart = [System.Diagnostics.Stopwatch]::StartNew()
        $process.Refresh()
        if ($process.HasExited) {
            Write-Warning "FM2 exited during input drive. Exit code: $($process.ExitCode)"
            break
        }

        $automationPacket = Invoke-AutomationGamepadATap -Path $AutomationGamepadPath `
            -Packet $automationPacket -HoldMilliseconds $KeyHoldMilliseconds
        ++$tapOkCount

        $tapStart.Stop()
        $remainingMs = $tapIntervalMs - [int]$tapStart.ElapsedMilliseconds
        if ($remainingMs -gt 0) {
            Start-Sleep -Milliseconds $remainingMs
        }
    }
    Write-Host "$tapLabel tap result: ok=$tapOkCount"

    Write-Host "Waiting $PostInputDelaySeconds seconds after input..."
    Start-Sleep -Seconds $PostInputDelaySeconds

    if ($ReplayScreenshotPath) {
        Invoke-ReplayScreenshotCapture -Path $ReplayScreenshotPath `
            -WindowTitlePart $ReplayScreenshotWindowTitlePart `
            -DarkLuminanceThreshold $ReplayScreenshotDarkLuminanceThreshold `
            -MinimumNonDarkPixels $ReplayScreenshotMinNonDarkPixels `
            -VerifyNonDark $VerifyReplayScreenshotNonDark
    }
} finally {
    Write-AutomationGamepadState -Path $AutomationGamepadPath -Packet 0 -Buttons 0
    if ($process) {
        $process.Refresh()
        if (-not $process.HasExited) {
            Write-Host "Stopping fm2.exe pid=$($process.Id)"
            Stop-Process -Id $process.Id -Force
            $process.WaitForExit(5000) | Out-Null
        }
    }
}

$recentAppLogs = @(Find-RecentFM2AppLogs -Fm2Executable $Fm2Exe -Since $runStartTime)
if ($recentAppLogs.Count -gt 0) {
    Write-Host "Recent FM2 app logs:"
    foreach ($appLog in $recentAppLogs) {
        Write-Host "  $($appLog.FullName) length=$($appLog.Length) lastWrite=$($appLog.LastWriteTime)"
    }
}

if ($VerifyInput) {
    Write-Host "Input receiver trace: gamepad automation path used; final state file=$AutomationGamepadPath"
}

if (Test-Path -LiteralPath $LogPath) {
    $logItem = Get-Item -LiteralPath $LogPath
    Write-Host "Log: $($logItem.FullName) length=$($logItem.Length) lastWrite=$($logItem.LastWriteTime)"
}
