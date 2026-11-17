#!/usr/bin/env bash
# Original AI-assisted (Claude), LGPL-2.1-or-later. Own MEASURE program only.
# Builds the AF_UNIX MEASURE bundle (docs/wine-repro/ws2_32-afunix/README.md):
# x86_64 and i686 static executables, the source, a run-measure.bat that
# never overwrites results, the license and SHA256SUMS.
set -euo pipefail
umask 022
[[ $# == 1 ]] || { echo 'Usage: build-measure-ws2_32-afunix.sh NEW_OUTPUT' >&2; exit 2; }
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
output="$(realpath -m -- "$1")"
src="$here/docs/wine-repro/ws2_32-afunix"
[[ ! -e "$output" && ! -L "$output" ]] || { echo 'Use a new output directory' >&2; exit 2; }
for arch in x86_64 i686; do
    command -v "$arch-w64-mingw32-gcc" >/dev/null || { echo "Missing $arch-w64-mingw32-gcc" >&2; exit 2; }
done
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
for arch in x86_64 i686; do
    "$arch-w64-mingw32-gcc" -O2 -static -Wall -Wextra -Werror "$src/measure.c" \
        -o "$work/afunix-measure-$arch.exe" -lws2_32 -lmswsock
    "$arch-w64-mingw32-gcc" -O2 -static -Wall -Wextra -Werror "$src/measure-sd.c" \
        -o "$work/afunix-measure-sd-$arch.exe" -lws2_32 -ladvapi32
    "$arch-w64-mingw32-gcc" -O2 -static -Wall -Wextra -Werror "$src/measure-del.c" \
        -o "$work/afunix-measure-del-$arch.exe" -lws2_32
done
mkdir -p -- "$output"
cp "$work/"*.exe "$src/measure.c" "$src/measure-sd.c" "$src/measure-del.c" "$src/README.md" "$here/LICENSE" "$output/"
printf '%s\r\n' '@echo off' 'setlocal DisableDelayedExpansion' 'cd /d "%~dp0"' \
    'if exist exitcodes.txt exit /b 2' 'if exist result-x86_64.txt exit /b 2' 'if exist result-i686.txt exit /b 2' \
    'if exist result-sd-x86_64.txt exit /b 2' 'if exist result-sd-i686.txt exit /b 2' \
    'if exist result-del-x86_64.txt exit /b 2' 'if exist result-del-i686.txt exit /b 2' \
    'set "failed=0"' \
    'afunix-measure-x86_64.exe > result-x86_64.txt 2>&1' 'set "code=%ERRORLEVEL%"' \
    '>>exitcodes.txt echo x86_64=%code%' 'if not "%code%"=="0" set "failed=1"' \
    'afunix-measure-i686.exe > result-i686.txt 2>&1' 'set "code=%ERRORLEVEL%"' \
    '>>exitcodes.txt echo i686=%code%' 'if not "%code%"=="0" set "failed=1"' \
    'afunix-measure-sd-x86_64.exe > result-sd-x86_64.txt 2>&1' 'set "code=%ERRORLEVEL%"' \
    '>>exitcodes.txt echo sd-x86_64=%code%' 'if not "%code%"=="0" set "failed=1"' \
    'afunix-measure-sd-i686.exe > result-sd-i686.txt 2>&1' 'set "code=%ERRORLEVEL%"' \
    '>>exitcodes.txt echo sd-i686=%code%' 'if not "%code%"=="0" set "failed=1"' \
    'afunix-measure-del-x86_64.exe > result-del-x86_64.txt 2>&1' 'set "code=%ERRORLEVEL%"' \
    '>>exitcodes.txt echo del-x86_64=%code%' 'if not "%code%"=="0" set "failed=1"' \
    'afunix-measure-del-i686.exe > result-del-i686.txt 2>&1' 'set "code=%ERRORLEVEL%"' \
    '>>exitcodes.txt echo del-i686=%code%' 'if not "%code%"=="0" set "failed=1"' \
    'pause' 'exit /b %failed%' > "$output/run-measure.bat"
(cd "$output" && sha256sum ./*.exe measure.c measure-sd.c measure-del.c README.md LICENSE run-measure.bat > SHA256SUMS)
echo "MEASURE bundle: $output"
