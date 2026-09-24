<#
.SYNOPSIS
    Builds Space Shooter (spaceshooter.gba) on Windows with devkitPro's MSYS2 environment.

.EXAMPLE
    .\build.ps1              # release ROM  -> spaceshooter.gba
    .\build.ps1 -DebugBuild  # debug ROM    -> spaceshooter_debug.gba
    .\build.ps1 -ProfileBuild # release code + test hooks -> spaceshooter_profile.gba
    .\build.ps1 -Clean       # remove build outputs
    .\build.ps1 -Run         # build, then open the ROM in mGBA (if found)

.NOTES
    devkitARM/Butano makefiles break on paths containing spaces. This script maps the project folder
    to a free drive letter with 'subst' for the duration of the build, so the repository can live anywhere.
#>
param(
    [switch]$DebugBuild,
    [switch]$ProfileBuild,
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
if (-not (Test-Path (Join-Path $dkp 'devkitARM\bin\arm-none-eabi-g++.exe'))) {
    throw "devkitARM not found. Run: pacman -S gba-dev (from the devkitPro MSYS2 shell)."
}
if (-not (Test-Path (Join-Path $root 'external\butano\butano\butano.mak'))) {
    throw "Butano submodule missing. Run: git submodule update --init"
}

# --- Python (the Windows Store 'python' alias is a stub, so resolve the real interpreter) -------------------------
$python = $env:PYTHON
if (-not $python) {
    foreach ($candidate in @('py -3', 'python3', 'python')) {
        try {
            $exe = & ([scriptblock]::Create("$candidate -c `"import sys; print(sys.executable)`"")) 2>$null
            if ($LASTEXITCODE -eq 0 -and $exe -and (Test-Path $exe)) { $python = $exe.Trim(); break }
        } catch { }
    }
}
if (-not $python) { throw 'Python 3 not found. Install it from python.org (tick "Add to PATH").' }
if ($python -match ' ') {
    # make cannot quote the interpreter path; use the 8.3 short path instead.
    $python = (& cmd /c "for %I in (`"$python`") do @echo %~sI").Trim()
}
$pythonUnix = $python -replace '\\', '/'

# --- Map project to a drive letter without spaces ---------------------------------------------------------------
$used = (Get-PSDrive -PSProvider FileSystem).Name
$letter = @('S','T','U','V','W','X','Y','Z','R','Q','P','O','N','M') | Where-Object { $used -notcontains $_ } | Select-Object -First 1
if (-not $letter) { throw 'No free drive letter available for subst.' }
& subst "${letter}:" "$root" | Out-Null
if ($LASTEXITCODE -ne 0) { throw "subst ${letter}: failed" }

try {
    if ($Jobs -le 0) { $Jobs = [Environment]::ProcessorCount }
    $makeArgs = "-j$Jobs PYTHON=$pythonUnix"
    if ($DebugBuild) { $makeArgs += ' DEBUG=1' }
    elseif ($ProfileBuild) { $makeArgs += ' PROFILE=1' }
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

$romName = if ($DebugBuild) { 'spaceshooter_debug.gba' } elseif ($ProfileBuild) { 'spaceshooter_profile.gba' } else { 'spaceshooter.gba' }
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
