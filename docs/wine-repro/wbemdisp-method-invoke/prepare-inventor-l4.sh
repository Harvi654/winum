#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
# Create a separate Inventor prefix and a candidate runtime next to it.
set -euo pipefail
if [[ $# != 4 ]]; then
    echo 'Usage: prepare-inventor-l4.sh CLOSED_2027_2_PREFIX BASE_WINUM_TARBALL OVERLAY NEW_DIRECTORY' >&2
    exit 2
fi
prefix="$(realpath -e -- "$1")"
archive="$(realpath -e -- "$2")"
overlay="$(realpath -e -- "$3")"
out="$(realpath -m -- "$4")"
case "$out/" in
    "$prefix/"*) echo 'Choose a destination outside the source prefix.' >&2; exit 2 ;;
esac
if [[ -e "$out" || -L "$4" || ! -f "$prefix/system.reg" ]] ||
   [[ "$(realpath -e "$prefix/dosdevices/c:")" != "$prefix/drive_c" ]]; then
    echo 'Use a Wine prefix with its own C: drive and a fresh destination.' >&2
    exit 2
fi
printf '%s  %s\n' 0523b2615596cbb66e7e9ed95a5024008a25f5141ba2e4483e862a47db0c9980 \
    "$archive" | sha256sum -c -
(cd "$overlay" && sha256sum -c SHA256SUMS)

runtime="$out/winum-11.22-2-x86_64"
candidate_prefix="$out/prefix"
mkdir -- "$out"
tar -xJf "$archive" -C "$out"
cp -aT --reflink=auto -- "$prefix" "$candidate_prefix"
[[ "$(realpath -e "$candidate_prefix/dosdevices/c:")" == "$candidate_prefix/drive_c" ]]

install_dll() {
    local source="$1" target="$2" directory
    directory="$(dirname "$target")"
    [[ "$(realpath -e "$directory")" == "$directory" ]]
    cp --remove-destination -- "$source" "$target"
    cmp -- "$source" "$target"
}
for arch in x86_64 i386; do
    system=system32
    if [[ "$arch" == i386 ]]; then system=syswow64; fi
    source_dll="$overlay/$arch-windows/wbemdisp.dll"
    install_dll "$source_dll" "$runtime/lib/wine/$arch-windows/wbemdisp.dll"
    install_dll "$source_dll" "$candidate_prefix/drive_c/windows/$system/wbemdisp.dll"
done
printf 'L4 directory: %s\n' "$out"
printf 'Prefix: %s\nRuntime: %s\n' "$candidate_prefix" "$runtime"
