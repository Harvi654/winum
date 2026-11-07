#!/usr/bin/env bash
# Original, AI-assisted (Codex)
# SPDX-License-Identifier: LGPL-2.1-or-later
# Only the synthetic fixture belongs in this prefix. Never source common.sh.
set -euo pipefail
umask 077
die() { echo "ERROR $*" >&2; exit 2; }
[[ $# == 2 ]] || die 'Usage: setup-repro-d3d12-prefix.sh INVENTOR_CHECKOUT NEW_PREFIX_UNDER_HOME_CACHE'
[[ ! -e "$2" && ! -L "$2" ]] || die 'Prefix must not exist (including a dangling symlink)'
inventor="$(realpath -e -- "$1")"
prefix="$(realpath -m -- "$2")"
cache_root="$(realpath -m -- "$HOME/.cache")"
case "$prefix" in "$cache_root"/*) ;; *) die 'Prefix must be below ~/.cache';; esac
[[ ! -e "$prefix" && ! -L "$prefix" ]] || die 'Prefix must not exist (including an empty directory)'
parent="$(dirname -- "$prefix")"
while [[ "$parent" != / ]]; do
    [[ ! -f "$parent/system.reg" && ! -d "$parent/drive_c" ]] || die 'Prefix cannot be nested in another Wine prefix'
    parent="$(dirname -- "$parent")"
done
: "${WINE:?Export the exact Wine executable first}"
[[ -x "$WINE" && -x "$(dirname -- "$WINE")/wineserver" ]] || die 'Wine and its sibling wineserver must exist'
for tool in python3 curl sha256sum tar zstd; do command -v "$tool" >/dev/null || die "Missing $tool"; done
# Parse literal pinned defaults only, without executing installer/config code.
pins="$(python3 - "$inventor/scripts/lib/common.sh" <<'PY'
from pathlib import Path
import re, sys
text = Path(sys.argv[1]).read_text()
for key in ('DXVK_VER', 'DXVK_SHA256', 'VKD3D_VER', 'VKD3D_SHA256'):
    match = re.search(r'^' + key + r'="\$\{' + key + r':-([^}"\n]+)\}"$', text, re.M)
    if not match:
        raise SystemExit('Unsupported pin declaration: ' + key)
    value = match[1]
    if not re.fullmatch(r'[0-9a-f]{64}' if key.endswith('SHA256') else r'[0-9]+(?:\.[0-9]+)+', value):
        raise SystemExit('Invalid literal pin: ' + key)
    print(value)
PY
)"
mapfile -t values <<< "$pins"
dxvk_version="${values[0]}"; dxvk_sha="${values[1]}"
vkd3d_version="${values[2]}"; vkd3d_sha="${values[3]}"
[[ "$vkd3d_version" == 3.0.1 && "$dxvk_version" == 2.7.1 ]] || die 'This experiment requires vkd3d-proton 3.0.1 and DXVK 2.7.1; review changed pins separately'
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
downloads="${DOWNLOAD_DIR:-$inventor/downloads}"
fetch_verified() {
    local name="$1" sha="$2" url="$3" source
    source="$downloads/$name"
    if [[ -f "$source" ]]; then
        # Verify the private copy that is extracted, without changing the cache.
        cp -- "$source" "$work/$name"
        echo "ARCHIVE source=cache file=$name"
    else
        curl -fL --retry 3 -o "$work/$name" "$url"
        echo "ARCHIVE source=download file=$name"
    fi
    printf '%s  %s\n' "$sha" "$work/$name" | sha256sum -c -
    echo "PIN file=$name sha256=$sha verified=1"
}
fetch_verified "dxvk-$dxvk_version.tar.gz" "$dxvk_sha" \
    "https://github.com/doitsujin/dxvk/releases/download/v$dxvk_version/dxvk-$dxvk_version.tar.gz"
fetch_verified "vkd3d-proton-$vkd3d_version.tar.zst" "$vkd3d_sha" \
    "https://github.com/HansKristian-Work/vkd3d-proton/releases/download/v$vkd3d_version/vkd3d-proton-$vkd3d_version.tar.zst"
tar -xzf "$work/dxvk-$dxvk_version.tar.gz" -C "$work"
tar --zstd -xf "$work/vkd3d-proton-$vkd3d_version.tar.zst" -C "$work"
# Check all required inputs before creating a prefix.
for dll in dxgi d3d8 d3d9 d3d10core d3d11; do
    for arch in x64 x32; do [[ -s "$work/dxvk-$dxvk_version/$arch/$dll.dll" ]] || die "Missing DXVK $arch/$dll"; done
done
for dll in d3d12 d3d12core; do
    for arch in x64 x86; do [[ -s "$work/vkd3d-proton-$vkd3d_version/$arch/$dll.dll" ]] || die "Missing vkd3d $arch/$dll"; done
done
mkdir -p -- "$(dirname -- "$prefix")"
mkdir -- "$prefix"
export WINEPREFIX="$prefix"
export WINEARCH=win64
# Avoid optional component prompts; do not inherit a caller's graphics overrides.
export WINEDLLOVERRIDES='mscoree,mshtml='
"$WINE" wineboot -i
"$(dirname -- "$WINE")/wineserver" -w
[[ -d "$prefix/drive_c/windows/system32" ]] || die 'wineboot did not create system32'
for target in system32 syswow64; do
    if [[ -d "$prefix/drive_c/windows/$target" ]]; then
        resolved="$(realpath -e -- "$prefix/drive_c/windows/$target")"
        case "$resolved" in "$prefix"/*) ;; *) die 'Windows DLL directory escapes the new prefix';; esac
    fi
done
for dll in dxgi d3d8 d3d9 d3d10core d3d11; do
    cp --remove-destination -- "$work/dxvk-$dxvk_version/x64/$dll.dll" "$prefix/drive_c/windows/system32/"
    if [[ -d "$prefix/drive_c/windows/syswow64" ]]; then
        cp --remove-destination -- "$work/dxvk-$dxvk_version/x32/$dll.dll" "$prefix/drive_c/windows/syswow64/"
    fi
done
for dll in d3d12 d3d12core; do
    cp --remove-destination -- "$work/vkd3d-proton-$vkd3d_version/x64/$dll.dll" "$prefix/drive_c/windows/system32/"
    if [[ -d "$prefix/drive_c/windows/syswow64" ]]; then
        cp --remove-destination -- "$work/vkd3d-proton-$vkd3d_version/x86/$dll.dll" "$prefix/drive_c/windows/syswow64/"
    fi
done
# Match install_dxvk/install_vkd3d, including the unused d3dcompiler_47 override.
for dll in dxgi d3d8 d3d9 d3d10core d3d11 d3dcompiler_47 d3d12 d3d12core; do
    "$WINE" reg add 'HKCU\Software\Wine\DllOverrides' /v "$dll" /t REG_SZ /d native /f
    echo "OVERRIDE dll=$dll mode=native"
done
"$(dirname -- "$WINE")/wineserver" -w
echo "PREFIX ready=$prefix dxvk=$dxvk_version vkd3d=$vkd3d_version fixture_only=1"
