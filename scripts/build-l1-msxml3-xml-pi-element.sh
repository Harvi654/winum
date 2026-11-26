#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
set -euo pipefail
[[ $# == 1 || $# == 2 ]] || { echo 'Usage: build-l1-msxml3-xml-pi-element.sh NEW_OUTPUT [WINE_TARBALL]' >&2; exit 2; }
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
tar -xf "$tarball" -C "$work" "wine-$WINE_VERSION"/{COPYING.LIB,include/wine/{debug,test}.h}

# Compile the function added by the tests-first patch.
python3 - "$repo/patches/tests-first/msxml3-tests-xml-pi-element.patch" "$work/xml-pi-element.c" <<'PYTHON'
from pathlib import Path
import sys
source = Path(sys.argv[1]).read_text()
start = source.index('+static void test_append_xml_pi_to_element(')
end = source.index(' START_TEST(domdoc)', start)
test = "".join(line[1:] for line in source[start:end].splitlines(keepends=True) if line.startswith('+'))
head = """/* Standalone build of the focused msxml3 domdoc test. */
#define COBJMACROS
#include <windows.h>
#include <ole2.h>
#include <oleauto.h>
#include <initguid.h>
#include <msxml2.h>
#include <wine/test.h>

/* Wine's test runtime provides this symbol for in-tree builds. */
const char * __cdecl __wine_dbg_strdup(const char *value) { return _strdup(value); }

"""
tail = r"""
START_TEST(xml_pi_element)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    ok(SUCCEEDED(hr), "CoInitializeEx failed: %#lx.\n", hr);
    if (FAILED(hr)) return;
    test_append_xml_pi_to_element();
    CoUninitialize();
}
"""
Path(sys.argv[2]).write_text(head + test + tail)
PYTHON
bat="$work/run-tests.bat"
printf '%s\r\n' '@echo off' 'cd /d "%~dp0"' 'if exist exitcodes.txt exit /b 2' > "$bat"
for arch in x86_64 i686; do
    exe="msxml3-xml-pi-element-$arch.exe"
    "$arch-w64-mingw32-gcc" -DSTANDALONE -DDECLSPEC_EXPORT= -O2 -Wall -Werror \
        -I "$src/include" "$work/xml-pi-element.c" \
        -o "$work/$exe" -lole32 -loleaut32 -luuid
    printf '%s\r\n' "$exe xml_pi_element > result-msxml3-xml-pi-element-$arch.txt 2>&1" \
        ">>exitcodes.txt echo $arch=%ERRORLEVEL%" >> "$bat"
done
printf '%s\r\n' 'type exitcodes.txt' >> "$bat"
cat > "$work/README.txt" <<INFO
Wine $WINE_VERSION MSXML declaration append test, winum commit $(git -C "$repo" rev-parse --short HEAD).
In a fresh directory on native offline Windows 10 run run-tests.bat.
Return result-msxml3-xml-pi-element-*.txt and exitcodes.txt.
INFO
mkdir -p -- "$out"
cp -- "$src/COPYING.LIB" "$work"/{*.exe,xml-pi-element.c,run-tests.bat,README.txt} "$out/"
(cd "$out" && sha256sum -- ./* > SHA256SUMS)
