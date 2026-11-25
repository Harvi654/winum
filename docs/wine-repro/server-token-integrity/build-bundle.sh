#!/usr/bin/env bash
# Original, AI-assisted (Claude), LGPL-2.1-or-later.
# Build the token integrity MEASURE bundle (x86_64 and i686) into a new directory.
#   bash docs/wine-repro/server-token-integrity/build-bundle.sh OUT
# Uses <arch>-w64-mingw32-gcc, or <arch>-w64-mingw32-clang when CC_KIND=clang.
set -euo pipefail
here=$(cd -- "$(dirname -- "$0")" && pwd)
out=$(realpath -m -- "${1:?output directory}")
mkdir -p -- "$out"
for arch in x86_64 i686; do
    "$arch-w64-mingw32-${CC_KIND:-gcc}" -Wall -Wextra -Werror -Wno-cast-function-type -O2 -static \
        "$here/measure.c" -ladvapi32 -o "$out/token-integrity-$arch.exe"
done
sed 's/$/\r/' "$here/run-measure.bat" > "$out/run-measure.bat"
cp -- "$here/measure.c" "$out/"
(cd -- "$out" && sha256sum token-integrity-x86_64.exe token-integrity-i686.exe run-measure.bat measure.c > SHA256SUMS)
