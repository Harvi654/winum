#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
set -euo pipefail
[[ $# == 1 || $# == 2 ]] || { echo 'Usage: build-l1-wbemdisp-method-invoke.sh NEW_OUTPUT [WINE_TARBALL]' >&2; exit 2; }
repo="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
[[ ! -e "$1" && ! -L "$1" ]] || { echo 'Choose a new output directory' >&2; exit 2; }
out="$(realpath -m -- "$1")"
source "$repo/wine/version.env"
WINE_PINNED_VERSION="$WINE_VERSION"
source "$repo/scripts/lib/wine-source.sh"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT

tarball="${2:-$work/wine-$WINE_VERSION.tar.xz}"
if [[ $# == 1 ]]; then
    curl -fL --retry 3 -o "$tarball" \
        "https://dl.winehq.org/wine/source/${WINE_VERSION%%.*}.x/wine-$WINE_VERSION.tar.xz"
fi
wine_verify_tarball "$tarball" "$WINE_VERSION"
source_dir="$work/wine-$WINE_VERSION"
tar -xf "$tarball" -C "$work" \
    "wine-$WINE_VERSION"/{COPYING.LIB,dlls/wbemdisp/tests/Makefile.in,include/wine/debug.h,include/wine/test.h}
patch -d "$source_dir" -p1 --fuzz=0 < "$repo/patches/tests-first/wbemdisp-tests-method-invoke.patch"

bat="$work/run-tests.bat"
printf '%s\r\n' '@echo off' 'cd /d "%~dp0"' 'if exist exitcodes.txt (echo already run - use a fresh copy & exit /b 2)' > "$bat"
for arch in x86_64 i686; do
    exe="wbemdisp-method-$arch.exe"
    "$arch-w64-mingw32-gcc" -O2 -static -Wall -Werror -DSTANDALONE -DDECLSPEC_EXPORT= \
        -I "$source_dir/include" "$source_dir/dlls/wbemdisp/tests/method.c" \
        -o "$work/$exe" -lole32 -loleaut32 -luuid -ladvapi32
    printf '%s\r\n' "echo running $exe" \
        "$exe method > result-wbemdisp-method-$arch.txt 2>&1" \
        ">>exitcodes.txt echo wbemdisp-method $arch=%ERRORLEVEL%" >> "$bat"
done
printf '%s\r\n' 'echo done' 'pause' >> "$bat"
cat > "$work/README.txt" <<INFO
Wine $WINE_VERSION wbemdisp method test, winum commit $(git -C "$repo" rev-parse --short HEAD).
Run run-tests.bat in a new folder in the offline Windows 10 VM.
Return both result-wbemdisp-method-*.txt files and exitcodes.txt.
The expected native result is zero failures, todo results and skips.
INFO
mkdir -p -- "$(dirname -- "$out")"
mkdir -- "$out"
cp -- "$source_dir/COPYING.LIB" "$source_dir/dlls/wbemdisp/tests/method.c" \
    "$work"/{*.exe,run-tests.bat,README.txt} "$out/"
(cd "$out" && sha256sum -- ./* > SHA256SUMS)
