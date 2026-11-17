#!/usr/bin/env bash
# Original AI-assisted (Claude), LGPL-2.1-or-later.
# Builds the MEASURE bundle docs/wine-repro/iphlpapi-adapters: our own
# measure.c for x86_64 and i686 (MinGW), a batch file that never
# overwrites results, the license and SHA256SUMS. The same EXEs run in
# the Windows VM and under Wine, so the two outputs can be diffed.
set -euo pipefail
umask 022
[[ $# == 1 ]] || { echo 'Usage: build-measure-iphlpapi-adapters.sh NEW_OUTPUT' >&2; exit 2; }
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
output="$(realpath -m -- "$1")"
src="$here/docs/wine-repro/iphlpapi-adapters/measure.c"
[[ ! -e "$output" && ! -L "$output" ]] || { echo 'Use a new output directory' >&2; exit 2; }
for arch in x86_64 i686; do
    command -v "$arch-w64-mingw32-gcc" >/dev/null || { echo "Missing $arch-w64-mingw32-gcc (pacman -S mingw-w64-gcc)" >&2; exit 2; }
done
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
for arch in x86_64 i686; do
    "$arch-w64-mingw32-gcc" -O2 -static -Wall -Wextra -Werror "$src" -o "$work/ia-measure-$arch.exe" -liphlpapi -lws2_32
done
mkdir -p -- "$output"
cp "$work/"*.exe "$src" "$here/LICENSE" "$output/"
printf '%s\r\n' '@echo off' 'cd /d "%~dp0"' \
    'if exist exitcodes.txt (echo already run - use a fresh copy & exit /b 2)' \
    'ia-measure-x86_64.exe > result-x86_64.txt 2>&1' '>>exitcodes.txt echo x86_64=%ERRORLEVEL%' \
    'ia-measure-i686.exe > result-i686.txt 2>&1' '>>exitcodes.txt echo i686=%ERRORLEVEL%' \
    'pause' > "$output/run-measure.bat"
cat > "$output/README.txt" <<'INFO'
MEASURE for winum (iphlpapi: GetAdaptersAddresses, GetIpAddrTable).
Original AI-assisted (Claude), LGPL-2.1-or-later. Only our own two EXEs,
their source, the license and a batch file. Prints no adapter names,
GUIDs, MAC or LAN addresses; loopback addresses in full, link-local ones
as "link-local", others as "other".

Windows 10: copy the folder to a new directory, run run-measure.bat,
return result-x86_64.txt, result-i686.txt and exitcodes.txt.
Wine: run the same EXEs in the prefix (see the README in the repository).
INFO
(cd "$output" && sha256sum ./*.exe ./*.c LICENSE README.txt run-measure.bat > SHA256SUMS)
echo "MEASURE bundle: $output"
