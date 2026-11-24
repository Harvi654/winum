#!/usr/bin/env bash
# Use a different fresh Wine prefix for each fontconfig condition.
set -euo pipefail
wine="$(realpath -e "${1:?Wine loader required}")"
export WINESERVER="$(realpath -e "${2:?wineserver required}")"
bundle="$(realpath -e "${3:?bundle directory required}")"
out="$(realpath -m "${4:?result directory required}")"
mkdir "$out"
(cd "$bundle" && sha256sum -c SHA256SUMS)
export WINEDEBUG=-all WINEDLLOVERRIDES='mscoree,mshtml='
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
cd "$bundle"
for rgba in none rgb; do
    (
        export WINEPREFIX="$work/prefix-$rgba" FONTCONFIG_FILE="$work/fonts-$rgba.conf"
        trap 'timeout -k 5s 20s "$WINESERVER" -k 2>/dev/null || true' EXIT
        cat > "$FONTCONFIG_FILE" <<CONF
<?xml version="1.0"?>
<!DOCTYPE fontconfig SYSTEM "urn:fontconfig:fonts.dtd">
<fontconfig>
 <include ignore_missing="no">/etc/fonts/fonts.conf</include>
 <match target="pattern"><edit name="rgba" mode="assign"><const>$rgba</const></edit></match>
 <match target="font">
   <edit name="antialias" mode="assign"><bool>true</bool></edit>
   <edit name="rgba" mode="assign"><const>$rgba</const></edit>
 </match>
</fontconfig>
CONF
        expected=5
        [[ $rgba != rgb ]] || expected=1
        actual="$(fc-match -f '%{rgba}' sans-serif)"
        test "$actual" = "$expected"
        timeout -k 5s 120s "$wine" wineboot -i > "$out/boot-$rgba.log" 2>&1 ||
            { cat "$out/boot-$rgba.log"; exit 1; }
        timeout -k 5s 60s "$WINESERVER" -w
        for arch in x86_64 i686; do
            result="$out/$rgba-$arch.txt"
            printf 'fontconfig_rgba=%s fc_match_rgba=%s\n' "$rgba" "$actual" > "$result"
            timeout -k 5s 150s "$wine" "$bundle/font-smoothing-$arch.exe" 2>&1 |
                tr -d '\r' >> "$result"
            grep -Fxq 'measure_complete matrices=8 restored=1' "$result"
            [[ $(grep -c '^case=' "$result") == 2880 ]]
            [[ $(grep -c '^matrix_cases=360$' "$result") == 8 ]]
        done
    )
done
