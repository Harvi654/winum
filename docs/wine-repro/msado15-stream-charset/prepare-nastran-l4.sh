#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
# Prepare an isolated Nastran trial with both candidate DLLs.
set -euo pipefail
if [[ $# != 5 ]]; then
    echo 'Usage: prepare-nastran-l4.sh CLOSED_2027_2_PREFIX BASE_WINUM_TARBALL WMI_OVERLAY ADO_OVERLAY NEW_DIRECTORY' >&2
    exit 2
fi
prefix="$(realpath -e -- "$1")"
archive="$(realpath -e -- "$2")"
wmi="$(realpath -e -- "$3")"
ado="$(realpath -e -- "$4")"
out="$(realpath -m -- "$5")"
case "$out/" in
    "$prefix/"*) echo 'Choose a destination outside the source prefix.' >&2; exit 2 ;;
esac
if [[ -e "$out" || -L "$5" || ! -f "$prefix/system.reg" ]] ||
   [[ "$(realpath -e "$prefix/dosdevices/c:")" != "$prefix/drive_c" ]]; then
    echo 'Use a Wine prefix with its own C: drive and a fresh destination.' >&2
    exit 2
fi
printf '%s  %s\n' 0523b2615596cbb66e7e9ed95a5024008a25f5141ba2e4483e862a47db0c9980 \
    "$archive" | sha256sum -c -
for overlay in "$wmi" "$ado"; do
    (cd "$overlay" && sha256sum -c SHA256SUMS)
done
for arch in x86_64 i386; do
    [[ -f "$wmi/$arch-windows/wbemdisp.dll" && -f "$ado/$arch-windows/msado15.dll" ]]
done

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
    for dll in wbemdisp msado15; do
        overlay="$wmi"
        if [[ "$dll" == msado15 ]]; then overlay="$ado"; fi
        source_dll="$overlay/$arch-windows/$dll.dll"
        install_dll "$source_dll" "$runtime/lib/wine/$arch-windows/$dll.dll"
        install_dll "$source_dll" "$candidate_prefix/drive_c/windows/$system/$dll.dll"
    done
done
printf 'L4 directory: %s\n' "$out"
printf 'Prefix: %s\nRuntime: %s\n' "$candidate_prefix" "$runtime"
