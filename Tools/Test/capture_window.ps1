# Captures the Unreal Editor window (including PIE + UI) with Win32 PrintWindow.
# Safer than "shot showui", which crashed the editor once in UE 5.8.
# usage: powershell -ExecutionPolicy Bypass -File capture_window.ps1 -Out C:\path\shot.png [-Title "Unreal Editor"]
param([string]$Out = "$PSScriptRoot\..\..\Saved\Screenshots\capture.png", [string]$Title = "Unreal Editor")

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class Win32Capture {
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdc, uint flags);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hwnd, out RECT rect);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
"@

$proc = Get-Process UnrealEditor -ErrorAction Stop | Where-Object { $_.MainWindowHandle -ne 0 -and $_.MainWindowTitle -like "*$Title*" } | Select-Object -First 1
if (-not $proc) { $proc = Get-Process UnrealEditor | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1 }
$h = $proc.MainWindowHandle
$r = New-Object Win32Capture+RECT
[Win32Capture]::GetWindowRect($h, [ref]$r) | Out-Null
$w = $r.Right - $r.Left; $hgt = $r.Bottom - $r.Top
$bmp = New-Object System.Drawing.Bitmap $w, $hgt
$g = [System.Drawing.Graphics]::FromImage($bmp)
$hdc = $g.GetHdc()
[Win32Capture]::PrintWindow($h, $hdc, 2) | Out-Null   # PW_RENDERFULLCONTENT
$g.ReleaseHdc($hdc)
New-Item -ItemType Directory -Force -Path (Split-Path $Out) | Out-Null
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()
Write-Output "saved $Out ($w x $hgt)"
