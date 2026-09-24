#!/usr/bin/env bash
# Build Space Shooter from a POSIX shell (Linux with devkitPro, or the devkitPro MSYS2 shell).
#   ./build.sh            -> spaceshooter.gba
#   ./build.sh DEBUG=1    -> spaceshooter_debug.gba
#   ./build.sh clean
set -euo pipefail
cd "$(dirname "$0")"

case "$PWD" in
    *" "*) echo "error: project path contains spaces ('$PWD'); move it or use build.ps1 on Windows." >&2; exit 1 ;;
esac

: "${DEVKITPRO:=/opt/devkitpro}"
: "${DEVKITARM:=$DEVKITPRO/devkitARM}"
export DEVKITPRO DEVKITARM

PY="${PYTHON:-$(command -v python3 || command -v python)}"
exec make -j"$(nproc 2>/dev/null || echo 4)" PYTHON="$PY" "$@"
