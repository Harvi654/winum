#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
set -euo pipefail
umask 077

[[ $# == 1 && -s "$1" && -f "$1" ]] || { echo 'Usage: run-autocad-managed-probes.sh DWG' >&2; exit 2; }
bundle="$(dirname -- "$(realpath -e -- "$0")")"
work="$(realpath -e -- "${ACAD_AUTOCOMPLETE_WORK:?Set the AutoCAD test directory}")"
prefix="$work/base-prefix"
test "$(realpath -e -- "$prefix/drive_c")" = "$prefix/drive_c"
test -f "$prefix/drive_c/Program Files/Autodesk/AutoCAD 2027/acad.exe"
test -s "$work/base-runtime.txt"
runtime="$(< "$work/base-runtime.txt")"
test -x "$runtime/bin/wine"
test -x "$runtime/bin/wineserver"
(cd -- "$bundle" && sha256sum --quiet -c SHA256SUMS)

host='C:\Program Files\dotnet\dotnet.exe'
dotnet="$prefix/drive_c/Program Files/dotnet"
if [[ ! -f "$dotnet/dotnet.exe" ]]; then
    echo 'probe_host=missing'
    exit 2
fi

stage="$(mktemp -d "$prefix/drive_c/winum-managed-open.XXXXXXXX")"
logdir="$(mktemp -d "$work/managed-open-logs.XXXXXXXX")"
cp -- "$1" "$stage/probe.dwg"
cp -- "$bundle"/{console,wpf}{.dll,.runtimeconfig.json} "$stage/"
stage_win="C:\\${stage##*/}"

export WINEPREFIX="$prefix"
export PATH="$runtime/bin:$PATH"
export WINE="$runtime/bin/wine"
export WINELOADER="$WINE"
export WINESERVER="$runtime/bin/wineserver"
export WINEDEBUG='-all,+pid,dotnet.exe:+seh,dotnet.exe:+loaddll,dotnet.exe:+module'

failed=0
for probe in console wpf; do
    framework=Microsoft.NETCore.App
    if [[ "$probe" == wpf ]]; then framework=Microsoft.WindowsDesktop.App; fi
    if [[ ! -d "$dotnet/shared/$framework/10.0.0" ]]; then
        printf 'probe=%s framework_missing=%s/10.0.0\n' "$probe" "$framework"
        failed=1
        continue
    fi
    rc=0
    timeout -k 5s 40s "$WINE" "$host" "$stage_win\\$probe.dll" "$stage_win\\probe.dwg" \
        > "$logdir/$probe.txt" 2>&1 || rc=$?
    printf 'probe=%s exit=%s\n' "$probe" "$rc"
    if (( rc != 0 )); then failed=1; fi

    python3 - "$logdir/$probe.txt" <<'PY'
import re
import sys

message = re.compile(r"probe_(?:runtime=\.NET [0-9.]+|stage=[a-z_]+|dwg_bytes=[0-9]+|"
                     r"exception=[A-Za-z0-9_.+]+ hresult=0x[0-9a-f]{8})")
mapping = re.compile(r'^([0-9a-f]+):[0-9a-f]+:trace:module:map_image_into_view '
                     r'mapping PE file L?"[^"]*[\\/]([a-z0-9_.-]+)" at (0x[0-9a-f]+)-(0x[0-9a-f]+)', re.I)
unhandled = re.compile(r"^([0-9a-f]+):[0-9a-f]+:.*Unhandled exception code ([0-9a-f]+) "
                       r"flags [0-9a-f]+ addr (?:0x)?([0-9a-f]+)", re.I)
regions = {}
with open(sys.argv[1], encoding="utf-8", errors="replace") as stream:
    for line in stream:
        line = line.strip()
        if message.fullmatch(line):
            print(line)
        elif match := mapping.match(line):
            pid, image, base, end = match.groups()
            regions[pid, int(base, 16)] = (image.lower(), int(end, 16))
        elif match := unhandled.match(line):
            pid, code, address = match.groups()
            ip = int(address, 16)
            images = [(name, ip - base) for (process, base), (name, end) in regions.items()
                      if process == pid and base <= ip < end]
            module = "unknown"
            if len(images) == 1:
                name, offset = images[0]
                module = f"{name}+0x{offset:x}"
            print(f"probe_exception_code=0x{code} probe_fault_module={module}")
PY
done
exit "$failed"
