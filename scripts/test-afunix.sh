#!/usr/bin/env bash
# Original, AI-assisted (Claude), LGPL-2.1-or-later.
# Build Wine with patches/series plus the AF_UNIX candidate and run the tests
# it touches. Variants:
#   tests      patches/series + the candidate's test patch only (the baseline)
#   v3         patches/series + v3 runtime and v4 measurement assertions
#   candidate  patches/series + the whole candidate series
#   test-afunix.sh tests|v3|candidate OUTDIR
# Never changes the production series; results are text logs in OUTDIR.
set -euo pipefail
HERE="$(cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/.." && pwd)"
VARIANT="${1:?tests, v3 or candidate}"
OUTDIR="$(mkdir -p "${2:?OUTDIR}" && cd "$2" && pwd)"
case "$VARIANT" in tests|v3|candidate) ;; *) echo "variant: tests, v3 or candidate" >&2; exit 2 ;; esac
CAND="$HERE/patches/candidates/ws2_32-af-unix"
TEST_PATCH=ws2_32-tests-afunix-winum.patch
source "$HERE/wine/version.env"
WINE_PINNED_VERSION="$WINE_VERSION"
source "$HERE/scripts/lib/wine-source.sh"
source "$HERE/scripts/lib/wine-patches.sh"
WORK="${WORK:-${XDG_CACHE_HOME:-$HOME/.cache}/winum/afunix-$VARIANT}"
JOBS="${JOBS:-$(nproc)}"
UNIT_TIMEOUT="${UNIT_TIMEOUT:-300}"
# test program, units ("" = all)
UNITS="ws2_32: ntdll:file kernel32:file advapi32:security"
say() { printf '\n==> %s\n' "$*"; }

mkdir -p "$WORK"
SRC="$WORK/wine-$WINE_VERSION"
rm -rf "$SRC"
TARBALL="$WORK/wine-$WINE_VERSION.tar.xz"
[ -s "$TARBALL" ] || curl -fL --retry 3 -o "$TARBALL" \
    "https://dl.winehq.org/wine/source/${WINE_VERSION%%.*}.x/wine-$WINE_VERSION.tar.xz"
wine_verify_tarball "$TARBALL" "$WINE_VERSION" >/dev/null
tar -xf "$TARBALL" -C "$WORK"

say "Applying patches/series"
wine_apply_patches "$SRC" "$HERE/patches" "$HERE" HEAD
say "Applying the AF_UNIX candidate ($VARIANT)"
while IFS= read -r p; do
    if [ "$VARIANT" = tests ] && [ "$p" != "$TEST_PATCH" ]; then continue; fi
    if [ "$VARIANT" = v3 ] && [ "$p" = ntdll-afunix-handle-reparse-tag.patch ]; then break; fi
    echo "  $p"
    patch -d "$SRC" -p1 -s --fuzz=0 < "$CAND/$p" || { echo "$p does not apply" >&2; exit 1; }
done < <(sed -e 's/#.*//' -e 's/[[:space:]]//g' -e '/^$/d' "$CAND/series")

say "Configuring with tests"
mkdir -p "$SRC/build" && cd "$SRC/build"
../configure --prefix=/usr --libdir=/usr/lib --sysconfdir=/etc \
    --enable-archs=x86_64,i386 --enable-tests --with-x \
    > "$WORK/configure.log" 2>&1 || { tail -40 "$WORK/configure.log"; exit 1; }
say "Building with $JOBS jobs"
make -j"$JOBS" > "$WORK/build.log" 2>&1 || { grep -E 'error' "$WORK/build.log" | head -40; tail -40 "$WORK/build.log"; exit 1; }
grep -E 'warning:' "$WORK/build.log" | grep -E 'server/(sock|fd)\.c|ntdll/unix/(socket|file)\.c|ws2_32/socket\.c|wine/afd\.h' \
    > "$OUTDIR/candidate-warnings.txt" || true

say "Running tests"
export WINEPREFIX="$WORK/prefix" WINEDEBUG=-all WINEDLLOVERRIDES="mscoree,mshtml="
rm -rf "$WINEPREFIX"
Xvfb -displayfd 3 -screen 0 1280x1024x24 -nolisten tcp 3>"$WORK/display" &
XVFB_PID=$!
trap 'timeout -k 5 10 ./server/wineserver -k 2>/dev/null || true; kill "$XVFB_PID" 2>/dev/null || true' EXIT
for _ in $(seq 50); do [ -s "$WORK/display" ] && break; sleep 0.2; done
export DISPLAY=":$(tr -d '\n' < "$WORK/display")"
./wine wineboot -i >/dev/null 2>&1 || true
./server/wineserver -w 2>/dev/null || true
mkdir -p "$OUTDIR/logs"
: > "$OUTDIR/summary-$VARIANT.tsv"
: > "$OUTDIR/failures-$VARIANT.txt"
for spec in $UNITS; do
    t="${spec%%:*}"; want="${spec#*:}"
    for arch in x86_64 i386; do
        exe="dlls/$t/tests/$arch-windows/${t}_test.exe"
        [ -f "$exe" ] || continue
        units="$want"
        [ -n "$units" ] || units="$(./wine "$exe" --list 2>/dev/null | sed -n 's/^    //p' | tr -d '\r')"
        for u in $units; do
            log="$OUTDIR/logs/$t.$arch.$u.log"
            timeout -k 10 "$UNIT_TIMEOUT" ./wine "$exe" "$u" > "$log" 2>&1 || echo "exit=$?" >> "$log"
            # Wine prefixes the summary with the process id ("00d8:afd: ...")
            line="$(grep -a -E "^([0-9a-f]+:)?$u: [0-9]+ tests executed" "$log" | tail -1 | sed -E 's/^[0-9a-f]+://' || true)"
            printf '%s\t%s\t%s\t%s\n' "$t" "$arch" "$u" "${line:-no summary ($(tail -1 "$log"))}" >> "$OUTDIR/summary-$VARIANT.tsv"
            grep -a 'Test failed' "$log" | sed -E 's/^[0-9a-f]+://; s/^([^:]+:[0-9]+).*/\1/' | sort -u \
                | sed "s/^/$t $arch $u /" >> "$OUTDIR/failures-$VARIANT.txt" || true
        done
    done
done
cat "$OUTDIR/summary-$VARIANT.tsv"
echo "failing lines ($VARIANT):"; cat "$OUTDIR/failures-$VARIANT.txt"
./server/wineserver -k 2>/dev/null || true

if [ "$VARIANT" = candidate ]; then
    # a runnable build for L4 in RobotStudio and Inventor (not a release;
    # unpacked like a winum release, e.g. next to winum-11.22-1-x86_64)
    NAME="winum-$WINE_VERSION-$WINUM_BUILD-afunix-x86_64"
    if [ -f "$CAND/server-afunix-client-address.patch" ]; then
        NAME="winum-$WINE_VERSION-$WINUM_BUILD-afunix-v4-x86_64"
    fi
    say "Packing $NAME for L4"
    rm -rf "$WORK/stage" "$WORK/$NAME"
    make install DESTDIR="$WORK/stage" > "$WORK/install.log" 2>&1 || { tail -40 "$WORK/install.log"; exit 1; }
    mv "$WORK/stage/usr" "$WORK/$NAME"
    cp "$SRC/PATCHES.txt" "$WORK/$NAME/" 2>/dev/null || true
    printf 'winum %s + candidate ws2_32-af-unix (%s), built by test-afunix.sh. Not a release.\n' \
        "$WINE_VERSION-$WINUM_BUILD" "$(git -C "$HERE" rev-parse --short HEAD)" > "$WORK/$NAME/CANDIDATE.txt"
    if [ -f "$CAND/server-afunix-client-address.patch" ]; then
        say "Testing the direct Linux host connection and repeating the measurements"
        bash "$HERE/scripts/build-measure-ws2_32-afunix-v4.sh" "$WORK/host-peer"
        (cd "$WORK/host-peer" && sha256sum -c SHA256SUMS)
        python3 "$WORK/host-peer/run-host-peer.py" --measure "$WORK/$NAME/bin/wine" "$WORK/host-peer" \
            | tee "$OUTDIR/result-wine-v4.txt"
    fi
    tar -C "$WORK" -cJf "$OUTDIR/$NAME.tar.xz" "$NAME"
    (cd "$OUTDIR" && sha256sum "$NAME.tar.xz" > "$NAME.tar.xz.sha256")
fi
say "Done: $OUTDIR"
