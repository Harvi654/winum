#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
set -euo pipefail
[[ $# == 1 || $# == 2 ]] || { echo 'Usage: build-l1-ole32-revokedragdrop.sh NEW_OUTPUT [WINE_TARBALL]' >&2; exit 2; }
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
tar -xf "$tarball" -C "$work" "wine-$WINE_VERSION"/{COPYING.LIB,include/wine/{debug,test}.h,dlls/ole32/tests/dragdrop.c}
patch -d "$src" -p1 --fuzz=0 < "$repo/patches/tests-first/ole32-tests-revokedragdrop-cross-process.patch"

# Wine's normal test runtime supplies this symbol.
cat > "$work/debug-shim.c" <<'C'
#include <string.h>
const char * __cdecl __wine_dbg_strdup(const char *str) { return _strdup(str); }
C
bat="$work/run-tests.bat"
printf '%s\r\n' '@echo off' 'cd /d "%~dp0"' 'if exist exitcodes.txt exit /b 2' > "$bat"
for arch in x86_64 i686; do
    exe="ole32-revokedragdrop-$arch.exe"
    "$arch-w64-mingw32-gcc" -DSTANDALONE -DDECLSPEC_EXPORT= -O2 -Wall -Werror \
        -I "$src/include" "$src/dlls/ole32/tests/dragdrop.c" "$work/debug-shim.c" \
        -o "$work/$exe" -lole32 -luuid -luser32 -lgdi32
    printf '%s\r\n' "$exe dragdrop cross_revoke_only > result-ole32-revokedragdrop-$arch.txt 2>&1" \
        ">>exitcodes.txt echo $arch=%ERRORLEVEL%" >> "$bat"
done
printf '%s\r\n' 'type exitcodes.txt' >> "$bat"
cat > "$work/README.txt" <<INFO
Wine $WINE_VERSION dragdrop test, winum commit $(git -C "$repo" rev-parse --short HEAD).
In a fresh directory on native offline Windows 10 run run-tests.bat.
Return result-ole32-revokedragdrop-*.txt and exitcodes.txt.
INFO
mkdir -p -- "$out"
cp -- "$src/COPYING.LIB" "$src/dlls/ole32/tests/dragdrop.c" "$work"/{*.exe,debug-shim.c,run-tests.bat,README.txt} "$out/"
(cd "$out" && sha256sum -- ./* > SHA256SUMS)
