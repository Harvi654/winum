#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
set -euo pipefail
[[ $# == 1 || $# == 2 ]] || { echo 'Usage: build-l1-combase-per-user-classes.sh NEW_OUTPUT [WINE_TARBALL]' >&2; exit 2; }
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
src="$work/wine-$WINE_VERSION"
tar -xf "$tarball" -C "$work" \
    "wine-$WINE_VERSION"/{COPYING.LIB,dlls/combase/tests/Makefile.in,include/wine/debug.h,include/wine/test.h}
patch -d "$src" -p1 --fuzz=0 < "$repo/patches/tests-first/combase-tests-per-user-classes.patch"

cd "$work"
printf 'EXPORTS\nDllGetClassObject\nDllCanUnloadNow\n' > fixture.def
cat > unit.manifest <<'XML'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
  <assemblyIdentity name="Winum.CombaseL1" version="1.0.0.0" processorArchitecture="*"/>
  <trustInfo xmlns="urn:schemas-microsoft-com:asm.v3">
    <security><requestedPrivileges>
      <requestedExecutionLevel level="asInvoker" uiAccess="false"/>
    </requestedPrivileges></security>
  </trustInfo>
</assembly>
XML
printf '%s\n' '101 RCDATA "per_user_server.dll"' '1 24 "unit.manifest"' > unit.rc
bat="$work/run-tests.bat"
printf '%s\r\n' '@echo off' 'cd /d "%~dp0"' \
    'if exist exitcodes.txt (echo already run - use a fresh copy & exit /b 2)' \
    'set "WINUM_COM_L1_RUNAS=1"' > "$bat"
for arch in x86_64 i686; do
    exe="combase-per-user-classes-$arch.exe"
    "$arch-w64-mingw32-gcc" -shared -O2 -Wall -Werror -Wl,--kill-at \
        "$src/dlls/combase/tests/per_user_server.c" fixture.def -o per_user_server.dll -luuid
    "$arch-w64-mingw32-windres" -i unit.rc -o unit.o
    "$arch-w64-mingw32-gcc" -O2 -static -Wall -Werror -DSTANDALONE -DDECLSPEC_EXPORT= \
        -I "$src/include" "$src/dlls/combase/tests/per_user_classes.c" unit.o \
        -o "$work/$exe" -lole32 -luuid -ladvapi32 -lshell32
    printf '%s\r\n' "echo running $exe" \
        "$exe per_user_classes > result-combase-per-user-classes-$arch.txt 2>&1" \
        ">>exitcodes.txt echo combase-per-user-classes $arch=%ERRORLEVEL%" >> "$bat"
done
printf '%s\r\n' 'echo done' 'pause' >> "$bat"
cat > README.txt <<INFO
Wine $WINE_VERSION combase per-user class test, winum commit $(git -C "$repo" rev-parse --short HEAD).
Run run-tests.bat from the normal desktop in the offline Windows 10 VM.
Accept the UAC prompts for the setup helpers only.
Return both result-combase-per-user-classes-*.txt files and exitcodes.txt.
The expected native result is zero failures, todos and skips.
INFO
mkdir -p -- "$(dirname -- "$out")"
mkdir -- "$out"
cp -- "$src/COPYING.LIB" "$src"/dlls/combase/tests/per_user_{classes,server}.c ./*.exe \
    "$bat" README.txt "$out/"
(cd "$out" && sha256sum -- ./* > SHA256SUMS)
