#!/usr/bin/env bash
# Original AI-assisted (Claude), LGPL-2.1-or-later.
# Builds the Windows MEASURE bundle docs/wine-repro/opengl32-pixel-format-alpha:
# our own measure.c for x86_64 and i686 (MinGW), a batch file and the license.
set -euo pipefail
umask 022
[[ $# == 1 ]] || { echo 'Usage: build-measure-opengl32-pixel-format.sh NEW_OUTPUT' >&2; exit 2; }
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
output="$(realpath -m -- "$1")"
src="$here/docs/wine-repro/opengl32-pixel-format-alpha/measure.c"
[[ ! -e "$output" && ! -L "$output" ]] || { echo 'Use a new output directory' >&2; exit 2; }
for arch in x86_64 i686; do
    command -v "$arch-w64-mingw32-gcc" >/dev/null || { echo "Missing $arch-w64-mingw32-gcc (pacman -S mingw-w64-gcc)" >&2; exit 2; }
done
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
for arch in x86_64 i686; do
    "$arch-w64-mingw32-gcc" -O2 -static -Wall -Werror "$src" -o "$work/pf-measure-$arch.exe" -lopengl32 -lgdi32 -luser32
done
mkdir -p -- "$output"
cp "$work/"*.exe "$src" "$here/LICENSE" "$output/"
printf '%s\r\n' '@echo off' 'cd /d "%~dp0"' \
    'pf-measure-x86_64.exe > result-x86_64.txt 2>&1' '>>exitcodes.txt echo x86_64=%ERRORLEVEL%' \
    'pf-measure-i686.exe > result-i686.txt 2>&1' '>>exitcodes.txt echo i686=%ERRORLEVEL%' \
    'pause' > "$output/run-measure.bat"
cat > "$output/README.txt" <<'INFO'
MEASURE for winum (opengl32 pixel format with alpha). Original AI-assisted
(Claude), LGPL-2.1-or-later. Only our own two EXEs, their source, the
license and a batch file.

Run on Windows 10 with the real GPU driver active (not the basic display
adapter). Copy the folder to a new directory, run run-measure.bat. Small
windows flash briefly. Return result-x86_64.txt, result-i686.txt and
exitcodes.txt.
INFO
(cd "$output" && sha256sum ./*.exe ./*.c LICENSE README.txt run-measure.bat > SHA256SUMS)
echo "MEASURE bundle: $output"
