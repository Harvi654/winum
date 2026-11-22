#!/usr/bin/env bash
# Original AI-assisted (Codex), LGPL-2.1-or-later. Own MEASURE program only.
# Builds the x86_64 and i686 MEASURE executables with sources and checksums.
set -euo pipefail
umask 022
[[ $# == 1 ]] || { echo 'Usage: build-measure-shell32-label-edit-enter.sh NEW_OUTPUT' >&2; exit 2; }
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
output="$(realpath -m -- "$1")"
src="$here/docs/wine-repro/shell32-label-edit-enter"
[[ ! -e "$output" && ! -L "$output" ]] || { echo 'Use a new output directory' >&2; exit 2; }
for arch in x86_64 i686; do
    command -v "$arch-w64-mingw32-gcc" >/dev/null || { echo "Missing $arch-w64-mingw32-gcc" >&2; exit 2; }
done
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
for arch in x86_64 i686; do
    "$arch-w64-mingw32-gcc" -O2 -static -Wall -Wextra -Werror -municode "$src/measure.c" \
        -o "$work/measure-$arch.exe" -lshell32 -lole32 -luuid -luser32
done
mkdir -p -- "$(dirname -- "$output")"
mkdir -- "$output"
cp "$work/"*.exe "$src/measure.c" "$src/README.md" "$here/LICENSE" "$output/"
sed 's/$/\r/' "$src/run-measure.bat" > "$output/run-measure.bat"
(cd "$output" && sha256sum ./*.exe measure.c README.md LICENSE run-measure.bat > SHA256SUMS)
echo "MEASURE bundle: $output"
