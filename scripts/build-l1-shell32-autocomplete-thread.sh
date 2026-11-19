#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
# Build the Wine autocomplete_thread test for the offline Windows 10 VM.
set -euo pipefail
[[ $# == 1 || $# == 2 ]] || { echo 'Usage: build-l1-shell32-autocomplete-thread.sh NEW_OUTPUT [WINE_TARBALL]' >&2; exit 2; }
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
wine_verify_tarball "$tarball" "$WINE_VERSION" >/dev/null
source_dir="$work/wine-$WINE_VERSION"
tar -xf "$tarball" -C "$work" \
    "wine-$WINE_VERSION"/{COPYING.LIB,dlls/shell32/tests/Makefile.in,include/wine/debug.h,include/wine/test.h}
patch -d "$source_dir" -p1 --fuzz=0 < "$repo/patches/tests-first/shell32-tests-autocomplete-thread.patch"

# Use MinGW's SDK headers and Wine's own standalone test harness.
for arch in x86_64 i686; do
    "$arch-w64-mingw32-gcc" -O2 -Wall -Werror -DSTANDALONE -DDECLSPEC_EXPORT= \
        -I"$source_dir/include" "$source_dir/dlls/shell32/tests/autocomplete_thread.c" \
        -o "$work/autocomplete-thread-$arch.exe" -lole32 -luuid -luser32 -lgdi32
done
bat="$work/run-tests.bat"
printf '%s\r\n' '@echo off' 'cd /d "%~dp0"' 'if exist exitcodes.txt (echo already run - use a fresh copy & exit /b 2)' > "$bat"
for arch in x86_64 i686; do
    printf '%s\r\n' "echo running autocomplete-thread-$arch.exe" \
        "autocomplete-thread-$arch.exe > result-shell32-autocomplete-thread-$arch.txt 2>&1" \
        ">>exitcodes.txt echo shell32-autocomplete-thread $arch=%ERRORLEVEL%" >> "$bat"
done
printf '%s\r\n' 'echo done' 'pause' >> "$bat"
cat > "$work/README.txt" <<EOF
Wine $WINE_VERSION shell32 autocomplete_thread test, winum commit $(git -C "$repo" rev-parse --short HEAD).
Copy this folder to a new directory in the offline Windows 10 VM and
double-click run-tests.bat. Leave the mouse and keyboard idle during the run.
Return only the two result-*.txt files and exitcodes.txt.
EOF
mkdir -p -- "$(dirname -- "$out")"
mkdir -- "$out"
cp -- "$source_dir/COPYING.LIB" "$work"/{*.exe,run-tests.bat,README.txt} "$out/"
(cd "$out" && sha256sum -- ./* > SHA256SUMS)
