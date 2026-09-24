<#
.SYNOPSIS
    Builds Space Shooter (spaceshooter.gba) on Windows with devkitPro's MSYS2 environment.

.EXAMPLE
    .\build.ps1              # release ROM -> spaceshooter.gba
    .\build.ps1 -DebugBuild  # debug ROM   -> spaceshooter_debug.gba
    .\build.ps1 -TestBuild   # test ROM    -> spaceshooter_test.gba (release code + automated test scenarios)
    .\build.ps1 -Clean       # remove build outputs (combine with -DebugBuild / -TestBuild)
    .\build.ps1 -Run         # build, then open the ROM in mGBA (if found)

.NOTES
    Needs devkitPro (GBA Development) and the host gcc in devkitPro's MSYS2 (pacman -S gcc), which
    compiles the asset generator. devkitPro makefiles break on paths containing spaces, so this script
    maps the project folder to a free drive letter with 'subst' for the duration of the build.
#>
param(
    [switch]$DebugBuild,
    [switch]$TestBuild,
    [switch]$Clean,
    [switch]$Run,
    [int]$Jobs = 0
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot

# --- devkitPro ---------------------------------------------------------------------------------------------------
$dkp = if ($env:DEVKITPRO_WIN) { $env:DEVKITPRO_WIN } else { 'C:\devkitPro' }
$bash = Join-Path $dkp 'msys2\usr\bin\bash.exe'
if (-not (Test-Path $bash)) {
    throw "devkitPro MSYS2 not found at '$dkp'. Install devkitPro (GBA Development) - see README.md."
}
if (-not (Test-Path (Join-Path $dkp 'devkitARM\bin\arm-none-eabi-gcc.exe'))) {
    throw "devkitARM not found. Run: pacman -S gba-dev (from the devkitPro MSYS2 shell)."
}
if (-not (Test-Path (Join-Path $dkp 'msys2\usr\bin\gcc.exe'))) {
    throw "Host gcc not found in devkitPro MSYS2. Run: pacman -S gcc (from the devkitPro MSYS2 shell)."
}

# --- Map project to a drive letter without spaces ---------------------------------------------------------------
$used = (Get-PSDrive -PSProvider FileSystem).Name
$letter = @('S','T','U','V','W','X','Y','Z','R','Q','P','O','N','M') | Where-Object { $used -notcontains $_ } | Select-Object -First 1
if (-not $letter) { throw 'No free drive letter available for subst.' }
& subst "${letter}:" "$root" | Out-Null
if ($LASTEXITCODE -ne 0) { throw "subst ${letter}: failed" }

try {
    if ($Jobs -le 0) { $Jobs = [Environment]::ProcessorCount }
    $makeArgs = "-j$Jobs"
    if ($DebugBuild) { $makeArgs += ' DEBUG=1' }
    elseif ($TestBuild) { $makeArgs += ' TESTS=1' }
    $cmd = if ($Clean) { "make clean $makeArgs" } else { "make $makeArgs" }
    $drive = $letter.ToLower()

    $env:MSYSTEM = 'MSYS'
    $env:CHERE_INVOKING = '1'
    & $bash --login -c "cd /$drive/ && $cmd"
    $code = $LASTEXITCODE
} finally {
    & subst "${letter}:" /D | Out-Null
}

if ($code -ne 0) { throw "Build failed (exit code $code)." }
if ($Clean) { Write-Host 'Clean done.'; exit 0 }

$romName = if ($DebugBuild) { 'spaceshooter_debug.gba' } elseif ($TestBuild) { 'spaceshooter_test.gba' } else { 'spaceshooter.gba' }
$rom = Join-Path $root $romName
if (-not (Test-Path $rom)) { throw "Build reported success but $rom is missing." }
$size = (Get-Item $rom).Length
Write-Host ("ROM: {0} ({1:N0} bytes)" -f $rom, $size)

if ($Run) {
    $mgba = @(
        $env:MGBA,
        (Join-Path $root 'tools\emulator\mGBA.exe'),
        'C:\Program Files\mGBA\mGBA.exe'
    ) | Where-Object { $_ -and (Test-Path $_) } | Select-Object -First 1
    if ($mgba) { Start-Process $mgba -ArgumentList "`"$rom`"" } else { Write-Warning 'mGBA not found; set $env:MGBA to mGBA.exe.' }
}
