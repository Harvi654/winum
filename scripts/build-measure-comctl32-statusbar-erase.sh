#!/usr/bin/env bash
# Original AI-assisted (Codex), LGPL-2.1-or-later. Own MEASURE programs only.
set -euo pipefail
umask 022
[[ $# == 1 ]] || { echo 'Usage: build-measure-comctl32-statusbar-erase.sh NEW_OUTPUT' >&2; exit 2; }
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
output="$(realpath -m -- "$1")"
src="$here/docs/wine-repro/comctl32-statusbar-erase"
[[ ! -e "$output" && ! -L "$output" ]] || { echo 'Use a new output directory' >&2; exit 2; }
for arch in x86_64 i686; do
    for tool in gcc windres; do
        command -v "$arch-w64-mingw32-$tool" >/dev/null || { echo "Missing $arch-w64-mingw32-$tool" >&2; exit 2; }
    done
done
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
cp "$src/"*.manifest "$work/"
for arch in x86_64 i686; do
    for version in 5 6; do
        printf '1 24 "statusbar-v%s.manifest"\n' "$version" > "$work/v$version.rc"
        "$arch-w64-mingw32-windres" -I "$work" "$work/v$version.rc" -O coff -o "$work/manifest-$arch-v$version.o"
        "$arch-w64-mingw32-gcc" -DSTATUSBAR_VERSION="$version" -O2 -static -Wall -Wextra -Werror \
            "$src/measure.c" "$work/manifest-$arch-v$version.o" \
            -o "$work/statusbar-erase-$arch-v$version.exe" -lcomctl32 -luxtheme -lgdi32 -luser32
    done
done
mkdir -p -- "$output"
cp "$work/"*.exe "$src/measure.c" "$src/"*.manifest "$src/README.md" "$here/LICENSE" "$output/"
python3 - "$output" <<'PY'
from pathlib import Path
import sys
p = Path(sys.argv[1])
cases = [f'{arch}-v{version}' for arch in ('x86_64', 'i686') for version in (5, 6)]
lines = ['@echo off', 'setlocal DisableDelayedExpansion', 'cd /d "%~dp0"',
         'if exist exitcodes.txt exit /b 2']
lines += [f'if exist result-{case}.txt exit /b 2' for case in cases]
lines += ['set "failed=0"']
for case in cases:
    lines += [f'statusbar-erase-{case}.exe > result-{case}.txt 2>&1',
              'set "code=%ERRORLEVEL%"', f'>>exitcodes.txt echo {case}=%code%',
              'if not "%code%"=="0" set "failed=1"']
lines += ['pause', 'exit /b %failed%']
(p/'run-measure.bat').write_bytes(('\r\n'.join(lines)+'\r\n').encode('ascii'))
PY
(cd "$output" && sha256sum ./*.exe measure.c ./*.manifest README.md LICENSE run-measure.bat > SHA256SUMS)
echo "MEASURE bundle: $output"
