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
for runner in run-measure.bat run-measure-high.bat; do
    sed "s/\$/\\r/" "$here/$runner" > "$out/$runner"
done
(cd -- "$out" && sha256sum hkcr-read-x86_64.exe hkcr-read-i686.exe run-measure.bat run-measure-high.bat > SHA256SUMS)
