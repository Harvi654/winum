#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
set -euo pipefail
umask 077

die() {
    printf 'Dialog trace setup: %s\n' "$*" >&2
    exit 1
}

if [[ $# != 2 ]]; then
    echo 'Usage: prepare-autocad-licensing-dialog-trace.sh SESSION_TEST_DIRECTORY BUILD_BUNDLE' >&2
    exit 2
fi
session="$(realpath -e -- "$1")" || die 'The source test directory is unavailable.'
bundle="$(realpath -e -- "$2")" || die 'The diagnostic bundle directory is unavailable.'
runtime="$(realpath -e -- "$session/wine")" || die 'The source Wine runtime is unavailable.'
grep -Fxq 'wineserver_variant=services_session_report' "$session/COMBINED-BUILDINFO" ||
    die 'The source test manifest does not identify the SCM candidate.'
grep -Eq '^commit:[[:space:]]+aeb7a59[[:xdigit:]]*[[:space:]]*$' "$runtime/BUILDINFO" ||
    die 'The source Wine runtime does not match the diagnostic reference.'
test -f "$session/candidate-prefix/drive_c/Program Files/Autodesk/AutoCAD 2027/acad.exe" ||
    die 'AutoCAD 2027 is missing from the source test prefix.'
(
    cd "$bundle" &&
    sha256sum -c SHA256SUMS &&
    grep -Fx 'reference_commit=aeb7a59084c3fd84fa6aeaa1f0f16a551abafcb0' BUILDINFO
) >&2 || die 'The diagnostic bundle failed verification.'

runtime_dll=lib/wine/x86_64-windows/comctl32_v6.dll
sxs_dll=drive_c/windows/winsxs/amd64_microsoft.windows.common-controls_6595b64144ccf1df_6.0.2600.2982_none_deadbeef/comctl32.dll
cached="$session/candidate-prefix/$sxs_dll"
test -r "$runtime/$runtime_dll" || die 'The reference Wine runtime is missing its x64 Common Controls DLL.'
test -r "$cached" || die 'The source prefix is missing its x64 WinSxS Common Controls DLL.'
cache=matches_runtime
if ! cmp -s -- "$runtime/$runtime_dll" "$cached"; then
    # Wine's PE loader identifies its builtin DLLs by this DOS stub marker.
    printf 'Wine builtin DLL' | cmp -s -n 16 -i 0:64 -- - "$cached" ||
        die 'The differing WinSxS Common Controls DLL lacks the Wine builtin marker.'
    cache=different_wine_builtin
    printf 'The source WinSxS DLL differs from the runtime. Using the diagnostic DLL in the test copy.\n' >&2
fi

work="$(mktemp -d "$(dirname -- "$session")/.acad-licensing-dialog-entry.XXXXXXXX")"
printf 'Test directory: %s\n' "$work"
cp -aT --reflink=auto -- "$session/candidate-prefix" "$work/candidate-prefix"
cp -aT --reflink=auto -- "$runtime" "$work/wine"
test "$(realpath -e "$work/candidate-prefix/dosdevices/c:")" = "$work/candidate-prefix/drive_c" ||
    die 'The copied C: drive does not point inside the test prefix.'
for target in "$work/wine/$runtime_dll" "$work/candidate-prefix/$sxs_dll"; do
    directory="$(dirname -- "$target")"
    test "$(realpath -e "$directory")" = "$directory" ||
        die "The DLL destination resolves outside its expected directory: $directory"
    cp --remove-destination -- "$bundle/comctl32_v6.dll" "$target"
    cmp -- "$bundle/comctl32_v6.dll" "$target"
done
{
    cat "$session/COMBINED-BUILDINFO"
    printf 'comctl32_v6_source_cache=%s\n' "$cache"
    printf 'dialog_origin_trace=1\ncomctl32_v6_sha256=%s\n' \
        "$(sha256sum <"$bundle/comctl32_v6.dll" | cut -d ' ' -f1)"
} > "$work/COMBINED-BUILDINFO"

printf 'bash %q %q\n' "$bundle/trace-autocad-licensing-service.sh" "$work"
