#!/usr/bin/env bash
# Runs the MEASURE bundle under one Wine build in a fresh prefix (CI, Xvfb).
# Usage: run-wine.sh WINE WINESERVER BUNDLE OUTDIR
# Original, AI-assisted (Claude), LGPL-2.1-or-later.
set -euo pipefail
wine="$(realpath -e "${1:?Wine loader required}")"
export WINESERVER="$(realpath -e "${2:?wineserver required}")"
bundle="$(realpath -e "${3:?bundle directory required}")"
out="$(realpath -m "${4:?result directory required}")"
mkdir "$out"
(cd "$bundle" && sha256sum -c SHA256SUMS)
export WINEDEBUG=-all WINEDLLOVERRIDES='mscoree,mshtml='
work="$(mktemp -d)"
trap 'timeout -k 5s 20s "$WINESERVER" -k 2>/dev/null || true; rm -rf -- "$work"' EXIT
export WINEPREFIX="$work/prefix"
timeout -k 5s 120s "$wine" wineboot -i > "$out/boot.log" 2>&1 || { tail -n 40 "$out/boot.log"; exit 1; }
timeout -k 5s 60s "$WINESERVER" -w
cd "$bundle"
for arch in x86_64 i686; do
    status=0
    timeout -k 5s 300s "$wine" "$bundle/enddialog-nondialog-$arch.exe" > "$out/result-enddialog-nondialog-$arch.txt" 2>&1 || status=$?
    printf 'enddialog-nondialog %s=%s\n' "$arch" "$status" >> "$out/exitcodes.txt"
done
cat "$out/exitcodes.txt"
