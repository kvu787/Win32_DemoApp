param([ValidateSet('Release','Debug')][string]$Configuration = 'Release', [switch]$Test)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Install/enable the Visual Studio x64 C++ tools and Windows SDK before building.' }
& (Join-Path $vs 'Common7\Tools\Launch-VsDevShell.ps1') -Arch amd64 -HostArch amd64 -SkipAutomaticLocation | Out-Null
$output = Join-Path $PSScriptRoot "BuildOutput\$Configuration"
New-Item -ItemType Directory -Path $output -Force | Out-Null
Push-Location (Join-Path $PSScriptRoot 'Source')
try {
    & rc.exe /nologo /fo (Join-Path $output 'App.res') App.rc
    if ($LASTEXITCODE -ne 0) { throw 'Resource compilation failed.' }
    $flags = if ($Configuration -eq 'Release') { @('/O2','/MT') } else { @('/Od','/MTd','/Zi') }
    & cl.exe /nologo /std:c++20 /W4 /permissive- /utf-8 /EHsc /DUNICODE /D_UNICODE /DWIN32_LEAN_AND_MEAN /DNOMINMAX /D_WIN32_WINNT=0x0A00 @flags ("/Fo$output\") ("/Fe$output\Win32_DemoApp.exe") ("/Fd$output\Win32_DemoApp.pdb") Main.cpp Gallery.cpp TextSearch.cpp Regression.cpp App.cpp (Join-Path $output 'App.res') /link /SUBSYSTEM:WINDOWS /MANIFEST:NO comctl32.lib comdlg32.lib shell32.lib ole32.lib uuid.lib uxtheme.lib dwmapi.lib user32.lib gdi32.lib
    if ($LASTEXITCODE -ne 0) { throw 'C++ build failed.' }
} finally { Pop-Location }
if ($Test) {
    $process = Start-Process -FilePath (Join-Path $output 'Win32_DemoApp.exe') -ArgumentList @('--self-test','--report',('"' + (Join-Path $output 'Audit.txt') + '"')) -WindowStyle Hidden -PassThru -Wait
    if ($process.ExitCode -ne 0) { throw "Native audit failed; see $output\Audit.txt" }
    Get-Content -LiteralPath (Join-Path $output 'Audit.txt') | Select-Object -Last 12
}
Write-Host "Executable: $output\Win32_DemoApp.exe"
