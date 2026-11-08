#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
set -euo pipefail
umask 022
[[ $# == 1 ]] || { echo 'Usage: build-measure-batch2.sh NEW_OUTPUT' >&2; exit 2; }
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
output="$(realpath -m -- "$1")"
[[ ! -e "$output" && ! -L "$output" ]] || { echo 'Use a new output directory' >&2; exit 2; }
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
for arch in x86_64 i686; do
    "$arch-w64-mingw32-gcc" -std=c11 -O2 -static -Wall -Wextra -Werror \
        "$here/docs/wine-repro/batch2-measure/typedesc.c" \
        -o "$work/typedesc-$arch.exe" -loleaut32 -lole32 -luuid
done
mkdir -p -- "$(dirname -- "$output")"
mkdir -- "$output"
cp -- "$work/"*.exe "$here/docs/wine-repro/batch2-measure/typedesc.c" \
    "$here/docs/wine-repro/batch2-measure/README.md" "$here/LICENSE" "$output/"
python3 - "$output" <<'PY'
from pathlib import Path
import sys
p=Path(sys.argv[1])
lines=['@echo off','setlocal DisableDelayedExpansion','cd /d "%~dp0"',
       'if exist exitcodes.txt exit /b 2',
       'if exist result-typedesc-x86_64.txt exit /b 2',
       'if exist result-typedesc-i686.txt exit /b 2',
       'if exist result-reader-x86_64.txt exit /b 2',
       'if exist result-reader-i686.txt exit /b 2', 'set "failed=0"']
for arch in ('x86_64','i686'):
    lines += [f'typedesc-{arch}.exe > result-typedesc-{arch}.txt 2>&1',
              'set "code=%ERRORLEVEL%"',f'>>exitcodes.txt echo typedesc-{arch}=%code%',
              'if not "%code%"=="0" set "failed=1"']
for arch in ('x86_64','i686'):
    lines += [f'typedesc-{arch}.exe --reader-only > result-reader-{arch}.txt 2>&1',
              'set "code=%ERRORLEVEL%"',f'>>exitcodes.txt echo reader-{arch}=%code%',
              'if not "%code%"=="0" set "failed=1"']
lines += ['pause','exit /b %failed%']
(p/'run-measure.bat').write_bytes(('\r\n'.join(lines)+'\r\n').encode('ascii'))
PY
(cd "$output" && sha256sum ./*.exe typedesc.c README.md LICENSE run-measure.bat > SHA256SUMS)
echo "Batch 2 initial MEASURE bundle: $output"
