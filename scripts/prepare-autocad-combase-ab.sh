#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
set -euo pipefail
umask 077

if [[ $# != 2 ]]; then
    echo 'Usage: prepare-autocad-combase-ab.sh PREVIOUS_DWG_TEST_DIR EMPTY_AB_DIR' >&2
    exit 2
fi

previous="$(realpath -e -- "$1")"
work="$(realpath -e -- "$2")"
prefix="$previous/base-prefix"
fixture="$previous/dwg-controls/line-2018.dwg"
name=winum-11.22-1-x86_64

test -d "$work"
if [[ -n "$(find "$work" -mindepth 1 -maxdepth 1 -print -quit)" ]]; then
    echo 'Use an empty directory for the A/B test.' >&2
    exit 2
fi
test -f "$prefix/drive_c/Program Files/Autodesk/AutoCAD 2027/acad.exe"
test "$(realpath -e -- "$prefix/dosdevices/c:")" = "$prefix/drive_c"
test "$(sha256sum < "$fixture" | cut -d' ' -f1)" = \
    1f4edd4c11aaf28b3295fd69fa59376f9cb911c637aa47a6073408ecac4797a3

download_build() (
    mkdir -- "$work/$1"
    cd "$work/$1"
    gh run download "$2" --repo Harvi654/winum -n winum-11.22-1-binary
    sha256sum -c "$name.tar.xz.sha256"
    tar -xJf "$name.tar.xz"
    grep -Eq "^commit:[[:space:]]+$3[[:xdigit:]]*$" "$name/BUILDINFO"
)

download_build reference 44528762825 aeb7a59
download_build comparison 44739990758 53ac693
reference="$work/reference/$name"
comparison="$work/comparison/$name"

patch_list() {
    awk '/^[[:xdigit:]]+  [^ ]+\.patch$/ {print}' "$1/BUILDINFO"
}

if ! grep -Eq '  combase-stub-activation-context\.patch$' "$reference/BUILDINFO" ||
    ! cmp -s <(patch_list "$reference" | sed '/  combase-stub-activation-context\.patch$/d') \
        <(patch_list "$comparison"); then
    echo 'Build manifests do not match the single-patch comparison.' >&2
    exit 1
fi

cp -aT --reflink=auto -- "$prefix" "$work/base-prefix"
cp -aT --reflink=auto -- "$prefix" "$work/candidate-prefix"
cp -aT --reflink=auto -- "$reference" "$work/wine"
cp -- "$fixture" "$work/line-2018.dwg"

# Use the reference Wine distribution in both trials. Only combase comes
# from the comparison build. Stage both PE copies to avoid prefix fallback.
for arch in x86_64 i386; do
    dll="lib/wine/$arch-windows/combase.dll"
    case "$arch" in
        x86_64) destination=system32 ;;
        i386) destination=syswow64 ;;
    esac
    test -s "$reference/$dll"
    test -s "$comparison/$dll"
    if cmp -s "$reference/$dll" "$comparison/$dll"; then
        echo "The $arch COM DLLs are identical." >&2
        exit 1
    fi
    for trial in base candidate; do
        system_dir="$work/$trial-prefix/drive_c/windows/$destination"
        test "$(realpath -e -- "$system_dir")" = "$system_dir"
        runtime="$reference"
        if [[ "$trial" == candidate ]]; then runtime="$comparison"; fi
        cp --remove-destination -- "$runtime/$dll" "$system_dir/combase.dll"
    done
    cp --remove-destination -- "$comparison/$dll" "$work/wine/$dll"
    printf 'combase_arch=%s reference=%s comparison=%s\n' "$arch" \
        "$(sha256sum < "$reference/$dll" | cut -d' ' -f1)" \
        "$(sha256sum < "$comparison/$dll" | cut -d' ' -f1)"
done

printf '%s\n' "$reference" > "$work/base-runtime.txt"
echo 'combase_ab=ready'
