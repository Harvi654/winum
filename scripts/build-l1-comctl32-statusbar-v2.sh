#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
# The exact Windows-derived test block from candidate 0001, both architectures/v5/v6.
set -euo pipefail
umask 022
[[ $# == 2 ]] || { echo 'Usage: build-l1-comctl32-statusbar-v2.sh WINE_SOURCE NEW_OUTPUT' >&2; exit 2; }
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
source="$(realpath -e -- "$1")"
output="$(realpath -m -- "$2")"
[[ ! -e "$2" && ! -L "$2" && ! -e "$output" && ! -L "$output" ]] || { echo 'Use a new output directory' >&2; exit 2; }
[[ -f "$source/include/wine/test.h" && -f "$source/include/wine/debug.h" ]] || { echo 'WINE_SOURCE must contain include/wine/test.h and debug.h' >&2; exit 2; }
for arch in x86_64 i686; do
    for tool in gcc windres; do
        command -v "$arch-w64-mingw32-$tool" >/dev/null || { echo "Missing $arch-w64-mingw32-$tool" >&2; exit 2; }
    done
done
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
mkdir -p "$work/include/wine"
cp "$source/include/wine/"{test,debug}.h "$work/include/wine/"
cp "$here/docs/wine-repro/comctl32-statusbar-erase/"*.manifest "$work/"
python3 - "$here/patches/candidates/comctl32-statusbar-v2/0001-status-background-tests.patch" "$work/status-background.c" <<'PY'
from pathlib import Path
import sys
lines=Path(sys.argv[1]).read_text().splitlines()
added='\n'.join(s[1:] for s in lines if s.startswith('+') and not s.startswith('+++'))
start=added.index('/* BEGIN status background tests:')
end=added.index('/* END status background tests */',start)+len('/* END status background tests */')
head='''/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Standalone wrapper only; test block copied unchanged from the Wine test patch. */
#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <shlwapi.h>
#include "wine/test.h"
const char * __cdecl __wine_dbg_strdup(const char *str) { return _strdup(str); }
'''
tail='''
START_TEST(status_background)
{
    INITCOMMONCONTROLSEX controls = {sizeof(controls), ICC_BAR_CLASSES};
    BOOL ret = InitCommonControlsEx(&controls);
    ok(ret, "InitCommonControlsEx failed\\n");
    if (ret) test_status_background(TARGET_VERSION == 6);
}
'''
Path(sys.argv[2]).write_text(head+added[start:end]+tail)
PY
for arch in x86_64 i686; do
    for version in 5 6; do
        printf '1 24 "statusbar-v%s.manifest"\n' "$version" > "$work/manifest.rc"
        "$arch-w64-mingw32-windres" -I "$work" "$work/manifest.rc" -O coff -o "$work/manifest.o"
        "$arch-w64-mingw32-gcc" -std=c11 -DSTANDALONE -DDECLSPEC_EXPORT= -DTARGET_VERSION="$version" \
            -O2 -static -Wall -Wextra -Werror -I "$work/include" "$work/status-background.c" "$work/manifest.o" \
            -o "$work/status-background-$arch-v$version.exe" -lcomctl32 -luxtheme -lgdi32 -luser32
    done
done
mkdir -p -- "$(dirname -- "$output")"
mkdir -- "$output"
cp "$work/"*.exe "$work/status-background.c" "$work/"*.manifest "$here/LICENSE" "$output/"
python3 - "$output" <<'PY'
from pathlib import Path
import sys
out=Path(sys.argv[1]); cases=[f'{arch}-v{v}' for arch in ('x86_64','i686') for v in (5,6)]
lines=['@echo off','setlocal DisableDelayedExpansion','cd /d "%~dp0"','if exist exitcodes.txt exit /b 2']
lines += [f'if exist result-{case}.txt exit /b 2' for case in cases]
lines += ['set "failed=0"']
for case in cases:
    lines += [f'status-background-{case}.exe status_background > result-{case}.txt 2>&1',
              'set "code=%ERRORLEVEL%"',f'>>exitcodes.txt echo {case}=%code%',
              'if not "%code%"=="0" set "failed=1"']
lines += ['pause','exit /b %failed%']
(out/'run-tests.bat').write_bytes(('\r\n'.join(lines)+'\r\n').encode('ascii'))
PY
cat > "$output/README.txt" <<'INFO'
L1: comctl32 status background v2. Original, AI-assisted (Codex), LGPL-2.1-or-later.
Run run-tests.bat once in a new directory on Windows 10 x64, on a visible,
unlocked desktop with themes enabled. Do not cover or interact with the windows.
Return result-x86_64-v5/v6.txt, result-i686-v5/v6.txt and exitcodes.txt.
Require all four exit codes 0, no failures/skips, and v6 traces with both
classic=0 themed=1 and classic=1 themed=0 for styles 0, 0x100 and 0x1100.
v5 covers 18 operations, v6 covers 36 (three styles, six operations, two themes).
The test-only patch's todo_wine marks apply to Wine, never to native Windows.
The subsequent fix removes fixed todos; its Windows assertions are identical.
This bundle contains no Wine or Microsoft DLLs. MinGW EXEs use system controls.
INFO
(cd "$output" && sha256sum ./*.exe status-background.c ./*.manifest LICENSE README.txt run-tests.bat > SHA256SUMS)
echo "L1 status-background bundle: $output"
