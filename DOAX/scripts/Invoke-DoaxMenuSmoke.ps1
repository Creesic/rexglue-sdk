<#
.SYNOPSIS
    Headless DOAX smoke run: boot -> drive Start/A -> screenshot-classify black vs visible.
.DESCRIPTION
    Launches doax.exe with the automation gamepad (REX_AUTOMATION_GAMEPAD_FILE),
    taps Start+A to advance through the boot intros / Press Start / four-menu /
    Travel, then captures the window client area and counts non-dark pixels to
    classify the result GOOD (visible) vs BLACK (the title->menu bug). Also
    reports the run's log file + peak guest draw count + fiber-swap count.

    Ported/adapted from plume scripts/fm2/Invoke-FM2GameplaySmoke.ps1.
#>
[CmdletBinding()]
param(
    [string]$DoaxExe,
    [string]$WorkingDirectory,
    [string]$GameDataRoot,
    [ValidateRange(0,120)][int]$StartupDelaySeconds = 8,
    [ValidateRange(1,180)][int]$InputDurationSeconds = 22,
    [ValidateRange(1,10)][int]$PressesPerSecond = 2,
    [ValidateRange(1,1000)][int]$KeyHoldMilliseconds = 140,
    # Buttons tapped each press. 0x1010 = A (0x1000) | Start (0x0010): advances
    # intros, Press Start, and confirms the default four-menu item.
    [int]$ButtonMask = 0x1010,
    [ValidateRange(0,60)][int]$PostInputDelaySeconds = 4,
    [string]$GamepadFile = (Join-Path $env:TEMP "rex-doax-automation-gamepad-state.txt"),
    [string]$ScreenshotPath = (Join-Path $env:TEMP ("doax-smoke-{0:yyyyMMdd-HHmmss}.png" -f (Get-Date))),
    [string]$WindowTitlePart = "doax",
    [ValidateRange(0,765)][int]$DarkLuminanceThreshold = 48,
    # Non-dark sampled pixels at/above which the frame is judged VISIBLE (GOOD).
    [int]$VisibleNonDarkPixels = 1500
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Add-CaptureType {
    if ("ReXGlue.DoaxSmoke.WindowCaptureV1" -as [type]) { return }
    Add-Type -AssemblyName System.Drawing
    Add-Type -ReferencedAssemblies @("System.Drawing.dll") -TypeDefinition @"
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
namespace ReXGlue.DoaxSmoke {
  public sealed class CaptureResult {
    public bool Ok; public string Status; public int Width; public int Height;
    public int SampledPixels; public int NonDarkPixels; public double AverageLuminance;
  }
  public static class WindowCaptureV1 {
    private delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [StructLayout(LayoutKind.Sequential)] private struct RECT { public int left, top, right, bottom; }
    [StructLayout(LayoutKind.Sequential)] private struct POINT { public int x, y; }
    [DllImport("user32.dll")] private static extern bool EnumWindows(EnumWindowsProc f, IntPtr p);
    [DllImport("user32.dll")] private static extern bool IsWindowVisible(IntPtr h);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] private static extern int GetWindowTextW(IntPtr h, StringBuilder s, int n);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] private static extern int GetWindowTextLengthW(IntPtr h);
    [DllImport("user32.dll")] private static extern bool GetClientRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] private static extern bool ClientToScreen(IntPtr h, ref POINT p);
    [DllImport("user32.dll")] private static extern bool ShowWindow(IntPtr h, int c);
    [DllImport("user32.dll")] private static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int cx, int cy, uint flags);
    [DllImport("user32.dll")] private static extern int GetWindowThreadProcessId(IntPtr h, out int pid);
    private static readonly IntPtr HWND_TOP = IntPtr.Zero;
    private const int SW_RESTORE = 9; private const uint SWP_NOACTIVATE = 0x0010; private const uint SWP_SHOWWINDOW = 0x0040;
    // Find the visible top-level window owned by a specific process (robust:
    // never matches an unrelated window that merely has 'doax' in its title).
    public static IntPtr FindVisibleWindowByProcessId(int targetPid) {
      IntPtr found = IntPtr.Zero;
      EnumWindows(delegate(IntPtr h, IntPtr l) {
        if (!IsWindowVisible(h)) return true;
        int pid; GetWindowThreadProcessId(h, out pid);
        if (pid != targetPid) return true;
        RECT r; if (!GetClientRect(h, out r)) return true;
        if ((r.right - r.left) <= 0 || (r.bottom - r.top) <= 0) return true;
        found = h; return false;
      }, IntPtr.Zero);
      return found;
    }
    public static IntPtr FindVisibleWindowByTitlePart(string part) {
      IntPtr found = IntPtr.Zero;
      EnumWindows(delegate(IntPtr h, IntPtr l) {
        if (!IsWindowVisible(h)) return true;
        int len = GetWindowTextLengthW(h); if (len <= 0) return true;
        StringBuilder t = new StringBuilder(len + 1); GetWindowTextW(h, t, t.Capacity);
        if (t.ToString().IndexOf(part, StringComparison.OrdinalIgnoreCase) >= 0) { found = h; return false; }
        return true;
      }, IntPtr.Zero);
      return found;
    }
    public static CaptureResult CaptureClientToPng(IntPtr h, string path, int darkThreshold) {
      CaptureResult r = new CaptureResult();
      if (h == IntPtr.Zero) { r.Status = "missing-window"; return r; }
      RECT rect; if (!GetClientRect(h, out rect)) { r.Status = "client-rect-failed"; return r; }
      int w = rect.right - rect.left, ht = rect.bottom - rect.top; r.Width = w; r.Height = ht;
      if (w <= 0 || ht <= 0) { r.Status = "empty-client"; return r; }
      ShowWindow(h, SW_RESTORE);
      SetWindowPos(h, HWND_TOP, 50, 50, w, ht, SWP_SHOWWINDOW | SWP_NOACTIVATE);
      Thread.Sleep(750);
      POINT o = new POINT { x = 0, y = 0 }; ClientToScreen(h, ref o);
      using (Bitmap bmp = new Bitmap(w, ht, PixelFormat.Format32bppArgb)) {
        using (Graphics g = Graphics.FromImage(bmp)) { g.CopyFromScreen(o.x, o.y, 0, 0, new Size(w, ht)); }
        long lumSum = 0; int nonDark = 0, sampled = 0;
        int sy = Math.Max(1, ht / 180), sx = Math.Max(1, w / 320);
        for (int y = 0; y < ht; y += sy) for (int x = 0; x < w; x += sx) {
          Color c = bmp.GetPixel(x, y); int lum = c.R + c.G + c.B; lumSum += lum; ++sampled;
          if (lum > darkThreshold) ++nonDark;
        }
        string dir = Path.GetDirectoryName(path); if (!String.IsNullOrEmpty(dir)) Directory.CreateDirectory(dir);
        bmp.Save(path, ImageFormat.Png);
        r.Ok = true; r.Status = "ok"; r.SampledPixels = sampled; r.NonDarkPixels = nonDark;
        r.AverageLuminance = sampled != 0 ? (double)lumSum / sampled : 0.0;
        return r;
      }
    }
  }
}
"@
}

function Write-GamepadState {
    param([string]$Path, [int]$Packet, [int]$Buttons)
    $content = @(
        "packet=$Packet", ("buttons=0x{0:X4}" -f $Buttons),
        "left_trigger=0", "right_trigger=0",
        "thumb_lx=0", "thumb_ly=0", "thumb_rx=0", "thumb_ry=0"
    ) -join "`n"
    $tmp = "$Path.tmp"
    Set-Content -LiteralPath $tmp -Value ($content + "`n") -Encoding ascii
    for ($i = 1; $i -le 20; ++$i) {
        try { Move-Item -LiteralPath $tmp -Destination $Path -Force -ErrorAction Stop; return }
        catch { if ($i -eq 20) { Set-Content -LiteralPath $Path -Value ($content + "`n") -Encoding ascii; return }; Start-Sleep -Milliseconds 10 }
    }
}

# --- resolve paths ---
$scriptRoot = Split-Path -Parent $PSCommandPath
$doaxRoot = Resolve-Path -LiteralPath (Join-Path $scriptRoot "..")
if (-not $DoaxExe) { $DoaxExe = Join-Path $doaxRoot "out\build\win-amd64-relwithdebinfo\doax.exe" }
if (-not $WorkingDirectory) { $WorkingDirectory = Split-Path -Parent $DoaxExe }
if (-not $GameDataRoot) { $GameDataRoot = (Resolve-Path -LiteralPath (Join-Path $doaxRoot "assets")).Path }
$DoaxExe = [IO.Path]::GetFullPath($DoaxExe); $WorkingDirectory = [IO.Path]::GetFullPath($WorkingDirectory)
if (-not (Test-Path -LiteralPath $DoaxExe)) { throw "doax.exe not found: $DoaxExe" }

$logDir = Join-Path $WorkingDirectory "logs"
$runStart = Get-Date

Write-Host "DOAX smoke: exe=$DoaxExe"
Write-Host "  gamepad=$GamepadFile buttons=0x$('{0:X4}' -f $ButtonMask) input=${InputDurationSeconds}s"

$process = $null
try {
    Write-GamepadState -Path $GamepadFile -Packet 0 -Buttons 0
    $env:REX_AUTOMATION_GAMEPAD_FILE = $GamepadFile
    try {
        $process = Start-Process -FilePath $DoaxExe -WorkingDirectory $WorkingDirectory `
            -ArgumentList @("--game_data_root", $GameDataRoot) -PassThru
    } finally { Remove-Item Env:\REX_AUTOMATION_GAMEPAD_FILE -ErrorAction SilentlyContinue }
    Write-Host "  pid=$($process.Id); startup wait ${StartupDelaySeconds}s..."
    Start-Sleep -Seconds $StartupDelaySeconds

    $taps = $InputDurationSeconds * $PressesPerSecond
    $intervalMs = [Math]::Max(1, [int](1000 / $PressesPerSecond))
    $packet = 1
    $crashed = $false
    $exitCode = 0
    for ($t = 1; $t -le $taps; ++$t) {
        $process.Refresh()
        if ($process.HasExited) { $crashed = $true; $exitCode = $process.ExitCode; Write-Warning ("doax exited during input (exit=0x{0:X8})" -f $exitCode); break }
        Write-GamepadState -Path $GamepadFile -Packet $packet -Buttons $ButtonMask
        Start-Sleep -Milliseconds ([Math]::Max(1, $KeyHoldMilliseconds))
        Write-GamepadState -Path $GamepadFile -Packet ($packet + 1) -Buttons 0
        $packet += 2
        Start-Sleep -Milliseconds $intervalMs
    }
    Write-Host "  drove $taps taps; post-input wait ${PostInputDelaySeconds}s..."
    Start-Sleep -Seconds $PostInputDelaySeconds

    # --- screenshot + classify (PID-scoped; CRASHED if the game process died) ---
    Add-CaptureType
    $process.Refresh()
    if ($process.HasExited) { $crashed = $true; if ($exitCode -eq 0) { $exitCode = $process.ExitCode } }
    $cap = $null
    if ($crashed) {
        $verdict = "CRASHED"
        Write-Host ("  process exited (exit=0x{0:X8}) -> no live game window to capture" -f $exitCode)
    } else {
        $hwnd = [ReXGlue.DoaxSmoke.WindowCaptureV1]::FindVisibleWindowByProcessId($process.Id)
        if ($hwnd -eq [IntPtr]::Zero) { $hwnd = $process.MainWindowHandle }
        $cap = [ReXGlue.DoaxSmoke.WindowCaptureV1]::CaptureClientToPng($hwnd, $ScreenshotPath, $DarkLuminanceThreshold)
        $verdict = if ($cap.Ok -and $cap.NonDarkPixels -ge $VisibleNonDarkPixels) { "VISIBLE" }
                   elseif ($cap.Ok) { "BLACK" } else { "NO-CAPTURE" }
        Write-Host ("  capture: status={0} {1}x{2} sampled={3} nonDark={4} avgLum={5} -> {6}" -f `
            $cap.Status, $cap.Width, $cap.Height, $cap.SampledPixels, $cap.NonDarkPixels, [Math]::Round($cap.AverageLuminance,1), $verdict)
        Write-Host "  screenshot: $ScreenshotPath"
    }
} finally {
    Write-GamepadState -Path $GamepadFile -Packet 0 -Buttons 0
    if ($process) { $process.Refresh(); if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force; $process.WaitForExit(5000) | Out-Null } }
}

# --- log summary ---
$log = $null
if (Test-Path -LiteralPath $logDir) {
    $log = Get-ChildItem -LiteralPath $logDir -Filter "doax_*.log" -File |
        Where-Object { $_.Name -notmatch '\.\d+\.log$' -and $_.LastWriteTime -ge $runStart.AddSeconds(-5) } |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
}
if ($log) {
    $draws = Select-String -Path $log.FullName -Pattern 'draws=(\d+)' -AllMatches |
        ForEach-Object { $_.Matches } | ForEach-Object { [int]$_.Groups[1].Value }
    $maxDraws = if ($draws) { ($draws | Measure-Object -Maximum).Maximum } else { 0 }
    $fiberSw = (Select-String -Path $log.FullName -Pattern 'FIBERSW ').Count
    Write-Host ("  log: {0}  peakDraws={1}  fiberSwaps={2}" -f $log.Name, $maxDraws, $fiberSw)
} else {
    Write-Host "  log: (none found for this run)"
}
$nonDarkStr = if ($cap) { $cap.NonDarkPixels } else { 'n/a' }
$exitStr = if ($crashed) { (" exit=0x{0:X8}" -f $exitCode) } else { '' }
Write-Host "RESULT verdict=$verdict nonDark=$nonDarkStr log=$(if($log){$log.Name}else{'none'})$exitStr"
