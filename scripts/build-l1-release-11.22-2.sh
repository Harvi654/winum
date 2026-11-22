#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
set -euo pipefail
repo="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
out="$(realpath -m -- "${1:?New bundle directory required}")"
[[ ! -e "$out" && ! -L "$out" ]] || { echo 'Choose a new output directory' >&2; exit 2; }
source "$repo/wine/version.env"
WINE_PINNED_VERSION="$WINE_VERSION"
source "$repo/scripts/lib/wine-source.sh"
source "$repo/scripts/lib/wine-patches.sh"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
tarball="${2:-$work/wine-$WINE_VERSION.tar.xz}"
if [[ $# == 1 ]]; then
    curl -fLsS --retry 3 -o "$tarball" "https://dl.winehq.org/wine/source/${WINE_VERSION%%.*}.x/wine-$WINE_VERSION.tar.xz"
fi
wine_verify_tarball "$tarball" "$WINE_VERSION" >/dev/null
tar -xf "$tarball" -C "$work"
src="$work/wine-$WINE_VERSION"

bash "$repo/scripts/stage-11.22-2-candidates.sh" "$work/patches"
wine_apply_patches "$src" "$work/patches" "$repo" HEAD
patch -d "$src" -p1 --fuzz=0 < "$repo/patches/tests-first/ucrtbase-tests-cxx-exception-size.patch"

mkdir "$work/build"
cd "$work/build"
"$src/configure" --enable-archs=x86_64,i386 --enable-tests --without-x     --without-wayland --without-vulkan --without-opengl --without-freetype --without-fontconfig     > "$work/configure.log" 2>&1 || { tail -n 60 "$work/configure.log"; exit 1; }
targets=()
for suite in ucrtbase user32; do
    for arch in x86_64 i386; do
        targets+=("dlls/$suite/tests/$arch-windows/${suite}_test.exe")
    done
done
make -j"${JOBS:-$(nproc)}" "${targets[@]}" > "$work/build.log" 2>&1 ||
    { tail -n 80 "$work/build.log"; exit 1; }
mkdir -p "$out"
for suite in ucrtbase user32; do
    case "$suite" in
        ucrtbase) unit=cpp; notes='The size assertion runs on x86_64 only. The i686 cpp unit checks the unaffected architecture.' ;;
        user32) unit=msg; notes='Leave the mouse and keyboard idle during both runs.' ;;
    esac
    bundle="$out/$suite"
    mkdir "$bundle"
    cp "$src/COPYING.LIB" "$bundle/"
    bat="$bundle/run-tests.bat"
    printf '%s\r\n' '@echo off' 'cd /d "%~dp0"' 'if exist exitcodes.txt (echo Use a fresh copy of this bundle. & exit /b 2)' > "$bat"
    for arch in x86_64 i386; do
        label="$arch"
        [[ $arch != i386 ]] || label=i686
        exe="$suite-$label.exe"
        cp "dlls/$suite/tests/$arch-windows/${suite}_test.exe" "$bundle/$exe"
        printf '%s\r\n' "echo Running $suite:$unit $label" \
            "$exe $unit > result-$suite-$unit-$label.txt 2>&1" \
            ">>exitcodes.txt echo $suite:$unit $label=%ERRORLEVEL%" >> "$bat"
    done
    printf '%s\r\n' 'type exitcodes.txt' >> "$bat"
    cat > "$bundle/README.txt" <<INFO
Wine $WINE_VERSION $suite:$unit, Winum test branch at $(git -C "$repo" rev-parse --short HEAD).
Verify SHA256SUMS before transferring this folder to a fresh location on the offline Windows 10 VM.
Run run-tests.bat from PowerShell or cmd. $notes
Return exitcodes.txt, the "tests executed" summary lines and any failure, todo
success, or skip lines from result-*.txt. Label the lines by their result filename.
Test sources: https://github.com/Harvi654/winum/tree/$(git -C "$repo" rev-parse HEAD)
INFO
    (cd "$bundle" && sha256sum -- ./* > SHA256SUMS && sha256sum -c SHA256SUMS)
done
