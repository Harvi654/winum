#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
set -euo pipefail
umask 077

if [[ $# -lt 3 || $# -gt 4 || ( $# == 4 && "$4" != --candidate-only ) ]]; then
    echo 'Usage: prepare-autocad-system-wine-ab.sh PREVIOUS_DWG_TEST_DIR COMPARISON_WINE_ROOT EMPTY_AB_DIR [--candidate-only]' >&2
    exit 2
fi

previous="$(realpath -e -- "$1")"
system="$(realpath -e -- "$2")"
work="$(realpath -e -- "$3")"
source_prefix="$previous/base-prefix"
reference="$(realpath -e -- "$(cat -- "$previous/base-runtime.txt")")"

test -d "$work"
if [[ -n "$(find "$work" -mindepth 1 -maxdepth 1 -print -quit)" ]]; then
    echo 'Use an empty directory for the A/B test.' >&2
    exit 2
fi
if [[ "$(realpath -e -- "$reference/bin/wine")" == "$(realpath -e -- "$system/bin/wine")" ]]; then
    echo 'The reference and system runtime resolve to the same Wine.' >&2
    exit 2
fi
for runtime in "$reference" "$system"; do
    for program in wine wineboot wineserver; do
        test -x "$runtime/bin/$program"
    done
done
test -f "$source_prefix/drive_c/Program Files/Autodesk/AutoCAD 2027/acad.exe"
test "$(realpath -e -- "$source_prefix/dosdevices/c:")" = "$source_prefix/drive_c"

framework_files=(
    'drive_c/windows/system32/mscoree.dll'
    'drive_c/windows/syswow64/mscoree.dll'
    'drive_c/windows/Microsoft.NET/Framework64/v4.0.30319/clr.dll'
    'drive_c/windows/Microsoft.NET/Framework/v4.0.30319/clr.dll'
    'drive_c/Program Files/dotnet/shared/Microsoft.NETCore.App/10.0.15/coreclr.dll'
)
(
    cd "$source_prefix"
    for file in "${framework_files[@]}"; do
        test -s "$file"
        test "$(realpath -e -- "$file")" = "$source_prefix/$file"
    done
    for shim in "${framework_files[@]:0:2}"; do
        if grep -aq 'WINE_MONO_OVERRIDES' "$shim"; then
            echo 'The source test prefix contains a Wine Mono shim.' >&2
            exit 2
        fi
    done
    sha256sum -- "${framework_files[@]}"
) > "$work/dotnet-files.sha256"

printf 'winum_wine=%s\n' "$("$reference/bin/wine" --version)"
if [[ $# == 4 ]]; then
    printf 'candidate_wine=%s\n' "$("$system/bin/wine" --version)"
    if [[ -f "$system/BUILDINFO" ]]; then
        awk '$1 == "commit:" {print "candidate_commit=" $2}' "$system/BUILDINFO"
    fi
else
    printf 'system_wine=%s\n' "$("$system/bin/wine" --version)"
fi
if [[ -f "$reference/BUILDINFO" ]]; then
    awk '$1 == "commit:" {print "winum_commit=" $2}' "$reference/BUILDINFO"
fi

prepare_prefix() (
    local trial="$1" runtime="$2" prefix="$work/$1-prefix" log="$work/$1-wineboot.txt"

    cp -aT --reflink=auto -- "$source_prefix" "$prefix"
    test "$(realpath -e -- "$prefix/dosdevices/c:")" = "$prefix/drive_c"
    test ! -L "$prefix/.update-timestamp"
    export WINEPREFIX="$prefix"
    export PATH="$runtime/bin:$PATH"
    export WINE="$runtime/bin/wine"
    export WINELOADER="$WINE"
    export WINESERVER="$runtime/bin/wineserver"
    export WINEDEBUG=-all
    # wineboot refreshes Wine's prefix state. The CLR shim stays disabled
    # during that update so the builtin shim cannot install Wine Mono.
    export WINEDLLOVERRIDES="${WINEDLLOVERRIDES:+$WINEDLLOVERRIDES;}mscoree=d"
    if ! timeout 120 "$runtime/bin/wineboot" -u > "$log" 2>&1; then
        "$WINESERVER" -k >> "$log" 2>&1 || true
        printf 'prefix_update=%s status=failed\n' "$trial" >&2
        exit 1
    fi
    "$WINESERVER" -k >> "$log" 2>&1 || true
    timeout 20 "$WINESERVER" -w >> "$log" 2>&1
    printf 'disable\n' > "$prefix/.update-timestamp"
    cd "$prefix"
    if ! sha256sum --status -c "$work/dotnet-files.sha256"; then
        printf 'dotnet_files=%s status=changed\n' "$trial" >&2
        exit 1
    fi
    printf 'dotnet_files=%s status=unchanged\n' "$trial"
)

if [[ $# == 3 ]]; then
    prepare_prefix base "$reference"
fi
prepare_prefix candidate "$system"
ln -s -- "$system" "$work/wine"
printf '%s\n' "$reference" > "$work/base-runtime.txt"
if [[ $# == 4 ]]; then
    echo 'candidate_prefix=ready'
else
    echo 'system_wine_ab=ready'
fi
