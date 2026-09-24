<#
.SYNOPSIS
    Runs Space Shooter's automated emulator tests (mGBA Lua scripts).

.EXAMPLE
    .\tests\run_tests.ps1                         # smoke test on the release ROM
    .\tests\run_tests.ps1 -Test full_run -DebugRom
    .\tests\run_tests.ps1 -Test smoke -Fast       # run uncapped (no video/audio sync)

.NOTES
    Needs an mGBA build with the --script option (0.11 / nightly). Put it in tools\emulator\ or set $env:MGBA.
    A small emulator window opens while the test runs.
#>
param(
    [string]$Test = 'smoke',
    [ValidateSet('release', 'debug', 'profile')]
    [string]$Build = 'release',
    [switch]$DebugRom,
    [switch]$Fast,
    [int]$TimeoutSec = 900
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
if ($DebugRom) { $Build = 'debug' }
$romName = switch ($Build) { 'debug' { 'spaceshooter_debug' } 'profile' { 'spaceshooter_profile' } default { 'spaceshooter' } }
$rom = Join-Path $root "$romName.gba"
$elf = Join-Path $root "$romName.elf"
if (-not (Test-Path $rom)) { throw "ROM not found: $rom (build it first)" }
if (-not (Test-Path $elf)) { throw "ELF not found: $elf (needed for the telemetry address)" }

$mgba = @($env:MGBA, (Join-Path $root 'tools\emulator\mGBA.exe')) | Where-Object { $_ -and (Test-Path $_) } | Select-Object -First 1
if (-not $mgba) { throw 'mGBA (with --script support) not found. See README.md > Testing.' }

$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
if ($env:DEVKITARM_WIN) { $nm = Join-Path $env:DEVKITARM_WIN 'bin\arm-none-eabi-nm.exe' }
$symbol = & $nm $elf | Select-String ' ss_telemetry$'
if (-not $symbol) { throw 'ss_telemetry symbol not found in ELF.' }
$address = '0x' + ($symbol.Line -split ' ')[0]

$out = Join-Path $PSScriptRoot 'out'
New-Item -ItemType Directory -Force $out | Out-Null
$outLua = ($out -replace '\\', '/')
$testsLua = ($PSScriptRoot -replace '\\', '/')
Remove-Item -ErrorAction SilentlyContinue (Join-Path $out "${Test}_result.txt")

$wrapper = Join-Path $out "run_$Test.lua"
@"
TELEMETRY_ADDR = $address
OUT_DIR = "$outLua"
TEST_NAME = "$Test"
dofile("$testsLua/lib/harness.lua")
dofile("$testsLua/$Test.lua")
"@ | Set-Content -Encoding ascii $wrapper

$mgbaArgs = @('--script', "`"$wrapper`"")
if ($Fast) { $mgbaArgs += @('-C', 'videoSync=0', '-C', 'audioSync=0') }
$mgbaArgs += "`"$rom`""

Write-Host "Running '$Test' on $romName.gba (telemetry at $address)..."
$sw = [Diagnostics.Stopwatch]::StartNew()
$proc = Start-Process -FilePath $mgba -ArgumentList $mgbaArgs -PassThru
if (-not $proc.WaitForExit($TimeoutSec * 1000)) {
    Stop-Process -Id $proc.Id -Force
    throw "Test '$Test' timed out after $TimeoutSec s."
}
$sw.Stop()

$resultFile = Join-Path $out "${Test}_result.txt"
if (-not (Test-Path $resultFile)) { throw "No result file produced ($resultFile)." }
Get-Content $resultFile
Write-Host ("Elapsed: {0:N1} s" -f $sw.Elapsed.TotalSeconds)
if ((Get-Content $resultFile -Tail 1) -notmatch ', 0 failed') { exit 1 }
