<#
.SYNOPSIS
    Builds the test ROM and runs the automated scenarios in mGBA (two boots: the second one checks
    that the high score survived in SRAM).

.EXAMPLE
    .\tests\run_tests.ps1            # build + run
    .\tests\run_tests.ps1 -NoBuild   # run the existing spaceshooter_test.gba

.NOTES
    Needs an mGBA build with --script support (0.11 / nightly) in tools\emulator\ or $env:MGBA.
    A small emulator window opens while the tests run (uncapped speed, about 20 seconds).
#>
param(
    [switch]$NoBuild,
    [int]$TimeoutSec = 600
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent

if (-not $NoBuild) {
    & (Join-Path $root 'build.ps1') -TestBuild
}

$rom = Join-Path $root 'spaceshooter_test.gba'
$elf = Join-Path $root 'spaceshooter_test.elf'
$sav = Join-Path $root 'spaceshooter_test.sav'
if (-not (Test-Path $rom)) { throw "Test ROM not found: $rom" }

$mgba = @($env:MGBA, (Join-Path $root 'tools\emulator\mGBA.exe')) | Where-Object { $_ -and (Test-Path $_) } | Select-Object -First 1
if (-not $mgba) { throw 'mGBA (with --script support) not found. See README.md > Testing.' }

$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$symbol = & $nm $elf | Select-String ' ss_test_io$'
if (-not $symbol) { throw 'ss_test_io symbol not found in the ELF.' }
$address = '0x' + ($symbol.Line -split ' ')[0]

$out = Join-Path $PSScriptRoot 'out'
New-Item -ItemType Directory -Force $out | Out-Null
Get-ChildItem $out -File | Remove-Item
Remove-Item -ErrorAction SilentlyContinue $sav        # fresh save: the first boot starts from the default hi-score

$failed = $false
foreach ($boot in 1, 2) {
    $result = Join-Path $out "results_boot$boot.txt"
    $wrapper = Join-Path $out "launch_boot$boot.lua"
    @"
TEST_IO = $address
OUT_DIR = "$($out -replace '\\', '/')"
RESULT_FILE = "$($result -replace '\\', '/')"
dofile("$(($PSScriptRoot -replace '\\', '/'))/launcher.lua")
"@ | Set-Content -Encoding ascii $wrapper

    Write-Host "Boot $boot..."
    $proc = Start-Process -FilePath $mgba -ArgumentList @('--script', "`"$wrapper`"", '-C', 'videoSync=0', '-C', 'audioSync=0', "`"$rom`"") -PassThru
    if (-not $proc.WaitForExit($TimeoutSec * 1000)) {
        Stop-Process -Id $proc.Id -Force
        throw "Boot $boot timed out after $TimeoutSec s."
    }
    if (-not (Test-Path $result)) { throw "Boot $boot produced no result file." }

    Get-Content $result | ForEach-Object {
        if ($_ -match '^\[FAIL\]|RESULT: FAIL') { Write-Host $_ -ForegroundColor Red }
        elseif ($_ -match '^===|^SUMMARY|^TOTAL|RESULT: PASS') { Write-Host $_ -ForegroundColor Cyan }
        else { Write-Host $_ }
    }
    if (-not (Select-String -Path $result -Pattern 'RESULT: PASS' -Quiet)) { $failed = $true }
}

Write-Host ''
if ($failed) { Write-Host 'TESTS FAILED' -ForegroundColor Red; exit 1 }
Write-Host 'ALL TESTS PASSED' -ForegroundColor Green
