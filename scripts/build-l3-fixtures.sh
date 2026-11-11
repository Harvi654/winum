#!/usr/bin/env bash
# Unmodified fixtures; the RPC executables use the existing MEASURE builder.
set -euo pipefail
here=$(cd "$(dirname "$0")/.." && pwd)
out=${1:?output directory required}
mkdir -p "$out"
out=$(realpath "$out")
base="$here/docs/wine-repro"
x86_64-w64-mingw32-gcc -O2 -Wall -Wextra -Werror -municode -static -o "$out/putimage.exe" "$base/winex11-flush-putimage/repro.c" -lgdi32 -luser32
x86_64-w64-mingw32-gcc -O2 -Wall -Wextra -Werror -municode -static -o "$out/shape.exe" "$base/winex11-shape-alpha-xwayland/repro.c" -lgdi32 -luser32
x86_64-w64-mingw32-gcc -O2 -Wall -Wextra -Werror -static -o "$out/raw-input.exe" "$base/winex11-xinput-bad-device/repro.c" -luser32
cc -O2 -Wall -Wextra -Werror -shared -fPIC -o "$out/observe.so" "$base/winex11-flush-putimage/observe-x11.c" -ldl -pthread
cc -O2 -Wall -Wextra -Werror -shared -fPIC -o "$out/fault-xinput.so" "$base/winex11-xinput-bad-device/fault-xinput.c" -ldl -pthread
bash "$here/scripts/build-measure-bundle.sh" "$out/measure-bundle" "$base/rpcrt4-ndrstubcall3/l3.list"
git -C "$here" rev-parse HEAD > "$out/COMMIT"
(cd "$out" && find . -type f ! -name SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SHA256SUMS)
