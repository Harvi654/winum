#!/usr/bin/env bash
# Run the same Windows MEASURE bundle under an explicitly selected Wine.
set -euo pipefail
wine="$(realpath -e -- "${1:?Wine loader required}")"
export WINESERVER="$(realpath -e -- "${2:?wineserver required}")"
bundle="$(realpath -e -- "${3:?MEASURE bundle directory required}")"
out="$(realpath -m -- "${4:?Result directory required}")"
mkdir -- "$out"
(cd "$bundle" && sha256sum -c SHA256SUMS)
export WINEDEBUG=-all WINEDLLOVERRIDES='mscoree,mshtml='
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
cd "$(dirname "$wine")"
for arch in x86_64 i686; do
    (
        export WINEPREFIX="$work/prefix-$arch"
        trap 'timeout -k 5s 15s "$WINESERVER" -k 2>/dev/null || true' EXIT
        timeout -k 5s 120s "$wine" wineboot -i > "$out/boot-$arch.log" 2>&1 ||
            { cat "$out/boot-$arch.log"; exit 1; }
        timeout -k 5s 60s "$WINESERVER" -w
        timeout -k 5s 90s "$wine" "$bundle/stale-bits-$arch.exe" |
            tr -d '\r' > "$out/result-$arch.txt"
        grep -Fxq 'measure_complete cases=12' "$out/result-$arch.txt"
        [[ $(grep -c ' step=after_pump ' "$out/result-$arch.txt") == 12 ]]
        if grep -q ' error=' "$out/result-$arch.txt"; then
            grep ' error=' "$out/result-$arch.txt"
            exit 1
        fi
    )
done
