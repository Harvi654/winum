#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
set -euo pipefail
umask 077

if [[ $# != 3 ]]; then
    echo 'Usage: prepare-autocad-crt-size-ab.sh PREVIOUS_DWG_TEST_DIR BUILD_BUNDLE EMPTY_AB_DIR' >&2
    exit 2
fi

previous="$(realpath -e -- "$1")"
bundle="$(realpath -e -- "$2")"
work="$(realpath -e -- "$3")"
source_prefix="$previous/base-prefix"
reference="$(realpath -e -- "$(< "$previous/base-runtime.txt")")"
runtime_dll=lib/wine/x86_64-windows/ucrtbase.dll
prefix_dll=drive_c/windows/system32/ucrtbase.dll

if [[ ! -d "$work" || -n "$(find "$work" -mindepth 1 -maxdepth 1 -print -quit)" ]]; then
    echo 'Use an empty directory for the CRT comparison.' >&2
    exit 2
fi
test -f "$source_prefix/drive_c/Program Files/Autodesk/AutoCAD 2027/acad.exe"
test "$(realpath -e -- "$source_prefix/dosdevices/c:")" = "$source_prefix/drive_c"
test -x "$reference/bin/wine"
grep -Eq '^commit:[[:space:]]+aeb7a59[[:xdigit:]]*[[:space:]]*$' "$reference/BUILDINFO"
(
    cd "$bundle"
    sha256sum -c SHA256SUMS
    grep -Fx 'wine_version=11.22' BUILDINFO
    grep -Fx 'reference_commit=aeb7a59084c3fd84fa6aeaa1f0f16a551abafcb0' BUILDINFO
)
test -s "$bundle/baseline/ucrtbase.dll"
test -s "$bundle/candidate/ucrtbase.dll"
if cmp -s "$bundle/baseline/ucrtbase.dll" "$bundle/candidate/ucrtbase.dll"; then
    echo 'The two CRT variants are identical.' >&2
    exit 2
fi

for variant in baseline candidate; do
    case "$variant" in
        baseline) prefix="$work/base-prefix"; runtime="$work/reference" ;;
        candidate) prefix="$work/candidate-prefix"; runtime="$work/wine" ;;
    esac
    printf 'Copying the prefix and runtime for %s.\n' "$variant"
    cp -aT --reflink=auto -- "$source_prefix" "$prefix"
    cp -aT --reflink=auto -- "$reference" "$runtime"
    test "$(realpath -e -- "$prefix/dosdevices/c:")" = "$prefix/drive_c"
    test "$(realpath -e -- "$(dirname -- "$runtime/$runtime_dll")")" = "$(dirname -- "$runtime/$runtime_dll")"
    test "$(realpath -e -- "$(dirname -- "$prefix/$prefix_dll")")" = "$(dirname -- "$prefix/$prefix_dll")"
    cp --remove-destination -- "$bundle/$variant/ucrtbase.dll" "$runtime/$runtime_dll"
    cp --remove-destination -- "$bundle/$variant/ucrtbase.dll" "$prefix/$prefix_dll"
    cmp -- "$runtime/$runtime_dll" "$prefix/$prefix_dll"
    printf 'crt_variant=%s sha256=%s\n' "$variant" "$(sha256sum < "$runtime/$runtime_dll" | cut -d' ' -f1)"
done

printf '%s\n' "$work/reference" > "$work/base-runtime.txt"
echo 'crt_size_ab=ready'
