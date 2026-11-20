#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
set -euo pipefail
umask 077

if [[ $# != 3 ]]; then
    echo 'Usage: prepare-autocad-combined-dialog.sh CRT_AB_DIR SHELL32_BUNDLE EMPTY_TEST_DIR' >&2
    exit 2
fi

previous="$(realpath -e -- "$1")"
bundle="$(realpath -e -- "$2")"
work="$(realpath -e -- "$3")"
source_prefix="$previous/candidate-prefix"
source_runtime="$previous/wine"
crt_sha=27705423171ce425d9d0a17c2853bbe5596ba5d373f2766d37db892474234325
shell_commit=12294c22bcd94ca23d7d268df27bd26bdf10b347

if [[ ! -d "$work" || -n "$(find "$work" -mindepth 1 -maxdepth 1 -print -quit)" ]]; then
    echo 'Use an empty directory for the combined dialog test.' >&2
    exit 2
fi
test -s "$previous/base-runtime.txt"
test -f "$source_prefix/drive_c/Program Files/Autodesk/AutoCAD 2027/acad.exe"
test "$(realpath -e -- "$source_prefix/dosdevices/c:")" = "$source_prefix/drive_c"
test -x "$source_runtime/bin/wine"
grep -Eq '^commit:[[:space:]]+aeb7a59[[:xdigit:]]*[[:space:]]*$' "$source_runtime/BUILDINFO"
printf '%s  %s\n' \
    "$crt_sha" "$source_runtime/lib/wine/x86_64-windows/ucrtbase.dll" \
    "$crt_sha" "$source_prefix/drive_c/windows/system32/ucrtbase.dll" | sha256sum -c -
(
    cd "$bundle"
    sha256sum -c SHA256SUMS
    grep -Fx "Source https://github.com/Harvi654/winum/tree/$shell_commit" BUILDINFO
)

prefix="$work/candidate-prefix"
runtime="$work/wine"
echo 'Copying the successful CRT test prefix and runtime.'
cp -aT --reflink=auto -- "$source_prefix" "$prefix"
cp -aT --reflink=auto -- "$source_runtime" "$runtime"
test "$(realpath -e -- "$prefix/dosdevices/c:")" = "$prefix/drive_c"

for arch in x86_64 i386; do
    case "$arch" in x86_64) system=system32 ;; i386) system=syswow64 ;; esac
    runtime_dir="$runtime/lib/wine/$arch-windows"
    prefix_dir="$prefix/drive_c/windows/$system"
    test "$(realpath -e -- "$runtime_dir")" = "$runtime_dir"
    test "$(realpath -e -- "$prefix_dir")" = "$prefix_dir"
    cp --remove-destination -- "$bundle/$arch-windows/shell32.dll" "$runtime_dir/shell32.dll"
    cp --remove-destination -- "$bundle/$arch-windows/shell32.dll" "$prefix_dir/shell32.dll"
    cmp -- "$runtime_dir/shell32.dll" "$prefix_dir/shell32.dll"
done

cp -- "$previous/base-runtime.txt" "$work/base-runtime.txt"
{
    printf 'reference_commit=aeb7a59\nucrtbase_x64_sha256=%s\nshell32_candidate_commit=%s\n' "$crt_sha" "$shell_commit"
    (cd "$runtime" && sha256sum lib/wine/{x86_64,i386}-windows/shell32.dll)
} > "$work/COMBINED-BUILDINFO"
echo 'autocad_combined_dialog=ready'
