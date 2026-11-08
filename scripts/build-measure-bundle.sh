#!/usr/bin/env bash
# Builds the programs listed in docs/wine-repro/measure-bundle.list for
# x86_64 and i686 with MinGW into one folder for the Windows VM, with
# run-measure.bat, a README and SHA256SUMS. Text results only come back.
# Usage: build-measure-bundle.sh OUTDIR [LIST]
# Original, AI-assisted (Claude), LGPL-2.1-or-later.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="${1:?usage: build-measure-bundle.sh OUTDIR [LIST]}"
LIST="${2:-$HERE/docs/wine-repro/measure-bundle.list}"
[ -e "$OUT" ] && { echo "$OUT exists; choose a new folder" >&2; exit 1; }
mkdir -p "$OUT"
bat="$OUT/run-measure.bat"
printf '%s\r\n' '@echo off' 'cd /d "%~dp0"' 'if exist exitcodes.txt (echo already run - use a fresh copy & exit /b 2)' > "$bat"
names=""
while IFS='|' read -r name src libs; do
    name="$(echo "$name" | xargs)"; src="$(echo "$src" | xargs)"; libs="$(echo "$libs" | xargs)"
    case "$name" in ''|'#'*) continue ;; esac
    for arch in x86_64 i686; do
        exe="$name-$arch.exe"
        # shellcheck disable=SC2086
        "$arch-w64-mingw32-gcc" -O2 -Wall -Werror -o "$OUT/$exe" "$HERE/$src" $libs
        printf '%s\r\n' "echo running $exe" "$exe > result-$name-$arch.txt 2>&1" ">>exitcodes.txt echo $name $arch=%ERRORLEVEL%" >> "$bat"
    done
    names="$names $name"
done < "$LIST"
printf '%s\r\n' 'echo done' 'pause' >> "$bat"
cat > "$OUT/README.txt" <<INFO
winum MEASURE bundle, commit $(git -C "$HERE" rev-parse --short HEAD). Programs:$names
(sources in the winum repository, LGPL-2.1-or-later). No Wine or Microsoft files.
On Windows 10: copy this folder to a new directory, run run-measure.bat,
do not touch mouse or keyboard while it runs, return result-*.txt and
exitcodes.txt.
INFO
(cd "$OUT" && sha256sum ./* > SHA256SUMS)
echo "bundle: $OUT ($names )"
