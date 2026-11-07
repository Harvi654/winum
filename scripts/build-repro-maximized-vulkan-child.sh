#!/usr/bin/env bash
# Original, AI-assisted (Codex)
# SPDX-License-Identifier: LGPL-2.1-or-later
set -euo pipefail
umask 022
[[ $# == 1 ]] || { echo 'Usage: build-repro-maximized-vulkan-child.sh NEW_OUTPUT' >&2; exit 2; }
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
src="$here/docs/wine-repro/win32u-maximized-vulkan-child"
output="$(realpath -m -- "$1")"
[[ ! -e "$output" && ! -L "$output" ]] || { echo 'Use a new output directory' >&2; exit 2; }
for tool in x86_64-w64-mingw32-gcc x86_64-w64-mingw32-windres; do
    command -v "$tool" >/dev/null || { echo "Missing $tool" >&2; exit 2; }
done
# Copy just the portable Vulkan headers, never add host /usr/include to MinGW.
vulkan_include="${VULKAN_HEADERS_INCLUDE:-/usr/include}"
[[ -f "$vulkan_include/vulkan/vulkan.h" ]] || { echo 'Install vulkan-headers (Debian: libvulkan-dev), or export VULKAN_HEADERS_INCLUDE' >&2; exit 2; }
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
mkdir "$work/include"
cp -R "$vulkan_include/vulkan" "$work/include/"
if [[ -d "$vulkan_include/vk_video" ]]; then cp -R "$vulkan_include/vk_video" "$work/include/"; fi
cp "$src/repro.manifest" "$work/"
printf '1 24 "repro.manifest"\n' > "$work/repro.rc"
x86_64-w64-mingw32-windres -I "$work" "$work/repro.rc" -O coff -o "$work/manifest.o"
x86_64-w64-mingw32-gcc -std=c11 -O2 -static -Wall -Wextra -Werror \
    -I "$work/include" "$src/repro.c" "$work/manifest.o" \
    -o "$work/maximized-vulkan-child.exe" -ldxguid -lcomctl32 -luxtheme -lgdi32 -luser32
mkdir -p -- "$(dirname -- "$output")"
mkdir -- "$output"
cp "$work/maximized-vulkan-child.exe" "$src/repro.c" "$src/repro.manifest" "$src/README.md" "$here/LICENSE" "$output/"
(cd "$output" && sha256sum maximized-vulkan-child.exe repro.c repro.manifest README.md LICENSE > SHA256SUMS)
echo "Synthetic reproducer bundle: $output"
