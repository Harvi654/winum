#!/usr/bin/env bash
# Builds the riched20 target device scroll test for the offline Windows 10
# VM: Wine's dlls/riched20/tests/editor.c after the release series and
# patches/tests-first/riched20-tests-target-device-scroll.patch, the new test
# function compiled on its own for x86_64 and i686.
# Usage: build-l1-riched20-target-device-scroll.sh NEW_OUTPUT [WINE_TARBALL]
# Original, AI-assisted (Claude), LGPL-2.1-or-later.
set -euo pipefail
[[ $# == 1 || $# == 2 ]] || { echo 'Usage: build-l1-riched20-target-device-scroll.sh NEW_OUTPUT [WINE_TARBALL]' >&2; exit 2; }
repo="$(cd -- "$(dirname -- "$0")/.." && pwd)"
[[ ! -e "$1" && ! -L "$1" ]] || { echo 'Choose a new output directory' >&2; exit 2; }
out="$(realpath -m -- "$1")"
source "$repo/wine/version.env"
WINE_PINNED_VERSION="$WINE_VERSION"
source "$repo/scripts/lib/wine-source.sh"
source "$repo/scripts/lib/wine-patches.sh"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT

tarball="${2:-$work/wine-$WINE_VERSION.tar.xz}"
if [[ $# == 1 ]]; then
    curl -fLsS --retry 3 -o "$tarball" \
        "https://dl.winehq.org/wine/source/${WINE_VERSION%%.*}.x/wine-$WINE_VERSION.tar.xz"
fi
wine_verify_tarball "$tarball" "$WINE_VERSION"
src="$work/wine-$WINE_VERSION"
tar -xf "$tarball" -C "$work"
# The test patch is made against the release series (editor.c is changed
# by riched20-scf-word-insertion-point).
wine_apply_patches "$src" "$repo/patches" "$repo" HEAD
patch -d "$src" -p1 --fuzz=0 < "$repo/patches/tests-first/riched20-tests-target-device-scroll.patch"

python3 - "$src/dlls/riched20/tests/editor.c" "$work/target-device-scroll.c" <<'PY'
from pathlib import Path
import sys
source = Path(sys.argv[1]).read_text()
start = source.index('/* EM_SETTARGETDEVICE with a reference DC')
test = source[start:source.index('START_TEST( editor )')]
head = """/* Standalone build of the riched20 target device scroll test. */
#include <windows.h>
#include <richedit.h>
#include <wine/test.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

/* Wine's test runtime provides this symbol for in-tree builds. */
const char * __cdecl __wine_dbg_strdup(const char *value) { return _strdup(value); }

"""
tail = """
START_TEST(target_device_scroll)
{
    HMODULE module = LoadLibraryW(L"riched20.dll");

    ok(!!module, "LoadLibraryW failed: %lu.\\n", GetLastError());
    if (!module) return;
    test_target_device_scroll();
    FreeLibrary(module);
}
"""
Path(sys.argv[2]).write_text(head + test + tail)
PY
bat="$work/run-tests.bat"
printf '%s\r\n' '@echo off' 'cd /d "%~dp0"' 'if exist exitcodes.txt exit /b 2' > "$bat"
for arch in x86_64 i686; do
    exe="riched20-target-device-scroll-$arch.exe"
    "$arch-w64-mingw32-gcc" -DSTANDALONE -DDECLSPEC_EXPORT= -O2 -Wall -Werror \
        -I "$src/include" "$work/target-device-scroll.c" \
        -o "$work/$exe" -luser32 -lgdi32
    printf '%s\r\n' "$exe target_device_scroll > result-riched20-target-device-scroll-$arch.txt 2>&1" \
        ">>exitcodes.txt echo $arch=%ERRORLEVEL%" >> "$bat"
done
printf '%s\r\n' 'type exitcodes.txt' >> "$bat"
cat > "$work/README.txt" <<INFO
Wine $WINE_VERSION riched20 target device scroll test, winum commit $(git -C "$repo" rev-parse --short HEAD).
In a fresh directory on native offline Windows 10 (96 dpi) run run-tests.bat
from the desktop and do not touch mouse or keyboard while it runs.
Return result-riched20-target-device-scroll-*.txt and exitcodes.txt.
INFO
mkdir -p -- "$out"
cp -- "$src/COPYING.LIB" "$work"/{*.exe,target-device-scroll.c,run-tests.bat,README.txt} "$out/"
(cd "$out" && sha256sum -- ./* > SHA256SUMS)
