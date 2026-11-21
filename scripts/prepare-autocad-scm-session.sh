#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
set -euo pipefail
umask 077

if [[ $# != 2 ]]; then
    echo 'Usage: prepare-autocad-scm-session.sh BASELINE_TEST_DIRECTORY BUILD_BUNDLE' >&2
    exit 2
fi
baseline="$(realpath -e -- "$1")"
bundle="$(realpath -e -- "$2")"
grep -Fxq 'variant=baseline' "$baseline/COMBINED-BUILDINFO"
grep -Eq '^commit:[[:space:]]+aeb7a59[[:xdigit:]]*[[:space:]]*$' "$baseline/wine/BUILDINFO"
test -f "$baseline/candidate-prefix/drive_c/Program Files/Autodesk/AutoCAD 2027/acad.exe"
(
    cd "$bundle"
    sha256sum -c SHA256SUMS
    grep -Fx 'reference_commit=aeb7a59084c3fd84fa6aeaa1f0f16a551abafcb0' BUILDINFO
)

work="$(mktemp -d "$(dirname -- "$baseline")/.acad-licensing-session.XXXXXXXX")"
printf 'Testovací priečinok: %s\n' "$work"
cp -aT --reflink=auto -- "$baseline/candidate-prefix" "$work/candidate-prefix"
cp -aT --reflink=auto -- "$(realpath -e "$baseline/wine")" "$work/wine"
test "$(realpath -e "$work/candidate-prefix/dosdevices/c:")" = "$work/candidate-prefix/drive_c"
test "$(realpath -e "$work/wine/bin")" = "$work/wine/bin"
cp --remove-destination -- "$bundle/candidate/wineserver" "$work/wine/bin/wineserver"
chmod 0755 -- "$work/wine/bin/wineserver"
cmp -- "$bundle/candidate/wineserver" "$work/wine/bin/wineserver"
{
    cat "$baseline/COMBINED-BUILDINFO"
    printf 'wineserver_variant=services_session_report\nwineserver_sha256=%s\n' \
        "$(sha256sum <"$work/wine/bin/wineserver" | cut -d ' ' -f1)"
} > "$work/COMBINED-BUILDINFO"

printf 'bash %q %q\n' "$HOME/Downloads/trace-autocad-licensing-scm.sh" "$work"
