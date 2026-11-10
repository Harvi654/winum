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
# Needs Wine's build dependencies (see the workflow), mingw-w64-gcc, Xvfb
# and about 15 GB of disk. Does not install anything.
#
# Environment (each on its own export line):
#   TESTS         test directories to run (default: all the series touches);
#                 the workflow splits them into groups, one job per group
#   GROUP         name used in the result file names (default: all)
#   UNIT_TIMEOUT  seconds per test unit (default 300; Wine's own default is 120)
#   MAKE_L1       1 = also pack the L1 bundle (series only; default 1)
set -euo pipefail
HERE="$(cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/.." && pwd)"
VARIANT="${1:?vanilla or series}"
OUTDIR="$(mkdir -p "${2:?OUTDIR}" && cd "$2" && pwd)"
case "$VARIANT" in vanilla|series) ;; *) echo "variant: vanilla or series" >&2; exit 2 ;; esac
# shellcheck disable=SC1091
source "$HERE/wine/version.env"
WINE_PINNED_VERSION="$WINE_VERSION"
# shellcheck disable=SC1091
source "$HERE/scripts/lib/wine-source.sh"
WORK="${WORK:-${XDG_CACHE_HOME:-$HOME/.cache}/winum/test-$VARIANT}"
JOBS="${JOBS:-$(nproc)}"
# test directories of the DLLs patches/series changes (the server, win32u and
# winex11.drv changes are exercised by the user32/gdi32/win32u tests)
TESTS="${TESTS:-advapi32 combase comctl32 gdi32 glu32 kernel32 kernelbase msi mspatcha mshtml msxml3 ntdll ole32 oleaut32 opengl32 riched20 msftedit rpcrt4 shell32 urlmon user32 wbemprox win32u wininet}"
UNIT_TIMEOUT="${UNIT_TIMEOUT:-300}"
GROUP="${GROUP:-all}"
MAKE_L1="${MAKE_L1:-1}"
say() { printf '\n==> %s\n' "$*"; }

mkdir -p "$WORK"
SRC="$WORK/wine-$WINE_VERSION"
rm -rf "$SRC"
case "$WINE_VERSION" in *.0|*.0.*) SUB="${WINE_VERSION%%.*}.0" ;; *) SUB="${WINE_VERSION%%.*}.x" ;; esac
TARBALL="$WORK/wine-$WINE_VERSION.tar.xz"
[ -s "$TARBALL" ] || curl -fL --retry 3 -o "$TARBALL" "https://dl.winehq.org/wine/source/$SUB/wine-$WINE_VERSION.tar.xz"
wine_verify_tarball "$TARBALL" "$WINE_VERSION" >/dev/null || exit 1
tar -xf "$TARBALL" -C "$WORK"
if [ "$VARIANT" = series ]; then
    say "Applying patches/series"
    # shellcheck disable=SC1091
    source "$HERE/scripts/lib/wine-patches.sh"
    wine_apply_patches "$SRC" "$HERE/patches" "$HERE" HEAD
fi
if [ -f "$HERE/patches/tests-first/series" ]; then
    say "Applying patches/tests-first (both variants)"
    while IFS= read -r t; do
        patch -d "$SRC" -p1 -s --fuzz=0 < "$HERE/patches/tests-first/$t" || { echo "tests-first: $t does not apply" >&2; exit 1; }
    done < <(sed -e 's/#.*//' -e 's/[[:space:]]//g' -e '/^$/d' "$HERE/patches/tests-first/series")
fi

want_mshtml=0
case " $TESTS " in
    *' mshtml '*)
        want_mshtml=1
        # Pins must match the selected Wine source, on both variants.
        source "$HERE/scripts/lib/wine-gecko.sh"
        wine_fetch_gecko "$SRC" "$WORK/gecko" "$HERE/wine/gecko.env"
        ;;
esac

say "Configuring with tests"
mkdir -p "$SRC/build" && cd "$SRC/build"
../configure --enable-archs=x86_64,i386 --enable-tests \
    --with-x --with-wayland --with-vulkan --with-opengl --with-freetype --with-fontconfig \
    --with-gnutls --with-pulse --with-alsa --with-gstreamer --with-dbus --with-krb5 \
    > "$WORK/configure.log" 2>&1 || { tail -40 "$WORK/configure.log"; exit 1; }
say "Building with $JOBS jobs"
make -j"$JOBS" > "$WORK/build.log" 2>&1 || { tail -60 "$WORK/build.log"; exit 1; }

say "Running tests ($GROUP): $TESTS"
export WINEPREFIX="$WORK/prefix" WINEDEBUG=-all WINEDLLOVERRIDES="mscoree,mshtml="
rm -rf "$WINEPREFIX"
# one X server for the whole run (xvfb-run per unit cost a server start each time)
Xvfb -displayfd 3 -screen 0 1280x1024x24 -nolisten tcp 3>"$WORK/display" &
XVFB_PID=$!
cleanup_test_processes()
{
    # Cover setup/coverage failures as well as the normal end of the loop.
    timeout -k 5 10 ./server/wineserver -k 2>/dev/null || true
    if [ -n "${mshtml_prefix:-}" ]; then
        WINEPREFIX="$mshtml_prefix" timeout -k 5 10 ./server/wineserver -k 2>/dev/null || true
    fi
    kill "$XVFB_PID" 2>/dev/null || true
}
trap cleanup_test_processes EXIT
for _ in $(seq 50); do [ -s "$WORK/display" ] && break; sleep 0.2; done
[ -s "$WORK/display" ] || { echo "Xvfb did not start" >&2; exit 1; }
export DISPLAY=":$(tr -d '\n' < "$WORK/display")"
./wine wineboot -i >/dev/null 2>&1 || true
./server/wineserver -w 2>/dev/null || true
mkdir -p "$OUTDIR/logs"
if [ "$want_mshtml" = 1 ]; then
    say "Installing verified Gecko and probing both mshtml backends"
    mshtml_prefix="$WORK/mshtml-prefix"
    rm -rf "$mshtml_prefix"
    (
    trap - EXIT
    # MSI/COM registration belongs only to this dedicated HTML test prefix;
    # the prefix of all other suites keeps its previous boot state.
    export WINEPREFIX="$mshtml_prefix"
    timeout -k 10 120 ./wine wineboot -i > "$OUTDIR/mshtml-boot.log" 2>&1
    timeout -k 10 60 ./server/wineserver -w
    wine_install_gecko "$WORK/gecko" "$OUTDIR"
    for arch in x86_64 i386; do
        compiler_arch="$arch"
        [ "$arch" != i386 ] || compiler_arch=i686
        "$compiler_arch-w64-mingw32-gcc" -O2 -Wall -Wextra -Werror \
            "$HERE/scripts/mshtml-backend-probe.c" -o "$OUTDIR/mshtml-backend-$arch.exe" \
            -lole32 -loleaut32 -luuid
        if ! WINEDLLOVERRIDES='mscoree=;mshtml=b' timeout -k 10 30 \
                ./wine "$OUTDIR/mshtml-backend-$arch.exe" > "$OUTDIR/mshtml-backend-$arch.txt" 2>&1; then
            echo "MSHTML_SETUP incomplete: backend probe failed or timed out ($arch)" >&2
            exit 1
        fi
        grep '^MSHTML_BACKEND ' "$OUTDIR/mshtml-backend-$arch.txt"
    done
    )
fi
: > "$OUTDIR/durations-$VARIANT-$GROUP.tsv"
for t in $TESTS; do
    for arch in x86_64 i386; do
        exe="dlls/$t/tests/$arch-windows/${t}_test.exe"
        if [ ! -f "$exe" ]; then
            [ "$t" != mshtml ] || { echo "MSHTML_SETUP incomplete: missing $arch test EXE" >&2; exit 1; }
            continue
        fi
        # Keep mshtml disabled during boot, installation and other suites.
        # Builtin is enabled only in these enumeration/test child processes.
        test_overrides="$WINEDLLOVERRIDES"
        test_prefix="$WINEPREFIX"
        if [ "$t" = mshtml ]; then
            test_overrides='mscoree=;mshtml=b'
            test_prefix="$mshtml_prefix"
        fi
        if [ "$t" = mshtml ]; then
            if ! units="$(WINEPREFIX="$test_prefix" WINEDLLOVERRIDES="$test_overrides" timeout -k 10 "$UNIT_TIMEOUT" \
                    ./wine "$exe" --list 2>/dev/null | sed -n 's/^    //p' | tr -d '\r')"; then
                echo "MSHTML_SETUP incomplete: unit enumeration failed ($arch)" >&2
                exit 1
            fi
            [ -n "$units" ] && printf '%s\n' "$units" | grep -qx dom || \
                { echo "MSHTML_SETUP incomplete: no dom unit ($arch)" >&2; exit 1; }
            printf '%s\n' "$units" > "$OUTDIR/mshtml-units-$VARIANT-$arch.txt"
        else
            units="$(./wine "$exe" --list 2>/dev/null | sed -n 's/^    //p' | tr -d '\r')" || true
        fi
        for u in $units; do
            log="$OUTDIR/logs/$t.$arch.$u.log"
            start=$(date +%s)
            WINEPREFIX="$test_prefix" WINEDLLOVERRIDES="$test_overrides" timeout -k 10 "$UNIT_TIMEOUT" \
                ./wine "$exe" "$u" > "$log" 2>&1 || echo "exit=$?" >> "$log"
            printf '%s\t%s\t%s\t%s\n' "$t" "$arch" "$u" "$(( $(date +%s) - start ))" >> "$OUTDIR/durations-$VARIANT-$GROUP.tsv"
        done
    done
done
./server/wineserver -k 2>/dev/null || true
if [ "$want_mshtml" = 1 ]; then
    WINEPREFIX="$mshtml_prefix" ./server/wineserver -k 2>/dev/null || true
fi

# tests-first: print every failure of their test files on this side, so the
# job log shows what vanilla and the series each fail (the compare step only
# lists differences)
if [ -f "$HERE/patches/tests-first/series" ]; then
    for f in $(sed -n 's#^+++ b/dlls/[^/]*/tests/\([^/[:space:]]*\.c\).*#\1#p' "$HERE"/patches/tests-first/*.patch | sort -u); do
        say "tests-first failures in $f ($VARIANT)"
        grep -H -E "$f:[0-9]+: (Test failed|Test succeeded inside todo)" "$OUTDIR"/logs/*.log 2>/dev/null \
            | sed "s#^$OUTDIR/logs/##" | tr -d '\r' | head -200 || true
    done
fi

# summaries and failure locations only (no raw logs in the artifact's index)
python3 "$HERE/scripts/compare-tests.py" summarize "$OUTDIR/logs" > "$OUTDIR/results-$VARIANT-$GROUP.json"
if [ "$want_mshtml" = 1 ]; then
    python3 "$HERE/scripts/check-mshtml-l2.py" "$OUTDIR/results-$VARIANT-$GROUP.json" "$OUTDIR"
fi

if [ "$VARIANT" = series ] && [ "$MAKE_L1" = 1 ]; then
    say "L1 bundle (Wine test executables for Windows 10)"
    B="$OUTDIR/l1-bundle"
    mkdir -p "$B"
    # dll:unit of patches with a Wine test that never ran on Windows
    L1_UNITS="${L1_UNITS:-comctl32:toolbar msxml3:domdoc urlmon:sec_mgr kernel32:actctx user32:mousemove}"
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
