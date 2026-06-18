<#
.SYNOPSIS
    Launches FM2, drives the intro with MnK Space input, then stops FM2.
.DESCRIPTION
    This smoke harness is for repeatable FM2 Plume trace runs. It starts the
    FM2 executable with MnK mode enabled, waits for startup, sends Space key
    taps to the FM2 window for a fixed duration, waits briefly in gameplay,
    then kills the FM2 process.
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
    [ValidateSet("auto", "sendinput", "postmessage", "both")]
    [string]$InputMethod = "postmessage",
    [bool]$TraceInput = $true,
    [ValidateRange(1, 100000)]
    [int]$TraceInputLimit = 512,
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
        "--fm2_plume_trace_direct_shader_bytes", "1024",
        "--mnk_mode"
    )
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Add-NativeInputType {
    if ("ReXGlue.FM2Smoke.NativeInputV6" -as [type]) {
        return
    }

    Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

namespace ReXGlue.FM2Smoke {
    public static class NativeInputV6 {
        private const int INPUT_KEYBOARD = 1;
        private const ushort VK_SPACE = 0x20;
        private const uint KEYEVENTF_KEYUP = 0x0002;
        private const uint KEYEVENTF_SCANCODE = 0x0008;
        private const uint MAPVK_VK_TO_VSC = 0;
        private const int SW_RESTORE = 9;
        private const uint WM_SETFOCUS = 0x0007;
        private const uint WM_ACTIVATE = 0x0006;
        private const uint WM_ACTIVATEAPP = 0x001C;
        private const uint WM_KEYDOWN = 0x0100;
        private const uint WM_KEYUP = 0x0101;
        private const uint SMTO_ABORTIFHUNG = 0x0002;

        [StructLayout(LayoutKind.Sequential)]
        private struct INPUT {
            public int type;
            public INPUTUNION u;
        }

        [StructLayout(LayoutKind.Explicit)]
        private struct INPUTUNION {
            [FieldOffset(0)]
            public MOUSEINPUT mi;
            [FieldOffset(0)]
            public KEYBDINPUT ki;
            [FieldOffset(0)]
            public HARDWAREINPUT hi;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct MOUSEINPUT {
            public int dx;
            public int dy;
            public uint mouseData;
            public uint dwFlags;
            public uint time;
            public UIntPtr dwExtraInfo;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct KEYBDINPUT {
            public ushort wVk;
            public ushort wScan;
            public uint dwFlags;
            public uint time;
            public UIntPtr dwExtraInfo;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct HARDWAREINPUT {
            public uint uMsg;
            public ushort wParamL;
            public ushort wParamH;
        }

        [DllImport("user32.dll")]
        private static extern bool SetForegroundWindow(IntPtr hWnd);

        [DllImport("user32.dll")]
        private static extern IntPtr GetForegroundWindow();

        private delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

        [DllImport("user32.dll")]
        private static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);

        [DllImport("user32.dll")]
        private static extern bool IsWindowVisible(IntPtr hWnd);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern int GetWindowTextW(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern int GetWindowTextLengthW(IntPtr hWnd);

        [DllImport("user32.dll")]
        private static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

        [DllImport("user32.dll")]
        private static extern bool BringWindowToTop(IntPtr hWnd);

        [DllImport("user32.dll")]
        private static extern IntPtr SetActiveWindow(IntPtr hWnd);

        [DllImport("user32.dll")]
        private static extern IntPtr SetFocus(IntPtr hWnd);

        [DllImport("user32.dll", SetLastError = true)]
        private static extern bool AttachThreadInput(uint idAttach, uint idAttachTo, bool fAttach);

        [DllImport("user32.dll")]
        private static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

        [DllImport("kernel32.dll")]
        private static extern uint GetCurrentThreadId();

        [DllImport("user32.dll", SetLastError = true)]
        private static extern uint SendInput(uint nInputs, INPUT[] pInputs, int cbSize);

        [DllImport("user32.dll", SetLastError = true)]
        private static extern bool PostMessage(IntPtr hWnd, uint msg, UIntPtr wParam, IntPtr lParam);

        [DllImport("user32.dll", SetLastError = true)]
        private static extern IntPtr SendMessageTimeout(IntPtr hWnd, uint msg, UIntPtr wParam,
            IntPtr lParam, uint flags, uint timeoutMilliseconds, out IntPtr result);

        [DllImport("user32.dll")]
        private static extern uint MapVirtualKey(uint uCode, uint uMapType);

        public static int LastError { get; private set; }

        public static int InputSize() {
            return Marshal.SizeOf(typeof(INPUT));
        }

        public static IntPtr ForegroundWindow() {
            return GetForegroundWindow();
        }

        public static bool IsForeground(IntPtr windowHandle) {
            return windowHandle != IntPtr.Zero && GetForegroundWindow() == windowHandle;
        }

        public static string GetWindowTitle(IntPtr windowHandle) {
            if (windowHandle == IntPtr.Zero) {
                return String.Empty;
            }

            int length = GetWindowTextLengthW(windowHandle);
            if (length <= 0) {
                return String.Empty;
            }

            StringBuilder title = new StringBuilder(length + 1);
            GetWindowTextW(windowHandle, title, title.Capacity);
            return title.ToString();
        }

        public static IntPtr FindProcessMainWindow(int processId, string excludedTitlePart) {
            IntPtr best = IntPtr.Zero;
            IntPtr fallback = IntPtr.Zero;

            EnumWindows(delegate(IntPtr windowHandle, IntPtr lParam) {
                if (!IsWindowVisible(windowHandle)) {
                    return true;
                }

                uint windowProcessId;
                GetWindowThreadProcessId(windowHandle, out windowProcessId);
                if (windowProcessId != (uint)processId) {
                    return true;
                }

                string title = GetWindowTitle(windowHandle);
                if (fallback == IntPtr.Zero) {
                    fallback = windowHandle;
                }

                if (!String.IsNullOrEmpty(excludedTitlePart) &&
                    title.IndexOf(excludedTitlePart, StringComparison.OrdinalIgnoreCase) >= 0) {
                    return true;
                }

                best = windowHandle;
                return false;
            }, IntPtr.Zero);

            return best != IntPtr.Zero ? best : fallback;
        }

        public static bool EnsureForeground(IntPtr windowHandle) {
            if (windowHandle == IntPtr.Zero) {
                LastError = 0;
                return false;
            }

            ShowWindow(windowHandle, SW_RESTORE);

            uint unusedProcessId;
            uint currentThread = GetCurrentThreadId();
            uint targetThread = GetWindowThreadProcessId(windowHandle, out unusedProcessId);
            IntPtr foregroundWindow = GetForegroundWindow();
            uint foregroundThread = foregroundWindow != IntPtr.Zero
                ? GetWindowThreadProcessId(foregroundWindow, out unusedProcessId)
                : 0;

            bool attachedTarget = false;
            bool attachedForeground = false;
            int setForegroundError = 0;
            try {
                if (targetThread != 0 && currentThread != targetThread) {
                    attachedTarget = AttachThreadInput(currentThread, targetThread, true);
                }
                if (foregroundThread != 0 && currentThread != foregroundThread &&
                    foregroundThread != targetThread) {
                    attachedForeground = AttachThreadInput(currentThread, foregroundThread, true);
                }

                BringWindowToTop(windowHandle);
                SetActiveWindow(windowHandle);
                SetFocus(windowHandle);
                bool foregroundSet = SetForegroundWindow(windowHandle);
                if (!foregroundSet) {
                    setForegroundError = Marshal.GetLastWin32Error();
                }
            } finally {
                if (attachedForeground) {
                    AttachThreadInput(currentThread, foregroundThread, false);
                }
                if (attachedTarget) {
                    AttachThreadInput(currentThread, targetThread, false);
                }
            }

            Thread.Sleep(75);
            bool isForeground = IsForeground(windowHandle);
            LastError = isForeground ? 0 : setForegroundError;
            return isForeground;
        }

        private static bool SendKeyboard(ushort virtualKey, uint flags) {
            INPUT[] inputs = new INPUT[1];
            ushort scanCode = (ushort)MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC);
            inputs[0].type = INPUT_KEYBOARD;
            if (scanCode != 0) {
                inputs[0].u.ki.wVk = 0;
                inputs[0].u.ki.wScan = scanCode;
                inputs[0].u.ki.dwFlags = flags | KEYEVENTF_SCANCODE;
            } else {
                inputs[0].u.ki.wVk = virtualKey;
                inputs[0].u.ki.dwFlags = flags;
            }

            uint sent = SendInput(1, inputs, Marshal.SizeOf(typeof(INPUT)));
            if (sent != 1) {
                LastError = Marshal.GetLastWin32Error();
                return false;
            }

            LastError = 0;
            return true;
        }

        private static bool SendWindowKeyboard(IntPtr windowHandle, uint message, ushort virtualKey, bool wasDown) {
            if (windowHandle == IntPtr.Zero) {
                LastError = 0;
                return false;
            }

            uint scanCode = MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC);
            int lParam = 1 | ((int)scanCode << 16);
            if (wasDown) {
                lParam |= unchecked((int)0xC0000000);
            }

            return SendWindowMessage(windowHandle, message, new UIntPtr(virtualKey), new IntPtr(lParam));
        }

        private static bool SendWindowMessage(IntPtr windowHandle, uint message, UIntPtr wParam, IntPtr lParam) {
            if (windowHandle == IntPtr.Zero) {
                LastError = 0;
                return false;
            }

            IntPtr result;
            IntPtr sendResult = SendMessageTimeout(windowHandle, message, wParam, lParam,
                SMTO_ABORTIFHUNG, 1000, out result);
            if (sendResult == IntPtr.Zero) {
                LastError = Marshal.GetLastWin32Error();
                return false;
            }

            LastError = 0;
            return true;
        }

        private static bool PrimeMessageInput(IntPtr windowHandle) {
            if (windowHandle == IntPtr.Zero) {
                LastError = 0;
                return false;
            }

            ShowWindow(windowHandle, SW_RESTORE);
            BringWindowToTop(windowHandle);

            bool activateAppOk = SendWindowMessage(windowHandle, WM_ACTIVATEAPP, new UIntPtr(1), IntPtr.Zero);
            int activateAppError = LastError;
            bool activateOk = SendWindowMessage(windowHandle, WM_ACTIVATE, new UIntPtr(1), IntPtr.Zero);
            int activateError = LastError;
            bool focusOk = SendWindowMessage(windowHandle, WM_SETFOCUS, UIntPtr.Zero, IntPtr.Zero);
            int focusError = LastError;
            if (!activateAppOk || !activateOk || !focusOk) {
                LastError = !activateAppOk ? activateAppError : (!activateOk ? activateError : focusError);
                return false;
            }

            LastError = 0;
            return true;
        }

        public static bool TapSpace(IntPtr windowHandle, int holdMilliseconds) {
            if (!EnsureForeground(windowHandle)) {
                return false;
            }

            if (!SendKeyboard(VK_SPACE, 0)) {
                return false;
            }
            Thread.Sleep(Math.Max(1, holdMilliseconds));
            return SendKeyboard(VK_SPACE, KEYEVENTF_KEYUP);
        }

        public static bool TapSpaceMessage(IntPtr windowHandle, int holdMilliseconds) {
            if (!PrimeMessageInput(windowHandle)) {
                return false;
            }

            if (!SendWindowKeyboard(windowHandle, WM_KEYDOWN, VK_SPACE, false)) {
                return false;
            }
            Thread.Sleep(Math.Max(1, holdMilliseconds));
            return SendWindowKeyboard(windowHandle, WM_KEYUP, VK_SPACE, true);
        }
    }
}
"@
}

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

function Find-FM2WindowHandle {
    param(
        [Parameter(Mandatory)]
        [System.Diagnostics.Process]$Process
    )

    $windowHandle = [ReXGlue.FM2Smoke.NativeInputV6]::FindProcessMainWindow(
        $Process.Id, "Plume Debug Replay")
    if ($windowHandle -ne [IntPtr]::Zero) {
        return $windowHandle
    }
    if ($Process.MainWindowHandle -ne [IntPtr]::Zero) {
        return $Process.MainWindowHandle
    }
    return [IntPtr]::Zero
}

function Wait-FM2Window {
    param(
        [Parameter(Mandatory)]
        [System.Diagnostics.Process]$Process,
        [ValidateRange(1, 120)]
        [int]$TimeoutSeconds = 30
    )

    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        $Process.Refresh()
        if ($Process.HasExited) {
            throw "FM2 exited before a window was available. Exit code: $($Process.ExitCode)"
        }
        $windowHandle = Find-FM2WindowHandle -Process $Process
        if ($windowHandle -ne [IntPtr]::Zero) {
            return $windowHandle
        }
        Start-Sleep -Milliseconds 250
    }

    return [IntPtr]::Zero
}

function Test-ArgumentPresent {
    param(
        [Parameter(Mandatory)]
        [string[]]$Arguments,
        [Parameter(Mandatory)]
        [string]$Name
    )

    foreach ($argument in $Arguments) {
        if ($argument -ieq $Name) {
            return $true
        }
    }
    return $false
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

function Test-FM2ReceivedSpace {
    param(
        [Parameter(Mandatory)]
        [System.IO.FileInfo[]]$LogFiles
    )

    if ($LogFiles.Count -eq 0) {
        return [pscustomobject]@{
            KeyDown = 0
            KeyUp = 0
            ADown = 0
            LogFiles = @()
        }
    }

    $paths = @($LogFiles | ForEach-Object { $_.FullName })
    $keyDown = @(Select-String -LiteralPath $paths -Pattern "MNK_INPUT_KEY event=down vk=32" -ErrorAction SilentlyContinue)
    $keyUp = @(Select-String -LiteralPath $paths -Pattern "MNK_INPUT_KEY event=up vk=32" -ErrorAction SilentlyContinue)
    $aDown = @(Select-String -LiteralPath $paths -Pattern "MNK_INPUT_STATE .*a=1" -ErrorAction SilentlyContinue)

    [pscustomobject]@{
        KeyDown = $keyDown.Count
        KeyUp = $keyUp.Count
        ADown = $aDown.Count
        LogFiles = $paths
    }
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

Add-NativeInputType
Write-Host "Native input helper: ReXGlue.FM2Smoke.NativeInputV6"
Write-Host "Native INPUT size: $([ReXGlue.FM2Smoke.NativeInputV6]::InputSize())"
Write-Host "Input method: $InputMethod"
Write-Host "Trace input: $TraceInput verify input: $VerifyInput"

$process = $null
$runStartTime = Get-Date
try {
    $effectiveFm2Args = @($Fm2Args)
    if ($TraceInput -and -not (Test-ArgumentPresent -Arguments $effectiveFm2Args -Name "--log_level")) {
        $effectiveFm2Args += @("--log_level", "info")
    }

    Write-Host "Launching: $Fm2Exe"
    Write-Host "Working directory: $WorkingDirectory"
    Write-Host "Arguments: $($effectiveFm2Args -join ' ')"

    $previousMnkMode = $env:REX_MNK_MODE
    $previousTraceInput = $env:REX_MNK_TRACE_INPUT
    $previousTraceInputLimit = $env:REX_MNK_TRACE_INPUT_LIMIT
    $env:REX_MNK_MODE = "1"
    Write-Host "Child environment: REX_MNK_MODE=1"
    if ($TraceInput) {
        $env:REX_MNK_TRACE_INPUT = "1"
        $env:REX_MNK_TRACE_INPUT_LIMIT = "$TraceInputLimit"
        Write-Host "Child environment: REX_MNK_TRACE_INPUT=1 REX_MNK_TRACE_INPUT_LIMIT=$TraceInputLimit"
    }

    try {
        $process = Start-Process -FilePath $Fm2Exe -WorkingDirectory $WorkingDirectory `
            -ArgumentList $effectiveFm2Args -PassThru
    } finally {
        if ($null -eq $previousMnkMode) {
            Remove-Item Env:\REX_MNK_MODE -ErrorAction SilentlyContinue
        } else {
            $env:REX_MNK_MODE = $previousMnkMode
        }
        if ($null -eq $previousTraceInput) {
            Remove-Item Env:\REX_MNK_TRACE_INPUT -ErrorAction SilentlyContinue
        } else {
            $env:REX_MNK_TRACE_INPUT = $previousTraceInput
        }
        if ($null -eq $previousTraceInputLimit) {
            Remove-Item Env:\REX_MNK_TRACE_INPUT_LIMIT -ErrorAction SilentlyContinue
        } else {
            $env:REX_MNK_TRACE_INPUT_LIMIT = $previousTraceInputLimit
        }
    }
    Write-Host "Started fm2.exe pid=$($process.Id)"

    Write-Host "Waiting $StartupDelaySeconds seconds for startup..."
    Start-Sleep -Seconds $StartupDelaySeconds

    $windowHandle = Wait-FM2Window -Process $process -TimeoutSeconds 30
    if ($windowHandle -eq [IntPtr]::Zero) {
        throw "FM2 did not expose a main window handle; refusing to send Space to an unknown foreground window."
    }

    if (-not [ReXGlue.FM2Smoke.NativeInputV6]::EnsureForeground($windowHandle)) {
        Write-Warning "Could not foreground FM2 window 0x$($windowHandle.ToString('X')). LastError=$([ReXGlue.FM2Smoke.NativeInputV6]::LastError). SendInput taps may fail; PostMessage can still target the FM2 window."
    } else {
        $windowTitle = [ReXGlue.FM2Smoke.NativeInputV6]::GetWindowTitle($windowHandle)
        Write-Host "Foregrounded FM2 window 0x$($windowHandle.ToString('X')) title='$windowTitle'"
    }

    $tapCount = $InputDurationSeconds * $PressesPerSecond
    $tapIntervalMs = [Math]::Max(1, [int](1000 / $PressesPerSecond))
    Write-Host "Sending Space $tapCount times over $InputDurationSeconds seconds, hold=${KeyHoldMilliseconds}ms..."

    $tapOkCount = 0
    $tapFailedCount = 0
    for ($tap = 1; $tap -le $tapCount; ++$tap) {
        $tapStart = [System.Diagnostics.Stopwatch]::StartNew()
        $process.Refresh()
        if ($process.HasExited) {
            Write-Warning "FM2 exited during input drive. Exit code: $($process.ExitCode)"
            break
        }

        $freshWindowHandle = Find-FM2WindowHandle -Process $process
        if ($freshWindowHandle -ne [IntPtr]::Zero) {
            $windowHandle = $freshWindowHandle
        }

        switch ($InputMethod) {
            "auto" {
                $sendOk = [ReXGlue.FM2Smoke.NativeInputV6]::TapSpace($windowHandle, $KeyHoldMilliseconds)
                $sendError = [ReXGlue.FM2Smoke.NativeInputV6]::LastError
                if (-not $sendOk) {
                    $freshWindowHandle = Find-FM2WindowHandle -Process $process
                    if ($freshWindowHandle -ne [IntPtr]::Zero) {
                        $windowHandle = $freshWindowHandle
                    }
                    $messageOk = [ReXGlue.FM2Smoke.NativeInputV6]::TapSpaceMessage($windowHandle, $KeyHoldMilliseconds)
                    $messageError = [ReXGlue.FM2Smoke.NativeInputV6]::LastError
                    if (-not $messageOk) {
                        Write-Warning "Auto input failed on tap $tap. SendInput LastError=$sendError PostMessage LastError=$messageError"
                        ++$tapFailedCount
                    } else {
                        ++$tapOkCount
                    }
                } else {
                    ++$tapOkCount
                }
            }
            "postmessage" {
                $messageOk = [ReXGlue.FM2Smoke.NativeInputV6]::TapSpaceMessage($windowHandle, $KeyHoldMilliseconds)
                $messageError = [ReXGlue.FM2Smoke.NativeInputV6]::LastError
                if (-not $messageOk) {
                    $freshWindowHandle = Find-FM2WindowHandle -Process $process
                    if ($freshWindowHandle -ne [IntPtr]::Zero) {
                        $windowHandle = $freshWindowHandle
                    }
                    $messageOk = [ReXGlue.FM2Smoke.NativeInputV6]::TapSpaceMessage($windowHandle, $KeyHoldMilliseconds)
                    $messageError = [ReXGlue.FM2Smoke.NativeInputV6]::LastError
                }
                if (-not $messageOk) {
                    Write-Warning "PostMessage failed on tap $tap. LastError=$messageError"
                    ++$tapFailedCount
                } else {
                    ++$tapOkCount
                }
            }
            "sendinput" {
                $sendOk = [ReXGlue.FM2Smoke.NativeInputV6]::TapSpace($windowHandle, $KeyHoldMilliseconds)
                $sendError = [ReXGlue.FM2Smoke.NativeInputV6]::LastError
                if (-not $sendOk) {
                    $freshWindowHandle = Find-FM2WindowHandle -Process $process
                    if ($freshWindowHandle -ne [IntPtr]::Zero) {
                        $windowHandle = $freshWindowHandle
                    }
                    $sendOk = [ReXGlue.FM2Smoke.NativeInputV6]::TapSpace($windowHandle, $KeyHoldMilliseconds)
                    $sendError = [ReXGlue.FM2Smoke.NativeInputV6]::LastError
                }
                if (-not $sendOk) {
                    Write-Warning "SendInput failed on tap $tap. LastError=$sendError"
                    ++$tapFailedCount
                } else {
                    ++$tapOkCount
                }
            }
            "both" {
                $messageOk = [ReXGlue.FM2Smoke.NativeInputV6]::TapSpaceMessage($windowHandle, $KeyHoldMilliseconds)
                $messageError = [ReXGlue.FM2Smoke.NativeInputV6]::LastError
                $sendOk = [ReXGlue.FM2Smoke.NativeInputV6]::TapSpace($windowHandle, $KeyHoldMilliseconds)
                $sendError = [ReXGlue.FM2Smoke.NativeInputV6]::LastError
                if (-not $messageOk -and -not $sendOk) {
                    Write-Warning "Both input methods failed on tap $tap. PostMessage LastError=$messageError SendInput LastError=$sendError"
                    ++$tapFailedCount
                } else {
                    ++$tapOkCount
                }
            }
        }

        $tapStart.Stop()
        $remainingMs = $tapIntervalMs - [int]$tapStart.ElapsedMilliseconds
        if ($remainingMs -gt 0) {
            Start-Sleep -Milliseconds $remainingMs
        }
    }
    Write-Host "Space tap result: ok=$tapOkCount failed=$tapFailedCount"

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
    if ($process) {
        $process.Refresh()
        if (-not $process.HasExited) {
            Write-Host "Stopping fm2.exe pid=$($process.Id)"
            Stop-Process -Id $process.Id -Force
            $process.WaitForExit(5000) | Out-Null
        }
    }
}

$recentAppLogs = Find-RecentFM2AppLogs -Fm2Executable $Fm2Exe -Since $runStartTime
if ($recentAppLogs.Count -gt 0) {
    Write-Host "Recent FM2 app logs:"
    foreach ($appLog in $recentAppLogs) {
        Write-Host "  $($appLog.FullName) length=$($appLog.Length) lastWrite=$($appLog.LastWriteTime)"
    }
}

if ($TraceInput -and $VerifyInput) {
    $inputVerification = Test-FM2ReceivedSpace -LogFiles $recentAppLogs
    Write-Host "Input receiver trace: keyDown=$($inputVerification.KeyDown) keyUp=$($inputVerification.KeyUp) aDown=$($inputVerification.ADown)"
    $minimumExpectedKeyEvents = [Math]::Max(1, $tapOkCount)
    if ($inputVerification.KeyDown -lt $minimumExpectedKeyEvents -or
        $inputVerification.KeyUp -lt $minimumExpectedKeyEvents -or
        $inputVerification.ADown -le 0) {
        throw "FM2 did not log enough received Space/A input. Expected at least $minimumExpectedKeyEvents Space down/up events from successful taps. Check foreground/integrity level and recent app logs under the FM2 build logs directory."
    }
}

if (Test-Path -LiteralPath $LogPath) {
    $logItem = Get-Item -LiteralPath $LogPath
    Write-Host "Log: $($logItem.FullName) length=$($logItem.Length) lastWrite=$($logItem.LastWriteTime)"
}
