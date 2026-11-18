#!/usr/bin/env bash
# Original AI-assisted (Codex), LGPL-2.1-or-later. Own MEASURE programs only.
set -euo pipefail
umask 022
[[ $# == 1 ]] || { echo 'Usage: build-measure-ws2_32-afunix-v4.sh NEW_OUTPUT' >&2; exit 2; }
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
output="$(realpath -m -- "$1")"
src="$here/docs/wine-repro/ws2_32-afunix"
[[ ! -e "$output" && ! -L "$output" ]] || { echo 'Use a new output directory' >&2; exit 2; }
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT

for arch in x86_64 i686; do
    "$arch-w64-mingw32-gcc" -O2 -static -Wall -Wextra -Werror -municode "$src/measure-v4.c" \
        -o "$work/afunix-v4-$arch.exe" -lws2_32 -ladvapi32
    "$arch-w64-mingw32-gcc" -O2 -static -Wall -Wextra -Werror "$src/host-peer.c" \
        -o "$work/host-peer-$arch.exe" -lws2_32
done

mkdir -- "$output"
cp "$work/"*.exe "$src/"{measure-v4.c,host-peer.c,run-host-peer.py,v4.md} "$here/LICENSE" "$output/"
sed 's/$/\r/' "$src/run-measure-v4.bat" > "$output/run-measure.bat"
(cd "$output" && sha256sum ./*.exe ./*.c run-host-peer.py v4.md LICENSE run-measure.bat > SHA256SUMS)
echo "MEASURE bundle: $output"
