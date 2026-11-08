#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
# Build the exact v3 candidate Wine test for the offline Windows 10 VM.
set -euo pipefail
umask 022
[[ $# == 1 ]] || { echo 'Usage: build-l1-typedesc-pointee.sh NEW_OUTPUT' >&2; exit 2; }
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
output="$(realpath -m -- "$1")"
[[ ! -e "$output" && ! -L "$output" ]] || { echo 'Use a new output directory' >&2; exit 2; }
revision=7298a818b79446bae153b8d5aa27235ab3703bf9
sum=9f516d63e9f814f4dad70ce0a1943211b8a6236008ac3025cf58004091c661db
cache="${XDG_CACHE_HOME:-$HOME/.cache}/winum/downloads"
archive="${WINE_L1_SOURCE_ARCHIVE:-$cache/wine-$revision.tar.gz}"
work="$(mktemp -d)"
partial=
trap 'rm -rf -- "$work"; if [[ -n "$partial" ]]; then rm -f -- "$partial"; fi' EXIT
if [[ ! -f "$archive" ]]; then
    [[ -z ${WINE_L1_SOURCE_ARCHIVE:-} ]] || { echo 'WINE_L1_SOURCE_ARCHIVE is missing' >&2; exit 2; }
    mkdir -p -- "$cache"
    partial="$(mktemp "$cache/.wine-l1.XXXXXXXX")"
    curl -fL --retry 3 -o "$partial" "https://codeload.github.com/wine-mirror/wine/tar.gz/$revision"
    printf '%s  %s\n' "$sum" "$partial" | sha256sum -c -
    mv -- "$partial" "$archive"
    partial=
fi
printf '%s  %s\n' "$sum" "$archive" | sha256sum -c -
mkdir -p "$work/include/wine"
tar -xzf "$archive" -C "$work" "wine-$revision/include/wine/test.h" "wine-$revision/include/wine/debug.h"
cp "$work/wine-$revision/include/wine/"{test,debug}.h "$work/include/wine/"
python3 - "$here/patches/candidates/oleaut32-typedesc-pointee-v3/oleaut32-typedesc-pointee.patch" "$work/pointee.c" <<'PY'
from pathlib import Path
import sys
patch=Path(sys.argv[1]).read_text()
added='\n'.join(l[1:] for l in patch.splitlines() if l.startswith('+') and not l.startswith('+++'))
start=added.index('/* BEGIN winum typedesc pointee test:')
end=added.index('/* END winum typedesc pointee test */',start)+len('/* END winum typedesc pointee test */')
head='''/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Exact candidate Wine test, standalone wrapper only. */
#define COBJMACROS
#include <windows.h>
#include <oleauto.h>
#include <stdio.h>
#include <string.h>
#include "wine/test.h"
const char * __cdecl __wine_dbg_strdup(const char *str) { return _strdup(str); }
'''
Path(sys.argv[2]).write_text(head+added[start:end]+'\nSTART_TEST(pointee)\n{\n    test_encoded_pointee();\n}\n')
PY
for arch in x86_64 i686; do
    "$arch-w64-mingw32-gcc" -std=c11 -DSTANDALONE -DDECLSPEC_EXPORT= \
        -O2 -static -Wall -Wextra -Werror -I "$work/include" "$work/pointee.c" \
        -o "$work/pointee-$arch.exe" -loleaut32 -lole32 -luuid
done
mkdir -p -- "$(dirname -- "$output")"
mkdir -- "$output"
cp "$work/"*.exe "$work/pointee.c" "$here/LICENSE" "$output/"
cp -a "$work/include" "$output/"
cp "$here/patches/candidates/oleaut32-typedesc-pointee-v3/README.md" "$output/README.md"
python3 - "$output" <<'PY'
from pathlib import Path
import sys
p=Path(sys.argv[1])
lines=['@echo off','setlocal DisableDelayedExpansion','cd /d "%~dp0"',
       'if exist exitcodes.txt exit /b 2','if exist result-x86_64.txt exit /b 2',
       'if exist result-i686.txt exit /b 2','set "failed=0"']
for arch in ('x86_64','i686'):
    lines += [f'pointee-{arch}.exe pointee > result-{arch}.txt 2>&1',
              'set "code=%ERRORLEVEL%"',f'>>exitcodes.txt echo {arch}=%code%',
              'if not "%code%"=="0" set "failed=1"']
lines += ['pause','exit /b %failed%']
(p/'run-tests.bat').write_bytes(('\r\n'.join(lines)+'\r\n').encode('ascii'))
PY
(cd "$output" && sha256sum ./*.exe pointee.c include/wine/*.h README.md LICENSE run-tests.bat > SHA256SUMS)
echo "Typedesc pointee L1 bundle: $output"
