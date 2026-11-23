#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
set -euo pipefail
here=$(cd -- "$(dirname -- "$0")" && pwd)
out=$(realpath -m -- "$1")
mkdir -p -- "$out"
for arch in x86_64 i686; do
    "$arch-w64-mingw32-clang" -Wall -Wextra -Werror -O2 -municode \
        "$here/measure.c" -lole32 -lshell32 -ladvapi32 -o "$out/hkcr-read-$arch.exe"
done
sed "s/\$/\\r/" "$here/run-measure.bat" > "$out/run-measure.bat"
(cd -- "$out" && sha256sum hkcr-read-x86_64.exe hkcr-read-i686.exe run-measure.bat > SHA256SUMS)
