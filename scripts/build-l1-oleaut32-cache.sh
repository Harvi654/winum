#!/usr/bin/env bash
# Original AI-assisted (Claude), LGPL-2.1-or-later.
# Builds the Windows 10 L1 bundle for patches/candidates/oleaut32-typelib-cache:
# the Wine test test_LoadTypeLib_released, taken unchanged from the candidate
# patch, as standalone x86_64 and i686 executables (MinGW), plus a batch file.
# Only our own EXEs, their source and the license go into the bundle.
set -euo pipefail
umask 022
[[ $# == 2 ]] || { echo 'Usage: build-l1-oleaut32-cache.sh WINE_SOURCE NEW_OUTPUT' >&2; exit 2; }
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
source="$(realpath -e -- "$1")"
output="$(realpath -m -- "$2")"
patch="$here/patches/oleaut32-typelib-keep-released.patch"
if [[ ! -f "$source/include/wine/test.h" ]]; then
    # A source archive has the tree one or two levels down.
    found="$(find "$source" -maxdepth 6 -path '*/include/wine/test.h' -print -quit)"
    [[ -n "$found" ]] || { echo "No include/wine/test.h in $source" >&2; exit 2; }
    source="${found%/include/wine/test.h}"
fi
[[ ! -e "$output" && ! -L "$output" ]] || { echo 'Use a new output directory' >&2; exit 2; }
for arch in x86_64 i686; do
    command -v "$arch-w64-mingw32-gcc" >/dev/null || { echo "Missing $arch-w64-mingw32-gcc (pacman -S mingw-w64-gcc)" >&2; exit 2; }
done
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
mkdir -p "$work/include/wine"
cp "$source/include/wine/"{test,debug}.h "$work/include/wine/"

# The two test functions exactly as the patch adds them to tests/typelib.c.
python3 - "$patch" "$work/typelib_released.c" <<'PY'
import sys
lines = open(sys.argv[1], encoding='utf-8').read().splitlines()
start = lines.index('+++ b/dlls/oleaut32/tests/typelib.c')
added = []
for line in lines[start + 1:]:
    if line.startswith('diff --git'):
        break
    if line.startswith('+'):
        added.append(line[1:])
first = added.index('static void write_reload_typelib(const WCHAR *filename, const GUID *guid, int count)')
body_start = added.index('static void test_LoadTypeLib_released(void)')
end = added.index('}', body_start)
body = '\n'.join(added[first:end + 1])
head = '''/* Standalone build of test_LoadTypeLib_released from the winum candidate
 * oleaut32-typelib-cache (dlls/oleaut32/tests/typelib.c). Original
 * AI-assisted (Claude), LGPL-2.1-or-later. */
#define COBJMACROS
#include <stdio.h>
#include <windows.h>
#include <ole2.h>
#include <oleauto.h>
#include "wine/test.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

/* wine_dbgstr_w() needs this; Wine's own test runner provides it. The
 * strings are only printed, a few bytes are left allocated. */
const char * __cdecl __wine_dbg_strdup(const char *str)
{
    return _strdup(str);
}

'''
tail = '''

START_TEST(typelib_released)
{
    CoInitialize(NULL);
    test_LoadTypeLib_released();
    CoUninitialize();
}
'''
with open(sys.argv[2], 'x', encoding='utf-8') as f:
    f.write(head + body + tail)
PY

for arch in x86_64 i686; do
    "$arch-w64-mingw32-gcc" -DSTANDALONE -DDECLSPEC_EXPORT= -O2 -static -Wall -Werror \
        -I"$work/include" "$work/typelib_released.c" \
        -o "$work/oleaut32-cache-$arch.exe" -loleaut32 -lole32 -luuid
done

mkdir -p -- "$output"
cp "$work/"*.exe "$work/typelib_released.c" "$here/LICENSE" "$output/"
python3 - "$output" <<'PY'
from pathlib import Path
import sys
p = Path(sys.argv[1])
lines = ['@echo off', 'setlocal DisableDelayedExpansion', 'cd /d "%~dp0"',
         'if exist exitcodes.txt exit /b 2', 'set "failed=0"']
for a in ['x86_64', 'i686']:
    lines += [f'if exist result-{a}.txt exit /b 2',
              f'oleaut32-cache-{a}.exe typelib_released > result-{a}.txt 2>&1',
              'set "code=%ERRORLEVEL%"', f'>>exitcodes.txt echo {a}=%code%',
              'if not "%code%"=="0" set "failed=1"']
lines += ['pause', 'exit /b %failed%']
(p / 'run-tests.bat').write_bytes(('\r\n'.join(lines) + '\r\n').encode('ascii'))
PY
cat > "$output/README.txt" <<'INFO'
L1 for the winum candidate oleaut32-typelib-cache. Original AI-assisted
(Claude), LGPL-2.1-or-later. Contains only our two EXEs, their source, the
license and a batch file. No Wine or Microsoft DLLs.

Copy the whole folder to a NEW directory on Windows 10 (a normal user is
enough) and run run-tests.bat. It writes and deletes a few small .tlb files
in that folder. Return only result-x86_64.txt, result-i686.txt and
exitcodes.txt.
INFO
(cd "$output" && sha256sum ./*.exe ./*.c LICENSE README.txt run-tests.bat > SHA256SUMS)
echo "L1 bundle: $output"
