param([string]$Report)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$launcherSource = Join-Path (Split-Path $PSScriptRoot -Parent) 'RunLatestBuild.ps1'
$launcherTempBase = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$launcherFixtureRoot = [IO.Path]::GetFullPath((Join-Path $launcherTempBase ('Win32_DemoApp_Launcher_' + [guid]::NewGuid().ToString('N'))))
if (!$launcherFixtureRoot.StartsWith($launcherTempBase,[StringComparison]::OrdinalIgnoreCase)) { throw 'Fixture must remain in the temporary directory.' }
$launcherCalls = [Collections.Generic.List[object]]::new()
$launcherResults = [Collections.Generic.List[string]]::new()

# Resolve the real script with an isolated TestBuilds folder. Never create a native process.
function Start-Process {
    [CmdletBinding()]
    param([string]$FilePath, [string]$WorkingDirectory, [string[]]$ArgumentList, [switch]$PassThru, [switch]$Wait)
    $launcherCalls.Add([pscustomobject]@{ FilePath=$FilePath; WorkingDirectory=$WorkingDirectory; Arguments=$ArgumentList; Wait=[bool]$Wait })
    [pscustomobject]@{ ExitCode=0 }
}
function Test-LauncherCondition([bool]$Condition, [string]$Name) {
    if (!$Condition) { throw $Name }
    $launcherResults.Add('PASS launcher: ' + $Name)
}
function Test-LauncherRejection([scriptblock]$Action, [string]$Pattern, [string]$Name) {
    $rejection = ''
    try { & $Action | Out-Null } catch { $rejection = $_.Exception.Message }
    Test-LauncherCondition ($rejection -match $Pattern) $Name
}
try {
    $fixtureBuilds = Join-Path $launcherFixtureRoot 'TestBuilds'
    $fixtureCandidate = Join-Path $fixtureBuilds 'FirstCandidate'
    New-Item -ItemType Directory -Path $fixtureCandidate | Out-Null
    $fixtureLauncher = Join-Path $launcherFixtureRoot 'RunLatestBuild.ps1'
    Copy-Item -LiteralPath $launcherSource -Destination $fixtureLauncher
    $fixturePointer = Join-Path $fixtureBuilds 'LatestReady.json'
    $fixtureReport = Join-Path $launcherFixtureRoot 'MockLaunch.txt'
    $fixtureExecutable = Join-Path $fixtureCandidate 'Win32_DemoApp.exe'
    [IO.File]::WriteAllBytes($fixtureExecutable, [byte[]]@())
    Test-LauncherCondition (!(Test-Path -LiteralPath $fixturePointer)) 'fixture starts with no LatestReady pointer'
    & $fixtureLauncher -TestMonitor NE18NZ2 -TestBuildDirectory $fixtureCandidate -Report $fixtureReport | Out-Null
    Test-LauncherCondition ($launcherCalls.Count -eq 1 -and $launcherCalls[0].FilePath -eq $fixtureExecutable -and $launcherCalls[0].WorkingDirectory -eq $fixtureCandidate -and $launcherCalls[0].Arguments[0] -eq '--launcher-test' -and $launcherCalls[0].Wait) 'explicit first-build override resolves without a pointer'
    Test-LauncherCondition (!(Test-Path -LiteralPath $fixturePointer)) 'override never creates or changes a pointer'
    Test-LauncherRejection { & $fixtureLauncher } 'LatestReady\.json' 'ordinary launch reports a missing pointer'
    Test-LauncherRejection { & $fixtureLauncher -TestBuildDirectory $fixtureCandidate } 'requires the explicit' 'override still requires NE18NZ2 test mode before pointer access'
    Test-LauncherRejection { & $fixtureLauncher -TestMonitor NE18NZ2 -TestBuildDirectory $launcherFixtureRoot -Report $fixtureReport } 'Invalid ready-build path' 'outside-TestBuilds override remains rejected'
    Test-LauncherRejection { & $fixtureLauncher -TestMonitor NE18NZ2 -TestBuildDirectory $fixtureCandidate } 'report path is required' 'override still requires a test report'
    Test-LauncherCondition ($launcherCalls.Count -eq 1) 'rejected inputs invoke no mocked process'

    @{ Directory='FirstCandidate'; Executable='Win32_DemoApp.exe' } | ConvertTo-Json | Set-Content -LiteralPath $fixturePointer -Encoding utf8
    $pointerHash = (Get-FileHash -LiteralPath $fixturePointer).Hash
    & $fixtureLauncher | Out-Null
    Test-LauncherCondition ($launcherCalls.Count -eq 2 -and $launcherCalls[1].FilePath -eq $fixtureExecutable -and !$launcherCalls[1].Wait -and !$launcherCalls[1].Arguments) 'ordinary launch still resolves the ready pointer without a hidden startup override'
    Test-LauncherCondition ((Get-FileHash -LiteralPath $fixturePointer).Hash -eq $pointerHash) 'ordinary launch never modifies the fixture pointer'
    [IO.File]::WriteAllText($fixturePointer, 'deliberately invalid pointer JSON')
    & $fixtureLauncher -TestMonitor NE18NZ2 -TestBuildDirectory $fixtureCandidate -Report $fixtureReport | Out-Null
    Test-LauncherCondition ($launcherCalls.Count -eq 3 -and $launcherCalls[2].FilePath -eq $fixtureExecutable) 'explicit override does not read a malformed irrelevant pointer'
} catch {
    $launcherResults.Add('FAIL launcher: ' + $_.Exception.Message)
    throw
} finally {
    $launcherResults.Add('Scope: isolated temporary copy; Start-Process mocked; no GUI/process launch; actual visibility remains unverified.')
    if ($Report) {
        $reportFullPath = [IO.Path]::GetFullPath($Report)
        New-Item -ItemType Directory -Path (Split-Path $reportFullPath -Parent) -Force | Out-Null
        [IO.File]::WriteAllLines($reportFullPath, $launcherResults)
    }
    $launcherResults | Write-Output
    if (Test-Path -LiteralPath $launcherFixtureRoot) {
        $resolvedFixture = [IO.Path]::GetFullPath((Get-Item -LiteralPath $launcherFixtureRoot).FullName)
        if (!$resolvedFixture.StartsWith($launcherTempBase,[StringComparison]::OrdinalIgnoreCase) -or $resolvedFixture -ne $launcherFixtureRoot) { throw 'Unsafe fixture cleanup path.' }
        Remove-Item -LiteralPath $resolvedFixture -Recurse -Force
    }
}
