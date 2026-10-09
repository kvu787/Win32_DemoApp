Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'TestBuilds'))
$latest = Get-Content -LiteralPath (Join-Path $root 'LatestReady.json') -Raw | ConvertFrom-Json
$directory = [IO.Path]::GetFullPath((Join-Path $root $latest.Directory))
if (!$directory.StartsWith($root + [IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Invalid ready-build path.' }
$executable = Join-Path $directory 'Win32_DemoApp.exe'
Start-Process -FilePath $executable -WorkingDirectory $directory -WindowStyle Hidden
