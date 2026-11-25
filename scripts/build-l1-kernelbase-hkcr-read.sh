#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
set -euo pipefail
[[ $# == 1 || $# == 2 ]] || { echo 'Usage: build-l1-kernelbase-hkcr-read.sh NEW_OUTPUT [WINE_TARBALL]' >&2; exit 2; }
[[ ! -e "$1" && ! -L "$1" ]] || { echo 'Choose a new output directory' >&2; exit 2; }
repo=$(cd -- "$(dirname -- "$0")/.." && pwd)
out=$(realpath -m -- "$1")
source "$repo/wine/version.env"
export WINE_PINNED_VERSION="$WINE_VERSION"
source "$repo/scripts/lib/wine-source.sh"
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
tarball="${2:-$work/wine-$WINE_VERSION.tar.xz}"
if [[ $# == 1 ]]; then
    curl -fL --retry 3 -o "$tarball" \
        "https://dl.winehq.org/wine/source/${WINE_VERSION%%.*}.x/wine-$WINE_VERSION.tar.xz"
fi
wine_verify_tarball "$tarball" "$WINE_VERSION"
src="$work/wine-$WINE_VERSION"
tar -xf "$tarball" -C "$work" \
    "wine-$WINE_VERSION"/{COPYING.LIB,dlls/advapi32/tests/registry.c,include/wine/{debug,test}.h}
patch -d "$src" -p1 --fuzz=0 < "$repo/patches/tests-first/advapi32-tests-hkcr-class-reads.patch"

{
    cat <<'C'
/* Standalone wrapper for the exact Wine test, AI-assisted (Codex), LGPL-2.1-or-later. */
#define _WIN32_WINNT 0x0a00
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "wine/test.h"
C
    sed -n '/^\/\* BEGIN HKCR read tests \*\//,/^\/\* END HKCR read tests \*\//p' \
        "$src/dlls/advapi32/tests/registry.c"
    cat <<'C'
START_TEST(hkcr_class_reads)
{
    test_hkcr_class_reads();
    test_hkcr_class_collision();
}
C
} > "$work/hkcr-class-reads.c"
test "$(grep -c '^/\* BEGIN HKCR read tests \*/' "$work/hkcr-class-reads.c")" -eq 1
test "$(grep -c '^/\* END HKCR read tests \*/' "$work/hkcr-class-reads.c")" -eq 1

bash "$repo/docs/wine-repro/kernelbase-hkcr-read/build-bundle.sh" "$work/bundle"
for arch in x86_64 i686; do
    "$arch-w64-mingw32-clang" -O2 -static -Wall -Wextra -Werror -Wno-unused-parameter \
        -DSTANDALONE -DDECLSPEC_EXPORT= -I "$src/include" "$work/hkcr-class-reads.c" \
        -o "$work/bundle/hkcr-class-reads-$arch.exe" -ladvapi32
done
mkdir -p -- "$(dirname -- "$out")"
mkdir -- "$out"
cp -- "$work/bundle/"*.exe "$src/COPYING.LIB" "$work/hkcr-class-reads.c" \
    "$repo/docs/wine-repro/kernelbase-hkcr-read/measure.c" "$out/"
printf '%s\r\n' \
    '@echo off' 'setlocal DisableDelayedExpansion' 'cd /d "%~dp0"' \
    'if exist exitcodes.txt (echo Use a fresh directory & exit /b 2)' \
    'if exist result-hkcr-class-reads-x86_64.txt exit /b 2' \
    'if exist result-hkcr-class-reads-i686.txt exit /b 2' \
    'set "failed=0"' 'for %%a in (x86_64 i686) do call :run %%a' \
    'type exitcodes.txt' 'exit /b %failed%' \
    ':run' 'echo Running HKCR L1 %1. Accept the setup helper UAC prompt.' \
    'hkcr-read-%1.exe --run-test hkcr-class-reads-%1.exe >result-hkcr-class-reads-%1.txt 2>&1' \
    'set "code=%ERRORLEVEL%"' '>>exitcodes.txt echo %1=%code%' \
    'if not "%code%"=="0" set "failed=1"' 'exit /b 0' > "$out/run-tests.bat"
(cd "$out" && sha256sum -- ./* > SHA256SUMS)
