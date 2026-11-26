#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
set -euo pipefail
[[ $# == 1 || $# == 2 ]] || { echo 'Usage: build-l1-riched20-default-tab-stop.sh NEW_OUTPUT [WINE_TARBALL]' >&2; exit 2; }
repo="$(cd -- "$(dirname -- "$0")/.." && pwd)"
[[ ! -e "$1" && ! -L "$1" ]] || { echo 'Choose a new output directory' >&2; exit 2; }
out="$(realpath -m -- "$1")"
source "$repo/wine/version.env"
WINE_PINNED_VERSION="$WINE_VERSION"
source "$repo/scripts/lib/wine-source.sh"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT

tarball="${2:-$work/wine-$WINE_VERSION.tar.xz}"
if [[ $# == 1 ]]; then
    curl -fLsS --retry 3 -o "$tarball" \
        "https://dl.winehq.org/wine/source/${WINE_VERSION%%.*}.x/wine-$WINE_VERSION.tar.xz"
fi
wine_verify_tarball "$tarball" "$WINE_VERSION"
src="$work/wine-$WINE_VERSION"
tar -xf "$tarball" -C "$work" "wine-$WINE_VERSION"/{COPYING.LIB,include/wine/{debug,test}.h,dlls/riched20/tests/richole.c}
patch -d "$src" -p1 --fuzz=0 < "$repo/patches/tests-first/riched20-tests-default-tab-stop.patch"

# Compile the focused functions from the exact patched Wine test.
python3 - "$src/dlls/riched20/tests/richole.c" "$work/tab-stop.c" <<'PY'
from pathlib import Path
import sys
source = Path(sys.argv[1]).read_text()
test = source[source.index('static void check_default_tab('):source.index('START_TEST(richole)')]
head = """/* Standalone build of the focused riched20 richole test. */
#define COBJMACROS
#include <windows.h>
#include <ole2.h>
#include <richedit.h>
#include <tom.h>
#include <wine/test.h>

/* Wine's test runtime provides this symbol for in-tree builds. */
const char * __cdecl __wine_dbg_strdup(const char *value) { return _strdup(value); }

"""
tail = """
START_TEST(tab_stop)
{
    HMODULE module = LoadLibraryW(L"riched20.dll");

    ok(!!module, "LoadLibraryW failed: %lu.\\n", GetLastError());
    if (!module) return;
    test_default_tab_stop();
    FreeLibrary(module);
}
"""
Path(sys.argv[2]).write_text(head + test + tail)
PY
bat="$work/run-tests.bat"
printf '%s\r\n' '@echo off' 'cd /d "%~dp0"' 'if exist exitcodes.txt exit /b 2' > "$bat"
for arch in x86_64 i686; do
    exe="riched20-default-tab-stop-$arch.exe"
    "$arch-w64-mingw32-gcc" -DSTANDALONE -DDECLSPEC_EXPORT= -O2 -Wall -Werror \
        -I "$src/include" "$work/tab-stop.c" \
        -o "$work/$exe" -lole32 -luser32 -lgdi32
    printf '%s\r\n' "$exe tab_stop > result-riched20-default-tab-stop-$arch.txt 2>&1" \
        ">>exitcodes.txt echo $arch=%ERRORLEVEL%" >> "$bat"
done
printf '%s\r\n' 'type exitcodes.txt' >> "$bat"
cat > "$work/README.txt" <<INFO
Wine $WINE_VERSION ITextDocument default tab stop test, winum commit $(git -C "$repo" rev-parse --short HEAD).
In a fresh directory on native offline Windows 10 run run-tests.bat.
Return result-riched20-default-tab-stop-*.txt and exitcodes.txt.
INFO
mkdir -p -- "$out"
cp -- "$src/COPYING.LIB" "$work"/{*.exe,tab-stop.c,run-tests.bat,README.txt} "$out/"
(cd "$out" && sha256sum -- ./* > SHA256SUMS)
