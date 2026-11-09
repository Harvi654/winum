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
resource_work="$(mktemp -d)"
trap 'rm -rf -- "$resource_work"' EXIT
# Original execution manifest, AI-assisted (Codex), LGPL-2.1-or-later.
# Explicit execution level avoids Windows' filename-based installer detection.
cat > "$resource_work/measure.manifest" <<'XML'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
  <assemblyIdentity name="Winum.Measure" version="1.0.0.0" processorArchitecture="*" type="win32"/>
  <trustInfo xmlns="urn:schemas-microsoft-com:asm.v3">
    <security><requestedPrivileges>
      <requestedExecutionLevel level="asInvoker" uiAccess="false"/>
    </requestedPrivileges></security>
  </trustInfo>
  <compatibility xmlns="urn:schemas-microsoft-com:compatibility.v1">
    <application><supportedOS Id="{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}"/></application>
  </compatibility>
</assembly>
XML
printf '%s\n' '1 24 "measure.manifest"' > "$resource_work/measure.rc"
for arch in x86_64 i686; do
    (cd "$resource_work" && "$arch-w64-mingw32-windres" -i measure.rc -o "measure-$arch.o")
done
cp "$resource_work/measure.manifest" "$OUT/"
bat="$OUT/run-measure.bat"
printf '%s\r\n' '@echo off' 'cd /d "%~dp0"' 'if exist exitcodes.txt (echo already run - use a fresh copy & exit /b 2)' > "$bat"
names=""
while IFS='|' read -r name src libs; do
    name="$(echo "$name" | xargs)"; src="$(echo "$src" | xargs)"; libs="$(echo "$libs" | xargs)"
    case "$name" in ''|'#'*) continue ;; esac
    for arch in x86_64 i686; do
        exe="$name-$arch.exe"
        # shellcheck disable=SC2086
        "$arch-w64-mingw32-gcc" -O2 -Wall -Werror -o "$OUT/$exe" "$HERE/$src" "$resource_work/measure-$arch.o" $libs
        printf '%s\r\n' "echo running $exe" "$exe > result-$name-$arch.txt 2>&1" ">>exitcodes.txt echo $name $arch=%ERRORLEVEL%" >> "$bat"
    done
    names="$names $name"
done < "$LIST"
printf '%s\r\n' 'echo done' 'pause' >> "$bat"
cat > "$OUT/README.txt" <<INFO
winum MEASURE bundle, commit $(git -C "$HERE" rev-parse --short HEAD). Programs:$names
(sources in the winum repository, LGPL-2.1-or-later). No Wine or Microsoft files.
EXEs embed the supplied original measure.manifest: asInvoker, Windows 10.
No compatibility mode or administrator relaunch is requested.
On Windows 10: copy this folder to a new directory, run run-measure.bat,
do not touch mouse or keyboard while it runs, return result-*.txt and
exitcodes.txt.
INFO
(cd "$OUT" && sha256sum ./* > SHA256SUMS)
echo "bundle: $OUT ($names )"
