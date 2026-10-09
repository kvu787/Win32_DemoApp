param([ValidateSet('NE18NZ2')][string]$TestMonitor, [string]$Report, [string]$TestBuildDirectory)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'TestBuilds'))
$latest = Get-Content -LiteralPath (Join-Path $root 'LatestReady.json') -Raw | ConvertFrom-Json
$directory = [IO.Path]::GetFullPath((Join-Path $root $latest.Directory))
if ($TestBuildDirectory) {
    if (!$TestMonitor) { throw 'A test build override requires the explicit NE18NZ2 launcher-test mode.' }
    $directory = [IO.Path]::GetFullPath($TestBuildDirectory)
}
if (!$directory.StartsWith($root + [IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Invalid ready-build path.' }
$executable = Join-Path $directory 'Win32_DemoApp.exe'
if ($TestMonitor) {
    if (!$Report) { throw 'A report path is required for a launcher test.' }
    $process = Start-Process -FilePath $executable -WorkingDirectory $directory -ArgumentList @('--launcher-test','--report',('"' + [IO.Path]::GetFullPath($Report) + '"')) -PassThru -Wait
    if ($process.ExitCode -ne 0) { throw "Launcher test failed; see $Report" }
} else {
    Start-Process -FilePath $executable -WorkingDirectory $directory
}
