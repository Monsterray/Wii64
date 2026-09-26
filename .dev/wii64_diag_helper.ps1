# wii64_diag_helper.ps1 - PowerShell half of wii64_diag.sh. Not committed
# (.dev/ is gitignored). Written once instead of re-derived by hand every
# time (this exact EnumWindows/PrintWindow/CPU-sampling code was retyped
# from scratch, with mistakes, several times across one debugging session
# before landing here).
param(
    [Parameter(Mandatory=$true)][string]$Action,
    [string]$DolphinExe,
    [string]$Dol,
    [string]$Profile,
    [string]$State,
    [string]$Out,
    [switch]$Batch,   # -b: Dolphin exits when the guest powers off (a chain's end)
    [switch]$Record   # no -e: the dol is the default game, booted by Movie > Start Recording Input
)

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Collections.Generic;
public class Wii64DiagWin {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc enumProc, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr hWnd, StringBuilder text, int count);
    [DllImport("user32.dll")] public static extern int GetWindowTextLength(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint procId);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdcBlt, uint nFlags);
    public static List<IntPtr> Windows = new List<IntPtr>();
    public static bool Report(IntPtr hWnd, IntPtr lParam) {
        if (IsWindowVisible(hWnd)) Windows.Add(hWnd);
        return true;
    }
}
"@

function Get-RenderWindow($targetPid) {
    # Dolphin shows two top-level windows: the small render window, titled
    # "Dolphin <ver> | JIT64 <SC|DC> | <backend> | <HLE|LLE>", and the main
    # GUI shell, titled just "Dolphin <ver>". "JIT64" in the title is the
    # reliable discriminator (confirmed across many captures this session --
    # window size alone drifts with DPI/theme).
    [Wii64DiagWin]::Windows.Clear()
    $cb = [Wii64DiagWin+EnumWindowsProc]{ param($h,$l) [Wii64DiagWin]::Report($h,$l) }
    # [void]: EnumWindows returns bool, and an unsuppressed return value here
    # leaks into THIS function's own output, turning $hwnd (at the call site)
    # into a [bool,IntPtr] array instead of a plain IntPtr -- confirmed by
    # the ".ToInt64() argument count 0" error that produced before this fix.
    [void][Wii64DiagWin]::EnumWindows($cb, [IntPtr]::Zero)
    foreach ($h in [Wii64DiagWin]::Windows) {
        $len = [Wii64DiagWin]::GetWindowTextLength($h)
        if ($len -eq 0) { continue }
        $sb = New-Object System.Text.StringBuilder ($len+1)
        [Wii64DiagWin]::GetWindowText($h, $sb, $sb.Capacity) | Out-Null
        $procId = 0
        [Wii64DiagWin]::GetWindowThreadProcessId($h, [ref]$procId) | Out-Null
        if ($procId -eq $targetPid -and $sb.ToString() -like "*JIT64*") {
            return $h
        }
    }
    return [IntPtr]::Zero
}

switch ($Action) {
    "start" {
        # A chain (-Batch) also reads the XFB back with the CPU for its per-game snapshot,
        # which only works with XFB copies written to emulated RAM (as on a Wii) --
        # Dolphin's default keeps them as host textures and RAM holds stale bytes.
        $batchArg = @(); if ($Batch) { $batchArg = @('-b', '-C', 'Graphics.Hacks.XFBToTextureEnable=False') }
        $bootArg = @('-e', $Dol); if ($Record) { $bootArg = @('-C', "Dolphin.Core.DefaultISO=$Dol") }
        $p = Start-Process -FilePath $DolphinExe -ArgumentList ($batchArg + $bootArg + @(
            '-u', $Profile,
            '-C', 'Graphics.Hacks.ImmediateXFBEnable=True',
            '-C', 'Dolphin.Interface.UsePanicHandlers=False',
            '-C', 'Dolphin.Interface.ConfirmStop=False',
            '-C', 'Dolphin.Core.CPUThread=True',
            '-C', 'Dolphin.Core.WiiSDCard=True',
            '-C', 'Dolphin.Core.WiiSDCardAllowWrites=True',
            '-C', 'Dolphin.Core.WiiSDCardEnableFolderSync=True'
        )) -PassThru
        $hwnd = [IntPtr]::Zero
        for ($i = 0; $i -lt 40 -and $hwnd -eq [IntPtr]::Zero -and -not $Record; $i++) {
            Start-Sleep -Milliseconds 500
            $hwnd = Get-RenderWindow $p.Id
        }
        if ($hwnd -eq [IntPtr]::Zero -and -not $Record) {
            Write-Output "WARNING: launched (PID $($p.Id)) but never found the render window"
        }
        "PID=$($p.Id)`nHWND=$($hwnd.ToInt64())" | Set-Content -Path $State
        Write-Output "Started PID=$($p.Id) hwnd=$($hwnd.ToInt64())"
    }
    "shot" {
        $lines = Get-Content $State
        $hwndVal = [int64](($lines | Where-Object { $_ -like 'HWND=*' }) -replace 'HWND=','')
        $hwnd = [IntPtr]$hwndVal
        $bmp = New-Object System.Drawing.Bitmap 900,650
        $g = [System.Drawing.Graphics]::FromImage($bmp)
        $hdc = $g.GetHdc()
        [Wii64DiagWin]::PrintWindow($hwnd, $hdc, 2) | Out-Null   # PW_RENDERFULLCONTENT
        $g.ReleaseHdc($hdc)
        $bmp.Save($Out)
        Write-Output "Saved $Out"
    }
    "status" {
        $lines = Get-Content $State
        $pidVal = [int](($lines | Where-Object { $_ -like 'PID=*' }) -replace 'PID=','')
        $proc = Get-Process -Id $pidVal -ErrorAction SilentlyContinue
        if (-not $proc) { Write-Output "PID $pidVal is not running"; break }
        $c1 = $proc.CPU
        Start-Sleep -Seconds 3
        $proc2 = Get-Process -Id $pidVal -ErrorAction SilentlyContinue
        $c2 = if ($proc2) { $proc2.CPU } else { $c1 }
        $pct = ($c2 - $c1) / 3 * 100
        Write-Output ("PID={0} Responding={1} CPU_over_3s={2:N1}% of one core" -f $pidVal, $proc.Responding, $pct)
    }
}
