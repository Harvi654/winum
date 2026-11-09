#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
# Isolate one production fix: identical tests/full series, with or without it.
set -euo pipefail
umask 022
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
prepare=0
if [[ ${1:-} == --prepare-only ]]; then prepare=1; shift; fi
[[ $# == 2 ]] || { echo 'Usage: test-com-stub-actctx.sh [--prepare-only] without|with NEW_OUTPUT' >&2; exit 2; }
variant="$1"
case "$variant" in without|with) ;; *) echo 'Unknown variant' >&2; exit 2 ;; esac
output="$(realpath -m -- "$2")"
[[ ! -e "$output" && ! -L "$output" ]] || { echo 'Use a new output directory' >&2; exit 2; }
if [[ -n ${COM_STUB_WORK:-} ]]; then
    [[ ! -e "$COM_STUB_WORK" && ! -L "$COM_STUB_WORK" ]] || { echo 'COM_STUB_WORK must be new' >&2; exit 2; }
    mkdir -p -- "$COM_STUB_WORK"
    work="$(realpath -e -- "$COM_STUB_WORK")"
else
    work="$(mktemp -d "${TMPDIR:-/tmp}/winum-com-stub.XXXXXXXX")"
fi
revision=7298a818b79446bae153b8d5aa27235ab3703bf9
sum=9f516d63e9f814f4dad70ce0a1943211b8a6236008ac3025cf58004091c661db
cache="${XDG_CACHE_HOME:-$HOME/.cache}/winum/downloads"
archive="${COM_STUB_SOURCE_ARCHIVE:-$cache/wine-$revision.tar.gz}"
partial=
trap 'if [[ -n "$partial" ]]; then rm -f -- "$partial"; fi' EXIT
if [[ ! -f "$archive" ]]; then
    [[ -z ${COM_STUB_SOURCE_ARCHIVE:-} ]] || { echo 'COM_STUB_SOURCE_ARCHIVE is missing' >&2; exit 2; }
    mkdir -p -- "$cache"
    partial="$(mktemp "$cache/.com-stub-source.XXXXXXXX")"
    curl -fL --retry 3 -o "$partial" "https://codeload.github.com/wine-mirror/wine/tar.gz/$revision"
    printf '%s  %s\n' "$sum" "$partial" | sha256sum -c -
    mv -- "$partial" "$archive"
    partial=
fi
printf '%s  %s\n' "$sum" "$archive" | sha256sum -c -
mkdir -p -- "$output" "$work/source"
tar -xf "$archive" -C "$work/source"
source_tree="$work/source/wine-$revision"
python3 - "$here" "$source_tree" "$variant" "$output/inputs.json" <<'PY'
import hashlib, json, re, subprocess, sys, tempfile
from pathlib import Path
repo, source = map(Path, sys.argv[1:3])
variant, output = sys.argv[3:]
if (source/'VERSION').read_text().strip() != 'Wine version 11.20':
    raise SystemExit('Unexpected upstream version')
if not re.search(r'^WINE_VERSION=11\.20$', (repo/'wine/version.env').read_text(), re.M):
    raise SystemExit('This experiment is pinned to Wine 11.20')
name = 'combase-stub-activation-context.patch'
candidate = repo/'patches/candidates/combase-stub-activation-context-v2'/name
patch_text = candidate.read_text()
allowed = {'dlls/combase/combase_private.h', 'dlls/combase/rpc.c',
           'dlls/combase/stubmanager.c', 'dlls/combase/tests/Makefile.in',
           'dlls/combase/tests/stub_actctx.c'}
sections = re.findall(r'^--- ([^\n]+)\n\+\+\+ ([^\n]+)', patch_text, re.M)
if len(sections) != 5 or set(re.findall(r'^\+\+\+ b/(\S+)', patch_text, re.M)) != allowed:
    raise SystemExit('Candidate touched-file inventory changed; review pair isolation')
for old, new in sections:
    old, new = old.split('\t', 1)[0], new.split('\t', 1)[0]
    if old != 'a/' + new[2:] and not (old == '/dev/null' and new == 'b/dlls/combase/tests/stub_actctx.c'):
        raise SystemExit('Unexpected candidate file creation/rename')
if [new.split('\t', 1)[0] for old, new in sections[-2:]] != [
        'b/dlls/combase/tests/Makefile.in', 'b/dlls/combase/tests/stub_actctx.c']:
    raise SystemExit('Expected only test sections after the implementation')
lines = [line.split('#', 1)[0].strip() for line in (repo/'patches/series').read_text().splitlines()]
series = [line for line in lines if line]
if series.count(name) != 1: raise SystemExit('Expected exactly one target patch')
def apply(path):
    subprocess.run(['patch', '--batch', '--forward', '--fuzz=0', '-p1', '-i', str(path)], cwd=source, check=True)
for entry in series:
    if not re.fullmatch(r'[a-z0-9-]+\.patch', entry): raise SystemExit('Invalid series entry')
    if entry == name:
        if variant == 'with': apply(candidate)
    else: apply(repo/'patches'/entry)
if variant == 'without':
    # Keep the exact candidate test/wrapper, without any implementation hunk.
    test_patch = patch_text[patch_text.index('--- a/dlls/combase/tests/Makefile.in\n'):]
    with tempfile.NamedTemporaryFile(mode='w', suffix='.patch') as temp:
        temp.write(test_patch); temp.flush(); apply(Path(temp.name))
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
tests = {str(p.relative_to(source)): sha(p) for dll in ('combase', 'ole32', 'oleaut32', 'rpcrt4')
         for p in sorted((source/f'dlls/{dll}/tests').rglob('*')) if p.is_file()}
implementation = {n: sha(source/'dlls/combase'/n) for n in ('combase_private.h', 'rpc.c', 'stubmanager.c')}
inputs = dict(schema=1, variant=variant,
              commit=subprocess.check_output(['git','-C',str(repo),'rev-parse','HEAD'],text=True).strip(),
              wine_commit='7298a818b79446bae153b8d5aa27235ab3703bf9',
              archive_sha256='9f516d63e9f814f4dad70ce0a1943211b8a6236008ac3025cf58004091c661db',
              production={str(p.relative_to(repo)): sha(p) for p in [repo/'patches/series'] + [repo/'patches'/n for n in series]},
              candidate=sha(candidate), tests=tests, implementation=implementation,
              omitted=[name] if variant == 'without' else [],
              policy='Same final tests and production series; only target implementation omitted in without.')
Path(output).write_text(json.dumps(inputs,indent=2,sort_keys=True)+'\n')
PY
printf 'Prepared %s in %s\n' "$variant" "$work"
if (( prepare )); then exit 0; fi
build="$work/build"
mkdir -- "$build"
cd "$build"
"$source_tree/configure" --enable-archs=x86_64,i386 --enable-tests \
    --with-x --with-wayland --with-vulkan --with-opengl --with-freetype --with-fontconfig \
    --with-gnutls --with-pulse --with-alsa --with-gstreamer --with-dbus --with-krb5 \
    > "$work/configure.log" 2>&1 || { tail -40 "$work/configure.log"; exit 1; }
for feature in SONAME_LIBX11 SONAME_LIBXRENDER SONAME_LIBFREETYPE SONAME_LIBFONTCONFIG; do
    grep -q "^#define $feature " include/config.h || { echo "Missing $feature"; exit 1; }
done
make -j"${JOBS:-$(nproc)}" > "$work/build.log" 2>&1 || { tail -60 "$work/build.log"; exit 1; }
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
    python3 "$here/scripts/com-stub-actctx-l2.py" run "$build" "$work/logs" "$output/results.json"
"$build/server/wineserver" -k
"$build/server/wineserver" -w
