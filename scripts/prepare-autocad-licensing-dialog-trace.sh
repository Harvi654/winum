#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
set -euo pipefail
umask 077

if [[ $# != 2 ]]; then
    echo 'Usage: prepare-autocad-licensing-dialog-trace.sh SESSION_TEST_DIRECTORY BUILD_BUNDLE' >&2
    exit 2
fi
session="$(realpath -e -- "$1")"
bundle="$(realpath -e -- "$2")"
grep -Fxq 'wineserver_variant=services_session_report' "$session/COMBINED-BUILDINFO"
grep -Eq '^commit:[[:space:]]+aeb7a59[[:xdigit:]]*[[:space:]]*$' "$session/wine/BUILDINFO"
test -f "$session/candidate-prefix/drive_c/Program Files/Autodesk/AutoCAD 2027/acad.exe"
(
    cd "$bundle"
    sha256sum -c SHA256SUMS
    grep -Fx 'reference_commit=aeb7a59084c3fd84fa6aeaa1f0f16a551abafcb0' BUILDINFO
)

runtime_dll=lib/wine/x86_64-windows/comctl32_v6.dll
sxs_dll=drive_c/windows/winsxs/amd64_microsoft.windows.common-controls_6595b64144ccf1df_6.0.2600.2982_none_deadbeef/comctl32.dll
cmp -- "$session/wine/$runtime_dll" "$session/candidate-prefix/$sxs_dll"

work="$(mktemp -d "$(dirname -- "$session")/.acad-licensing-dialog-entry.XXXXXXXX")"
printf 'Test directory: %s\n' "$work"
cp -aT --reflink=auto -- "$session/candidate-prefix" "$work/candidate-prefix"
cp -aT --reflink=auto -- "$session/wine" "$work/wine"
test "$(realpath -e "$work/candidate-prefix/dosdevices/c:")" = "$work/candidate-prefix/drive_c"
for target in "$work/wine/$runtime_dll" "$work/candidate-prefix/$sxs_dll"; do
    directory="$(dirname -- "$target")"
    test "$(realpath -e "$directory")" = "$directory"
    cp --remove-destination -- "$bundle/comctl32_v6.dll" "$target"
    cmp -- "$bundle/comctl32_v6.dll" "$target"
done
{
    cat "$session/COMBINED-BUILDINFO"
    printf 'dialog_origin_trace=1\ncomctl32_v6_sha256=%s\n' \
        "$(sha256sum <"$bundle/comctl32_v6.dll" | cut -d ' ' -f1)"
} > "$work/COMBINED-BUILDINFO"

printf 'bash %q %q\n' "$bundle/trace-autocad-licensing-service.sh" "$work"
