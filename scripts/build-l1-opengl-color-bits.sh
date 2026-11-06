#!/usr/bin/env bash
# Original AI-assisted (Claude), LGPL-2.1-or-later.
# Builds the Windows 10 L1 bundle for patches/candidates/opengl-window-color-bits:
# the Wine test test_choosepixelformat_alpha, taken unchanged from the
# candidate patch, as standalone x86_64 and i686 executables (MinGW). The same
# EXEs run under Wine: vanilla should fail, the patched build pass.
set -euo pipefail
umask 022
[[ $# == 2 ]] || { echo 'Usage: build-l1-opengl-color-bits.sh WINE_SOURCE NEW_OUTPUT' >&2; exit 2; }
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
source="$(realpath -e -- "$1")"
output="$(realpath -m -- "$2")"
patch="$here/patches/opengl32-window-format-color-bits.patch"
if [[ ! -f "$source/include/wine/test.h" ]]; then
    found="$(find "$source" -maxdepth 6 -path '*/include/wine/test.h' -print -quit)"
    [[ -n "$found" ]] || { echo "No include/wine/test.h in $source" >&2; exit 2; }
    source="${found%/include/wine/test.h}"
fi
[[ ! -e "$output" && ! -L "$output" ]] || { echo 'Use a new output directory' >&2; exit 2; }
for arch in x86_64 i686; do
    command -v "$arch-w64-mingw32-gcc" >/dev/null || { echo "Missing $arch-w64-mingw32-gcc (pacman -S mingw-w64-gcc)" >&2; exit 2; }
done
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
mkdir -p "$work/include/wine"
cp "$source/include/wine/"{test,debug}.h "$work/include/wine/"

python3 - "$patch" "$work/choosepixelformat_alpha.c" <<'PY'
import sys
lines = open(sys.argv[1], encoding='utf-8').read().splitlines()
start = lines.index('+++ b/dlls/opengl32/tests/opengl.c')
added = []
for line in lines[start + 1:]:
    if line.startswith('diff --git'):
        break
    if line.startswith('+'):
        added.append(line[1:])
first = added.index('static void test_choosepixelformat_alpha(void)')
end = added.index('}', first)
body = '\n'.join(added[first:end + 1])
head = '''/* Standalone build of test_choosepixelformat_alpha from the winum candidate
 * opengl-window-color-bits (dlls/opengl32/tests/opengl.c). Original
 * AI-assisted (Claude), LGPL-2.1-or-later. */
#include <stdio.h>
#include <windows.h>
#include <GL/gl.h>
#include "wine/test.h"

#define WGL_DRAW_TO_WINDOW_ARB    0x2001
#define WGL_ACCELERATION_ARB      0x2003
#define WGL_SUPPORT_OPENGL_ARB    0x2010
#define WGL_DOUBLE_BUFFER_ARB     0x2011
#define WGL_PIXEL_TYPE_ARB        0x2013
#define WGL_COLOR_BITS_ARB        0x2014
#define WGL_ALPHA_BITS_ARB        0x201b
#define WGL_DEPTH_BITS_ARB        0x2022
#define WGL_STENCIL_BITS_ARB      0x2023
#define WGL_FULL_ACCELERATION_ARB 0x2027
#define WGL_TYPE_RGBA_ARB         0x202b

const char * __cdecl __wine_dbg_strdup(const char *str)
{
    return _strdup(str);
}

/* the test suite loads this after making a context current */
static struct
{
    BOOL (WINAPI *wglChoosePixelFormatARB)(HDC, const int *, const FLOAT *, UINT, int *, UINT *);
} ext;

'''
tail = '''

START_TEST(choosepixelformat_alpha)
{
    /* like the suite's START_TEST: a current context on another window */
    const PIXELFORMATDESCRIPTOR pfd =
    {
        .nSize = sizeof(PIXELFORMATDESCRIPTOR),
        .nVersion = 1,
        .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        .iPixelType = PFD_TYPE_RGBA,
        .cColorBits = 24,
    };
    HWND hwnd = CreateWindowA( "static", NULL, WS_OVERLAPPEDWINDOW, 10, 10, 200, 200, NULL, NULL, NULL, NULL );
    HDC hdc = GetDC( hwnd );
    int format = ChoosePixelFormat( hdc, &pfd );
    HGLRC ctx;

    if (!format || !SetPixelFormat( hdc, format, &pfd ) || !(ctx = wglCreateContext( hdc )) || !wglMakeCurrent( hdc, ctx ))
    {
        skip( "No OpenGL context.\\n" );
        return;
    }
    trace( "renderer %s, version %s\\n", glGetString( GL_RENDERER ), glGetString( GL_VERSION ) );
    ext.wglChoosePixelFormatARB = (void *)wglGetProcAddress( "wglChoosePixelFormatARB" );
    test_choosepixelformat_alpha();
    wglMakeCurrent( NULL, NULL );
    wglDeleteContext( ctx );
    ReleaseDC( hwnd, hdc );
    DestroyWindow( hwnd );
}
'''
with open(sys.argv[2], 'x', encoding='utf-8') as f:
    f.write(head + body + tail)
PY

for arch in x86_64 i686; do
    "$arch-w64-mingw32-gcc" -DSTANDALONE -DDECLSPEC_EXPORT= -O2 -static -Wall -Werror \
        -I"$work/include" "$work/choosepixelformat_alpha.c" \
        -o "$work/opengl-color-bits-$arch.exe" -lopengl32 -lgdi32 -luser32
done

mkdir -p -- "$output"
cp "$work/"*.exe "$work/choosepixelformat_alpha.c" "$here/LICENSE" "$output/"
printf '%s\r\n' '@echo off' 'cd /d "%~dp0"' \
    'opengl-color-bits-x86_64.exe choosepixelformat_alpha > result-x86_64.txt 2>&1' '>>exitcodes.txt echo x86_64=%ERRORLEVEL%' \
    'opengl-color-bits-i686.exe choosepixelformat_alpha > result-i686.txt 2>&1' '>>exitcodes.txt echo i686=%ERRORLEVEL%' \
    'pause' > "$output/run-tests.bat"
cat > "$output/README.txt" <<'INFO'
L1 for the winum candidate opengl-window-color-bits. Original AI-assisted
(Claude), LGPL-2.1-or-later. Only our own two EXEs, their source, the
license and a batch file.

Windows 10 with the real GPU driver: run run-tests.bat, return
result-x86_64.txt, result-i686.txt and exitcodes.txt.
Under Wine: wine opengl-color-bits-x86_64.exe choosepixelformat_alpha
INFO
(cd "$output" && sha256sum ./*.exe ./*.c LICENSE README.txt run-tests.bat > SHA256SUMS)
echo "L1 bundle: $output"
