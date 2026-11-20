#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
set -euo pipefail
umask 077

work="$(realpath -e "${ACAD_AUTOCOMPLETE_WORK:?Set ACAD_AUTOCOMPLETE_WORK to the existing test directory}")"
source_prefix="$work/base-prefix"
test_work="$work/dotnet-10.0.15"
version=10.0.15
core_zip="dotnet-runtime-$version-win-x64.zip"
desktop_zip="windowsdesktop-runtime-$version-win-x64.zip"
dotnet_path='drive_c/Program Files/dotnet'
frameworks=(host/fxr shared/Microsoft.NETCore.App shared/Microsoft.WindowsDesktop.App)

if [[ -e "$test_work" || -L "$test_work" ]]; then
    echo 'The dotnet-10.0.15 comparison directory already exists.' >&2
    exit 2
fi
test "$(realpath -e "$source_prefix")" = "$source_prefix"
test -f "$source_prefix/drive_c/Program Files/Autodesk/AutoCAD 2027/acad.exe"
test -s "$work/base-runtime.txt"
test -x "$(cat "$work/base-runtime.txt")/bin/wine"
for framework in "${frameworks[@]}"; do
    target="$source_prefix/$dotnet_path/$framework"
    test "$(realpath -e "$target/10.0.0")" = "$target/10.0.0"
    if [[ -e "$target/$version" || -L "$target/$version" ]]; then
        echo 'The baseline already contains a .NET 10.0.15 framework.' >&2
        exit 2
    fi
done

prefix_bytes="$(du -s -B1 -- "$source_prefix" | cut -f1)"
free_bytes="$(df -B1 --output=avail "$work" | awk 'NR == 2 { print $1 }')"
if (( free_bytes < prefix_bytes + 512 * 1024 * 1024 )); then
    echo 'Insufficient space for a full prefix copy and the staged archives.' >&2
    exit 2
fi

stage="$(mktemp -d "$work/.dotnet-archive.XXXXXXXX")"
trap 'rm -rf -- "$stage"' EXIT
download_base=https://builds.dotnet.microsoft.com/dotnet
curl -fLsS --retry 2 --max-time 300 "$download_base/Runtime/$version/$core_zip?winum=$version" -o "$stage/$core_zip"
curl -fLsS --retry 2 --max-time 300 "$download_base/WindowsDesktop/$version/$desktop_zip?winum=$version" -o "$stage/$desktop_zip"
(
    cd "$stage"
    sha256sum -c <<'SHA256'
c9f219ad2ec570478f4b1950ae56f88a48e76cabba75e7f54764a1d8a9009e87  dotnet-runtime-10.0.15-win-x64.zip
60f6a65f98a0882deeba9ab78ac923745b420cb9c0a84b9fe47cf3ac5f61c7ed  windowsdesktop-runtime-10.0.15-win-x64.zip
SHA256
    unzip -qo "$core_zip" -d runtime
    unzip -qo "$desktop_zip" -d runtime
)
test -s "$stage/runtime/dotnet.exe"
test -s "$stage/runtime/host/fxr/$version/hostfxr.dll"
test -s "$stage/runtime/shared/Microsoft.NETCore.App/$version/coreclr.dll"
test -s "$stage/runtime/shared/Microsoft.WindowsDesktop.App/$version/wpfgfx_cor3.dll"

mkdir -- "$test_work"
cp -aT --reflink=auto -- "$source_prefix" "$test_work/base-prefix"
cp -- "$work/base-runtime.txt" "$test_work/base-runtime.txt"
test "$(realpath -e "$test_work/base-prefix/dosdevices/c:")" = "$test_work/base-prefix/drive_c"
target="$test_work/base-prefix/$dotnet_path"
for framework in "${frameworks[@]}"; do
    test "$(realpath -e "$target/$framework")" = "$target/$framework"
done
test ! -L "$target/dotnet.exe"
for framework in "${frameworks[@]}"; do
    mv -T -- "$stage/runtime/$framework/$version" "$target/$framework/$version"
done
install -m 755 -- "$stage/runtime/dotnet.exe" "$target/dotnet.exe"
printf 'dotnet_test_ready=%s\n' "$version"
