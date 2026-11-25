#!/usr/bin/env bash
# Original, AI-assisted (Claude), LGPL-2.1-or-later.
# Build the L1 bundle of the token integrity test for the offline VM: the
# exact block of patches/tests-first/advapi32-tests-token-integrity-level.patch,
# as a standalone program with Wine's test.h, plus the MEASURE program.
#   bash scripts/build-l1-token-integrity.sh NEW_OUTPUT [WINE_TARBALL]
# WINE_SRC=<extracted Wine tree> instead of a tarball is for local runs only.
set -euo pipefail
[[ $# == 1 || $# == 2 ]] || { echo 'Usage: build-l1-token-integrity.sh NEW_OUTPUT [WINE_TARBALL]' >&2; exit 2; }
[[ ! -e "$1" ]] || { echo 'Choose a new output directory' >&2; exit 2; }
repo=$(cd -- "$(dirname -- "$0")/.." && pwd)
out=$(realpath -m -- "$1")
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
src="$work/wine"
files=(COPYING.LIB dlls/advapi32/tests/security.c include/wine/test.h include/wine/debug.h)
if [[ -n "${WINE_SRC:-}" ]]; then
    for f in "${files[@]}"; do install -D -m 644 "$WINE_SRC/$f" "$src/$f"; done
else
    source "$repo/wine/version.env"
    export WINE_PINNED_VERSION="$WINE_VERSION"
    source "$repo/scripts/lib/wine-source.sh"
    tarball="${2:-$work/wine-$WINE_VERSION.tar.xz}"
    [[ $# == 2 ]] || curl -fL --retry 3 -o "$tarball" \
        "https://dl.winehq.org/wine/source/${WINE_VERSION%%.*}.x/wine-$WINE_VERSION.tar.xz"
    wine_verify_tarball "$tarball" "$WINE_VERSION" >/dev/null
    tar -xf "$tarball" -C "$work" "${files[@]/#/wine-$WINE_VERSION/}"
    mv "$work/wine-$WINE_VERSION" "$src"
fi
patch -d "$src" -p1 --fuzz=0 -s < "$repo/patches/tests-first/advapi32-tests-token-integrity-level.patch"
{
    printf '%s\n' '/* Standalone wrapper for the exact Wine test, AI-assisted (Claude), LGPL-2.1-or-later. */' \
        '#define _WIN32_WINNT 0x0a00' '#include <windows.h>' '#include <stdio.h>' '#include "wine/test.h"'
    sed -n '/^\/\* BEGIN token integrity tests \*\//,/^\/\* END token integrity tests \*\//p' \
        "$src/dlls/advapi32/tests/security.c"
    printf '%s\n' 'START_TEST(token_integrity)' '{' '    test_token_integrity_level();' '}'
} > "$work/token-integrity-test.c"
test "$(grep -c '^/\* END token integrity tests \*/' "$work/token-integrity-test.c")" -eq 1
bash "$repo/docs/wine-repro/server-token-integrity/build-bundle.sh" "$work/bundle"
for arch in x86_64 i686; do
    "$arch-w64-mingw32-${CC_KIND:-gcc}" -O2 -static -Wall -Wno-unused-parameter \
        -DSTANDALONE -DDECLSPEC_EXPORT= -I "$src/include" "$work/token-integrity-test.c" \
        -o "$work/bundle/token-integrity-test-$arch.exe" -ladvapi32
done
mkdir -p -- "$out"
cp -- "$work/bundle/"* "$work/token-integrity-test.c" "$src/COPYING.LIB" "$out/"
rm -f "$out/SHA256SUMS"
printf '%s\r\n' '@echo off' 'rem Run once from a normal and once from an elevated console.' \
    'setlocal' 'cd /d "%~dp0"' 'set "level=medium"' \
    'whoami /groups | find "S-1-16-12288" >nul && set "level=high"' \
    'set "failed=0"' '>l1-exitcodes-%level%.txt type nul' \
    'for %%a in (x86_64 i686) do call :run %%a' 'type l1-exitcodes-%level%.txt' 'exit /b %failed%' \
    ':run' 'token-integrity-test-%1.exe >result-l1-token-integrity-%1-%level%.txt 2>&1' \
    'set "code=%ERRORLEVEL%"' '>>l1-exitcodes-%level%.txt echo %1=%code%' \
    'if not "%code%"=="0" set "failed=1"' 'exit /b 0' > "$out/run-tests.bat"
(cd "$out" && sha256sum -- ./* > SHA256SUMS)
