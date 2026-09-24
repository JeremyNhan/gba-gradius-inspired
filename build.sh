#!/usr/bin/env bash
# Build Space Shooter from a POSIX shell (Linux with devkitPro, or the devkitPro MSYS2 shell).
#   ./build.sh            -> spaceshooter.gba
#   ./build.sh DEBUG=1    -> spaceshooter_debug.gba
#   ./build.sh TESTS=1    -> spaceshooter_test.gba
#   ./build.sh clean
# Needs devkitARM (gba-dev) and a host C compiler (gcc or cc) for the asset generator.
set -euo pipefail
cd "$(dirname "$0")"

case "$PWD" in
    *" "*) echo "error: project path contains spaces ('$PWD'); move it or use build.ps1 on Windows." >&2; exit 1 ;;
esac

: "${DEVKITPRO:=/opt/devkitpro}"
: "${DEVKITARM:=$DEVKITPRO/devkitARM}"
export DEVKITPRO DEVKITARM

HOSTCC="${HOSTCC:-$(command -v gcc || command -v cc)}"
exec make -j"$(nproc 2>/dev/null || echo 4)" HOSTCC="$HOSTCC" "$@"
