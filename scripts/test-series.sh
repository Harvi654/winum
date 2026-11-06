#!/bin/bash
# Original AI-assisted (Claude), LGPL-2.1-or-later.
# Build Wine with its test suite, vanilla or with patches/series, and run the
# tests of the DLLs the series touches (L2). Used by .github/workflows/test-series.yml
# for both variants; the compare step (scripts/compare-tests.py) then shows
# what the series changes. The series variant also packs the Wine test
# executables of patches whose tests never ran on Windows (L1 bundle).
#
#   bash scripts/test-series.sh vanilla|series OUTDIR
#
# Needs Wine's build dependencies (see the workflow), mingw-w64-gcc, xvfb-run
# and about 15 GB of disk. Does not install anything.
set -euo pipefail
HERE="$(cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/.." && pwd)"
VARIANT="${1:?vanilla or series}"
OUTDIR="$(mkdir -p "${2:?OUTDIR}" && cd "$2" && pwd)"
case "$VARIANT" in vanilla|series) ;; *) echo "variant: vanilla or series" >&2; exit 2 ;; esac
# shellcheck disable=SC1091
source "$HERE/wine/version.env"
WORK="${WORK:-${XDG_CACHE_HOME:-$HOME/.cache}/winum/test-$VARIANT}"
JOBS="${JOBS:-$(nproc)}"
# test directories of the DLLs patches/series changes (the server, win32u and
# winex11.drv changes are exercised by the user32/gdi32/win32u tests)
TESTS="${TESTS:-advapi32 combase comctl32 gdi32 glu32 kernelbase msi mspatcha msxml3 ntdll ole32 oleaut32 opengl32 riched20 rpcrt4 shell32 urlmon user32 wbemprox win32u wininet}"
UNIT_TIMEOUT="${UNIT_TIMEOUT:-900}"
say() { printf '\n==> %s\n' "$*"; }

mkdir -p "$WORK"
SRC="$WORK/wine-$WINE_VERSION"
rm -rf "$SRC"
case "$WINE_VERSION" in *.0|*.0.*) SUB="${WINE_VERSION%%.*}.0" ;; *) SUB="${WINE_VERSION%%.*}.x" ;; esac
TARBALL="$WORK/wine-$WINE_VERSION.tar.xz"
[ -s "$TARBALL" ] || curl -fL --retry 3 -o "$TARBALL" "https://dl.winehq.org/wine/source/$SUB/wine-$WINE_VERSION.tar.xz"
tar -xf "$TARBALL" -C "$WORK"
if [ "$VARIANT" = series ]; then
    say "Applying patches/series"
    # shellcheck disable=SC1091
    source "$HERE/scripts/lib/wine-patches.sh"
    wine_apply_patches "$SRC" "$HERE/patches" "$HERE" HEAD
fi

say "Configuring with tests"
mkdir -p "$SRC/build" && cd "$SRC/build"
../configure --enable-archs=x86_64,i386 --enable-tests \
    --with-x --with-wayland --with-vulkan --with-opengl --with-freetype --with-fontconfig \
    --with-gnutls --with-pulse --with-alsa --with-gstreamer --with-dbus --with-krb5 \
    > "$WORK/configure.log" 2>&1 || { tail -40 "$WORK/configure.log"; exit 1; }
say "Building with $JOBS jobs"
make -j"$JOBS" > "$WORK/build.log" 2>&1 || { tail -60 "$WORK/build.log"; exit 1; }

say "Running tests: $TESTS"
export WINEPREFIX="$WORK/prefix" WINEDEBUG=-all WINEDLLOVERRIDES="mscoree,mshtml="
rm -rf "$WINEPREFIX"
xvfb-run -a -s "-screen 0 1280x1024x24" ./wine wineboot -i >/dev/null 2>&1 || true
mkdir -p "$OUTDIR/logs"
for t in $TESTS; do
    for arch in x86_64 i386; do
        exe="dlls/$t/tests/$arch-windows/${t}_test.exe"
        [ -f "$exe" ] || continue
        units="$(./wine "$exe" --list 2>/dev/null | sed -n 's/^    //p' | tr -d '\r')" || true
        for u in $units; do
            log="$OUTDIR/logs/$t.$arch.$u.log"
            timeout -k 10 "$UNIT_TIMEOUT" xvfb-run -a -s "-screen 0 1280x1024x24" \
                ./wine "$exe" "$u" > "$log" 2>&1 || echo "exit=$?" >> "$log"
        done
    done
done
./server/wineserver -k 2>/dev/null || true

# summaries and failure locations only (no raw logs in the artifact's index)
python3 "$HERE/scripts/compare-tests.py" summarize "$OUTDIR/logs" > "$OUTDIR/results-$VARIANT.json"

if [ "$VARIANT" = series ]; then
    say "L1 bundle (Wine test executables for Windows 10)"
    B="$OUTDIR/l1-bundle"
    mkdir -p "$B"
    # dll:unit of patches with a Wine test that never ran on Windows
    L1_UNITS="${L1_UNITS:-advapi32:security msxml3:domdoc ntdll:exception ole32:storage32 shell32:shlfolder shell32:shelldispatch}"
    {
        printf '%s\r\n' '@echo off' 'cd /d "%~dp0"' 'if exist exitcodes.txt exit /b 2'
        for du in $L1_UNITS; do
            d="${du%%:*}" u="${du##*:}"
            for arch in x86_64 i386; do
                cp "dlls/$d/tests/$arch-windows/${d}_test.exe" "$B/${d}_test-$arch.exe"
                printf '%s\r\n' "${d}_test-$arch.exe $u > result-$d-$u-$arch.txt 2>&1" \
                    ">>exitcodes.txt echo $d $u $arch=%ERRORLEVEL%"
            done
        done
        printf '%s\r\n' 'pause'
    } > "$B/run-tests.bat"
    cp "$SRC/COPYING.LIB" "$B/"
    cat > "$B/README.txt" <<INFO
L1 bundle of winum $(git -C "$HERE" rev-parse --short HEAD): Wine's own test
programs, built from Wine $WINE_VERSION with patches/series (LGPL-2.1-or-later,
source: the winum source archive of this commit). Only these test programs,
the license and a batch file; no Wine or Microsoft DLLs.

On Windows 10 (real GPU driver not needed): copy the folder to a new directory,
run run-tests.bat, return result-*.txt and exitcodes.txt. Test units:
$L1_UNITS
Only the checks the patches added count for L1; failures elsewhere in these
files are recorded but are not caused by the patches.
INFO
    (cd "$B" && sha256sum ./* > SHA256SUMS)
fi
say "Done: $OUTDIR"
