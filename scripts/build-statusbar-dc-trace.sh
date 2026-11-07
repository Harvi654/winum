#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
# An isolated diagnostic Wine build; never edits the checkout's production series.
set -euo pipefail
umask 022
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
prepare=0
if [[ ${1:-} == --prepare-only ]]; then prepare=1; shift; fi
[[ $# == 1 ]] || { echo 'Usage: build-statusbar-dc-trace.sh [--prepare-only] NEW_OUTPUT' >&2; exit 2; }
output="$(realpath -m -- "$1")"
[[ ! -e "$output" && ! -L "$output" ]] || { echo 'Use a new output directory' >&2; exit 2; }
for tool in curl tar sha256sum patch python3; do command -v "$tool" >/dev/null; done
cache="${XDG_CACHE_HOME:-$HOME/.cache}/winum/downloads"
work="${WINE_SB_TRACE_WORK:-$(mktemp -d "${TMPDIR:-/tmp}/winum-sbdiag.XXXXXXXX")}"
if [[ -n ${WINE_SB_TRACE_WORK:-} ]]; then
    [[ ! -e "$work" && ! -L "$work" ]] || { echo 'WINE_SB_TRACE_WORK must be new' >&2; exit 2; }
    mkdir -p -- "$work"
fi
work="$(realpath -e -- "$work")"
# Wine's public mirror, exact wine-11.20 commit (not a moving tag).
revision=7298a818b79446bae153b8d5aa27235ab3703bf9
sum=9f516d63e9f814f4dad70ce0a1943211b8a6236008ac3025cf58004091c661db
archive="${WINE_SB_SOURCE_ARCHIVE:-$cache/wine-$revision.tar.gz}"
if [[ ! -f "$archive" ]]; then
    [[ -z ${WINE_SB_SOURCE_ARCHIVE:-} ]] || { echo 'WINE_SB_SOURCE_ARCHIVE is missing' >&2; exit 2; }
    mkdir -p -- "$cache"
    partial="$(mktemp "$cache/.wine-source.XXXXXXXX")"
    trap 'rm -f -- "${partial:-}"' EXIT
    curl -fL --retry 3 -o "$partial" "https://codeload.github.com/wine-mirror/wine/tar.gz/$revision"
    printf '%s  %s\n' "$sum" "$partial" | sha256sum -c -
    mv -- "$partial" "$archive"
fi
printf '%s  %s\n' "$sum" "$archive" | sha256sum -c -
mkdir -p -- "$output" "$work/pristine" "$work/recipe/scripts/lib" "$work/recipe/wine" "$work/recipe/patches"
tar -xf "$archive" -C "$work/pristine"
mv -- "$work/pristine/wine-$revision" "$work/pristine/wine-11.20"
source_tree="$work/pristine/wine-11.20"
[[ $(cat "$source_tree/VERSION") == 'Wine version 11.20' ]] || { echo 'Unexpected Wine version' >&2; exit 2; }
# The unchanged builder expects the official archive's directory name.
tar -C "$work/pristine" -cf "$work/wine-11.20.tar" wine-11.20
recipe="$work/recipe"
cp -- "$here/scripts/build-wine.sh" "$recipe/scripts/"
cp -- "$here/scripts/build-statusbar-dc-trace.sh" "$recipe/scripts/"
cp -- "$here/scripts/lib/wine-patches.sh" "$recipe/scripts/lib/"
cp -- "$here/patches/"*.patch "$here/patches/series" "$recipe/patches/"
cp -- "$here/patches/debug/statusbar-dc-trace/win32u-statusbar-dc-trace.patch" "$recipe/patches/"
printf '\n# Diagnostic only, outside the production series\nwin32u-statusbar-dc-trace.patch\n' >> "$recipe/patches/series"
cp -- "$here/wine/version.env" "$recipe/wine/"
cp -- "$here/LICENSE" "$recipe/"
source "$recipe/wine/version.env"
[[ $WINE_VERSION == 11.20 ]] || { echo 'This trace is for Wine 11.20 only' >&2; exit 2; }
printf '\nWINUM_BUILD=%s-sbdiag\n' "$WINUM_BUILD" >> "$recipe/wine/version.env"
name="winum-11.20-$WINUM_BUILD-sbdiag"
export WINUM_SB_CHECKOUT="$here"
export WINUM_SB_COMMIT="$(git -C "$here" rev-parse HEAD)"
# Adjust only the private copy: retain production patch dates/commit and describe
# the diagnostic archive honestly instead of inheriting release metadata.
python3 - "$recipe" <<'PY'
from pathlib import Path
import sys
recipe = Path(sys.argv[1])
p = recipe / 'scripts/build-wine.sh'
s = p.read_text()
old = 'wine_apply_patches "$SRC" "$HERE/patches" "$HERE" HEAD'
assert s.count(old) == 1
s = s.replace(old, 'wine_apply_patches "$SRC" "$HERE/patches" "$WINUM_SB_CHECKOUT" "$WINUM_SB_COMMIT"')
s = s.replace('git -C "$HERE" rev-parse --short HEAD',
              'git -C "$WINUM_SB_CHECKOUT" rev-parse --short "$WINUM_SB_COMMIT"')
start = s.index('source of this build - vanilla Wine')
end = s.index('INFO\n', start)
s = s[:start] + '''source of this diagnostic build is the adjacent -source.tar.xz artifact,
made by build-statusbar-dc-trace.sh from the actual compiled source tree.
This is not a release. See SBDIAG-INPUTS for the verified upstream archive;
the source SHA above identifies the local directory-name-normalised tar.
''' + s[end:]
s = s.replace('say "Packing"', 'cp "$HERE/SBDIAG-INPUTS" "$ROOT/SBDIAG-INPUTS"\nsay "Packing"')
p.write_text(s)
p = recipe / 'scripts/lib/wine-patches.sh'
s = p.read_text().replace('"patches/$name"',
    '"$(if [ "$name" = win32u-statusbar-dc-trace.patch ]; then echo patches/debug/statusbar-dc-trace/$name; else echo patches/$name; fi)"')
p.write_text(s)
PY
cat > "$recipe/SBDIAG-INPUTS" <<INFO
Diagnostic only; not a published release.
winum commit: $WINUM_SB_COMMIT
Wine mirror commit: $revision
Upstream archive: https://codeload.github.com/wine-mirror/wine/tar.gz/$revision
Upstream archive SHA-256: $sum
The builder input tar only normalises the top-level directory to wine-11.20.
The adjacent source artifact contains the actual compiled source, the private
build recipe and this wrapper. Normal patches/series was not edited.
INFO
source "$recipe/scripts/lib/wine-patches.sh"
wine_apply_patches "$source_tree" "$recipe/patches" "$here" HEAD
printf 'Prepared and checked with --fuzz=0: %s\n' "$work"
if (( prepare )); then exit 0; fi
export WORK="$work/build"
export WINE_VER_OVERRIDE=11.20
export WINE_SOURCE_TARBALL="$work/wine-11.20.tar"
bash "$recipe/scripts/build-wine.sh" "$output"
# Corresponding source from the actual build, with all notices; exclude build products.
mkdir -- "$work/source-package"
tar -C "$WORK" --exclude=wine-11.20/build -cf - wine-11.20 | tar -C "$work/source-package" -xf -
cp -a -- "$recipe" "$work/source-package/winum"
cp -- "$here/patches/debug/statusbar-dc-trace/README.md" "$work/source-package/README.md"
tar -C "$work/source-package" -cJf "$output/$name-source.tar.xz" wine-11.20 winum README.md
(cd "$output" && sha256sum "$name-source.tar.xz" > "$name-source.tar.xz.sha256")
printf 'Diagnostic binary and corresponding source: %s\n' "$output"
