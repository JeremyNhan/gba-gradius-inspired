<#
.SYNOPSIS
    Builds the release, debug and profile ROMs and runs every automated emulator test.

.EXAMPLE
    .\tests\run_all.ps1          # build + test
    .\tests\run_all.ps1 -NoBuild # test existing ROMs
#>
param([switch]$NoBuild)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$build = Join-Path $root 'build.ps1'
$run = Join-Path $PSScriptRoot 'run_tests.ps1'

if (-not $NoBuild) {
    & $build
    & $build -DebugBuild
    & $build -ProfileBuild
}

# The save test needs a fresh save file so the hi-score it checks comes from this run.
Remove-Item -ErrorAction SilentlyContinue (Join-Path $root 'spaceshooter_profile.sav')

$suites = @(
    @{ Test = 'smoke'; Build = 'release' },
    @{ Test = 'death'; Build = 'debug' },
    @{ Test = 'full_run'; Build = 'debug' },
    @{ Test = 'full_run'; Build = 'profile' },
    @{ Test = 'save_check'; Build = 'profile' }
)

$failed = @()
foreach ($suite in $suites) {
    Write-Host ''
    Write-Host ("=== {0} ({1}) ===" -f $suite.Test, $suite.Build) -ForegroundColor Cyan
    & $run -Test $suite.Test -Build $suite.Build -Fast
    if ($LASTEXITCODE -ne 0) { $failed += "$($suite.Test) ($($suite.Build))" }
}

Write-Host ''
if ($failed.Count -eq 0) {
    Write-Host 'ALL TESTS PASSED' -ForegroundColor Green
} else {
    Write-Host ('FAILED: ' + ($failed -join ', ')) -ForegroundColor Red
    exit 1
}
