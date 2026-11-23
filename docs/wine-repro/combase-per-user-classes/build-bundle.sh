#!/usr/bin/env bash
# Original MEASURE builder, AI-assisted (Codex), LGPL-2.1-or-later.
set -euo pipefail

probe="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "$probe/../../.." && pwd)"
out="$(realpath -m "${1:?Usage: build-bundle.sh NEW_OUTPUT}")"
bash "$repo/scripts/build-measure-bundle.sh" "$out" "$probe/measure.list"
for arch in x86_64 i686; do
    bits=64
    if [[ "$arch" == i686 ]]; then bits=32; fi
    "$arch-w64-mingw32-gcc" -shared -O2 -Wall -Werror -Wl,--kill-at -DMARKER="1$bits" \
        -o "$out/user-$bits.dll" "$probe/server.c" -luuid
    "$arch-w64-mingw32-gcc" -shared -O2 -Wall -Werror -Wl,--kill-at -DMARKER="2$bits" \
        -o "$out/machine-$bits.dll" "$probe/server.c" -luuid
done
cat >> "$out/README.txt" <<'INFO'

Run run-measure.bat from the normal Windows desktop. Accept the UAC prompt
for each matching helper so only the HKLM setup runs elevated. The helper
removes its temporary class after the activation sample.
INFO
(cd "$out" && sha256sum ./*.exe ./*.dll ./README.txt ./measure.manifest ./run-measure.bat > SHA256SUMS &&
    sha256sum -c SHA256SUMS)
