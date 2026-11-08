#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
# Paired comctl32 L2: production DLLs vs production + status-bar v2.
set -euo pipefail
umask 022
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
prepare=0
if [[ ${1:-} == --prepare-only ]]; then prepare=1; shift; fi
[[ $# == 2 ]] || { echo 'Usage: test-statusbar-v2.sh [--prepare-only] baseline|candidate NEW_OUTPUT' >&2; exit 2; }
variant="$1"
case "$variant" in baseline|candidate) ;; *) echo 'Unknown variant' >&2; exit 2 ;; esac
output="$(realpath -m -- "$2")"
[[ ! -e "$output" && ! -L "$output" ]] || { echo 'Use a new output directory' >&2; exit 2; }
if [[ -n ${WINE_SB_L2_WORK:-} ]]; then
    work="$WINE_SB_L2_WORK"
    [[ ! -e "$work" && ! -L "$work" ]] || { echo 'WINE_SB_L2_WORK must be new' >&2; exit 2; }
    mkdir -p -- "$work"
else
    work="$(mktemp -d "${TMPDIR:-/tmp}/winum-sbv2-l2.XXXXXXXX")"
fi
work="$(realpath -e -- "$work")"
# Reuse the candidate builder's pinned, SHA-256 checked source preparation.
export WINE_SB_V2_WORK="$work/prepared"
bash "$here/scripts/build-statusbar-v2.sh" --prepare-only "$work/preparation-output" > "$work/prepare.log" 2>&1 || {
    tail -n 40 "$work/prepare.log"; exit 1;
}
candidate="$work/prepared/pristine/wine-11.20"
if [[ "$variant" == baseline ]]; then
    mkdir -- "$work/baseline"
    tar -xf "$work/prepared/wine-11.20.tar" -C "$work/baseline"
    source_tree="$work/baseline/wine-11.20"
    source "$here/scripts/lib/wine-patches.sh"
    wine_apply_patches "$source_tree" "$here/patches" "$here" HEAD > "$work/baseline-patches.log"
    # Identical final assertions and source line numbers in both builds. Only
    # the baseline runtime retains v1; its new assertions can fail as expected.
    cp -- "$candidate/dlls/comctl32/tests/status.c" "$source_tree/dlls/comctl32/tests/status.c"
else
    source_tree="$candidate"
fi
mkdir -p -- "$output"
python3 - "$here" "$source_tree" "$variant" "$output/inputs.json" <<'PY'
import hashlib, json, subprocess, sys
from pathlib import Path
repo, source = map(Path, sys.argv[1:3])
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
tests = {str(p.relative_to(source)): sha(p) for p in sorted((source/'dlls/comctl32/tests').iterdir()) if p.is_file()}
patches = {str(p.relative_to(repo)): sha(p) for p in sorted((repo/'patches').glob('*.patch'))}
patches['patches/series'] = sha(repo/'patches/series')
candidate = {str(p.relative_to(repo)): sha(p) for p in sorted((repo/'patches/candidates/comctl32-statusbar-v2').glob('*.patch'))}
data = dict(schema=1, variant=sys.argv[3], commit=subprocess.check_output(['git','-C',str(repo),'rev-parse','HEAD'],text=True).strip(),
            wine_commit='7298a818b79446bae153b8d5aa27235ab3703bf9',
            wine_archive_sha256='9f516d63e9f814f4dad70ce0a1943211b8a6236008ac3025cf58004091c661db',
            production=patches, candidate=candidate, tests=tests,
            test_policy='Identical final candidate tests in both variants; baseline runtime keeps v1.')
Path(sys.argv[4]).write_text(json.dumps(data,indent=2,sort_keys=True)+'\n')
PY
printf 'Prepared %s: %s\n' "$variant" "$work"
if (( prepare )); then exit 0; fi
for tool in make xvfb-run xauth timeout python3 x86_64-w64-mingw32-gcc i686-w64-mingw32-gcc; do command -v "$tool" >/dev/null; done
build="$work/build"
mkdir -- "$build"
cd "$build"
"$source_tree/configure" --enable-archs=x86_64,i386 --enable-tests \
    --with-x --with-wayland --with-vulkan --with-opengl --with-freetype --with-fontconfig \
    --with-gnutls --with-pulse --with-alsa --with-gstreamer --with-dbus --with-krb5 \
    > "$work/configure.log" 2>&1 || { tail -n 40 "$work/configure.log"; exit 1; }
for feature in SONAME_LIBX11 SONAME_LIBXRENDER SONAME_LIBFREETYPE SONAME_LIBFONTCONFIG; do
    grep -q "^#define $feature " include/config.h || { echo "Missing $feature"; exit 1; }
done
make -j"${JOBS:-$(nproc)}" > "$work/build.log" 2>&1 || { tail -n 60 "$work/build.log"; exit 1; }
# One persistent desktop for enumeration, initialization and all test units.
export WINEPREFIX="$work/prefix"
export WINEDEBUG=-all
export WINEDLLOVERRIDES='mscoree,mshtml='
export WINETEST_DEBUG=1
export WINETEST_TIME=0
export WINETEST_COLOR=0
export WINETEST_INTERACTIVE=0
export WINETEST_REPORT_SUCCESS=0
export WINETEST_PLATFORM=wine
export WINETEST_REPORT_FLAKY=0
export WINETEST_MUTE_THRESHOLD=1000000
export XDG_SESSION_TYPE=x11
unset WAYLAND_DISPLAY
trap '"$build/server/wineserver" -k 2>/dev/null || true' EXIT
xvfb-run -a -s '-screen 0 1280x1024x24 -nolisten tcp' \
    python3 "$here/scripts/statusbar-v2-l2.py" run "$source_tree" "$build" "$work/logs" "$output/results.json"
"$build/server/wineserver" -k
"$build/server/wineserver" -w
printf 'Bounded results: %s\n' "$output"
