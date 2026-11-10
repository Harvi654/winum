#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
# The caller supplies its build source, cache and repository pins. No Wine
# is invoked while downloading. Installation runs only in its test prefix.
wine_fetch_gecko()
{
    local source_dir="$1" cache_dir="$2" pins="$3" arch expected file
    # shellcheck disable=SC1090
    source "$pins"
    python3 - "$source_dir/dlls/appwiz.cpl/addons.c" "$WINE_GECKO_VERSION" \
        "$WINE_GECKO_SHA256_X86" "$WINE_GECKO_SHA256_X86_64" <<'PY' || return 1
from pathlib import Path
import re, sys
source = Path(sys.argv[1]).read_text()
match = re.search(r'#define GECKO_VERSION "([^"]+)"\s+'
                  r'#ifdef __i386__\s+#define GECKO_ARCH "x86"\s+'
                  r'#define GECKO_SHA "([0-9a-f]{64})"\s+'
                  r'#elif defined\(__x86_64__\)\s+#define GECKO_ARCH "x86_64"\s+'
                  r'#define GECKO_SHA "([0-9a-f]{64})"', source)
if not match or match.groups() != tuple(sys.argv[2:]):
    sys.exit('Wine Gecko pins do not match the selected Wine source; update and verify them first')
PY
    mkdir -p "$cache_dir" || return 1
    for arch in x86 x86_64; do
        if [ "$arch" = x86 ]; then expected="$WINE_GECKO_SHA256_X86"
        else expected="$WINE_GECKO_SHA256_X86_64"; fi
        file="$cache_dir/wine-gecko-$WINE_GECKO_VERSION-$arch.msi"
        if [ ! -s "$file" ]; then
            curl -fL --retry 3 --connect-timeout 30 --max-time 300 \
                -o "$file.part" "https://dl.winehq.org/wine/wine-gecko/$WINE_GECKO_VERSION/$(basename "$file")" || return 1
            printf '%s  %s\n' "$expected" "$file.part" | sha256sum -c - || return 1
            mv -- "$file.part" "$file" || return 1
        fi
        printf '%s  %s\n' "$expected" "$file" | sha256sum -c - || return 1
    done
}

# Call from the built Wine directory after wineboot under Xvfb. The caller
# keeps mshtml disabled here; only the probe and mshtml tests enable it.
wine_install_gecko()
{
    local cache_dir="$1" output_dir="$2" arch expected file windows_file
    for arch in x86 x86_64; do
        if [ "$arch" = x86 ]; then expected="$WINE_GECKO_SHA256_X86"
        else expected="$WINE_GECKO_SHA256_X86_64"; fi
        file="$cache_dir/wine-gecko-$WINE_GECKO_VERSION-$arch.msi"
        printf '%s  %s\n' "$expected" "$file" | sha256sum -c - || return 1
        windows_file="$(timeout -k 10 30 ./wine winepath -w "$file")" || return 1
        if ! timeout -k 10 120 ./wine msiexec /i "$windows_file" /qn /norestart \
                > "$output_dir/gecko-install-$arch.log" 2>&1; then
            echo "MSHTML_SETUP incomplete: Gecko installation failed or timed out ($arch)" >&2
            return 1
        fi
        printf 'MSHTML_SETUP gecko=%s arch=%s installed=1 sha256_verified=1\n' "$WINE_GECKO_VERSION" "$arch"
    done
}
