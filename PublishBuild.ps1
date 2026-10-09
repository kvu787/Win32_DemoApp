param([switch]$GuiAudit, [switch]$Candidate)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    if (!$Candidate -and !$GuiAudit) { throw 'Ready publication requires -GuiAudit on an unlocked, cleared NE18NZ2 slot. Use -Candidate for hidden-only verification.' }
    if ($Candidate -and $GuiAudit) { throw 'Choose either an unverified candidate or verified GUI publication.' }
    $changes = & git status --porcelain
    if ($LASTEXITCODE -ne 0 -or $changes) { throw 'Commit the source before creating a ready snapshot.' }
    $commit = (& git rev-parse HEAD).Trim()
    & (Join-Path $PSScriptRoot 'Build.ps1') -Test
    $root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'TestBuilds'))
    New-Item -ItemType Directory -Path $root -Force | Out-Null
    $name = [DateTime]::UtcNow.ToString('yyyy-MM-dd_HH-mm-ss-fff') + '_' + $commit.Substring(0,12)
    if ($Candidate) { $name = 'Candidate_' + $name }
    $snapshot = [IO.Path]::GetFullPath((Join-Path $root $name))
    if (!$snapshot.StartsWith($root + [IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $snapshot)) { throw 'Snapshot must be a new directory inside TestBuilds.' }
    New-Item -ItemType Directory -Path $snapshot | Out-Null
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'BuildOutput\Release\Win32_DemoApp.exe') -Destination $snapshot
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'README.md') -Destination $snapshot
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Documentation') -Destination $snapshot -Recurse
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Tools\SnapshotRun.cmd') -Destination (Join-Path $snapshot 'Run.cmd')
    $reports = Join-Path $snapshot 'Verification'
    New-Item -ItemType Directory -Path $reports | Out-Null
    $report = Join-Path $reports 'IndependentLaunch.txt'
    $process = Start-Process -FilePath (Join-Path $snapshot 'Win32_DemoApp.exe') -ArgumentList @('--self-test','--report',('"' + $report + '"')) -WorkingDirectory ([IO.Path]::GetTempPath()) -WindowStyle Hidden -PassThru -Wait
    if ($process.ExitCode -ne 0) { throw "Copied executable verification failed; this directory is not ready: $snapshot" }
    if ($GuiAudit) {
        $guiReport = Join-Path $reports 'GuiAudit.txt'
        $gui = Start-Process -FilePath (Join-Path $snapshot 'Win32_DemoApp.exe') -ArgumentList @('--gui-test','--report',('"' + $guiReport + '"')) -WorkingDirectory ([IO.Path]::GetTempPath()) -PassThru -Wait
        if ($gui.ExitCode -ne 0) { throw "NE18NZ2 GUI audit failed; this directory is not ready: $snapshot" }
        & (Join-Path $PSScriptRoot 'RunLatestBuild.ps1') -TestMonitor NE18NZ2 -TestBuildDirectory $snapshot -Report (Join-Path $reports 'PowerShellLauncher.txt')
        & (Join-Path $PSScriptRoot 'RunLatestBuild.cmd') -TestMonitor NE18NZ2 -TestBuildDirectory $snapshot -Report (Join-Path $reports 'CmdLauncher.txt')
        if ($LASTEXITCODE -ne 0) { throw 'Root CMD launcher verification failed; snapshot is not ready.' }
    }
    $information = [ordered]@{
        Project = 'Win32_DemoApp'
        SourceCommit = $commit
        CreatedUtc = [DateTime]::UtcNow.ToString('o')
        Configuration = 'Windows 11 x64 Release; static MSVC runtime; embedded resources'
        ExecutableSha256 = (Get-FileHash -LiteralPath (Join-Path $snapshot 'Win32_DemoApp.exe')).Hash
        IndependentLaunchVerified = $true
        GuiAudit = [bool]$GuiAudit
        GuiTestMonitor = 'NE18NZ2; exact friendly identity; SW_SHOWNOACTIVATE; no input injection'
        Limitations = 'Modal cancellation, keyboard focus traversal, real monitor DPI transitions, accessibility/high contrast and all variants require manual verification.'
        Ready = !$Candidate
        Candidate = [bool]$Candidate
    }
    $information | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $snapshot 'BuildInfo.json') -Encoding utf8
    Get-ChildItem -LiteralPath $snapshot -Recurse -File | ForEach-Object { $_.IsReadOnly = $true }
    if (!$Candidate) {
        $pointer = Join-Path $root ('LatestReady.' + [guid]::NewGuid().ToString('N') + '.json')
        @{ Directory=$name; Executable='Win32_DemoApp.exe'; SourceCommit=$commit } | ConvertTo-Json | Set-Content -LiteralPath $pointer -Encoding utf8
        [IO.File]::Move($pointer,(Join-Path $root 'LatestReady.json'),$true)
    }
    Write-Host "$(if ($Candidate) { 'Unverified GUI candidate' } else { 'Ready executable' }): $snapshot\Win32_DemoApp.exe"
    Write-Host "Launcher: $snapshot\Run.cmd"
} finally { Pop-Location }
