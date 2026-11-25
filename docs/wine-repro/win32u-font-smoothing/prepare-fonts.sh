#!/usr/bin/env bash
# Generate the free probe fonts beside the Windows executables.
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "$here/../../.." && pwd)"
out="$(realpath -e "${1:?existing bundle directory required}")"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
source "$repo/wine/version.env"
source "$repo/scripts/lib/wine-source.sh"
export WINE_PINNED_VERSION="$WINE_VERSION"
test "$WINE_VERSION" = 11.22
wine_source=${2:-$work/wine.tar.xz}
liberation=${3:-$work/liberation.tar.gz}
if [[ $# -lt 2 ]]; then
    curl -fsSL --retry 3 https://dl.winehq.org/wine/source/11.x/wine-11.22.tar.xz -o "$wine_source"
fi
if [[ $# -lt 3 ]]; then
    curl -fsSL --retry 3 \
        https://github.com/liberationfonts/liberation-fonts/files/7261482/liberation-fonts-ttf-2.1.5.tar.gz \
        -o "$liberation"
fi
wine_verify_tarball "$wine_source" 11.22
printf '%s  %s\n' 7191c669bf38899f73a2094ed00f7b800553364f90e2637010a69c0e268f25d0 "$liberation" |
    sha256sum -c -
tar -xf "$wine_source" -C "$work" wine-11.22/{COPYING.LIB,fonts/tahoma.ttf,fonts/tahoma.sfd}
lib=liberation-fonts-ttf-2.1.5
tar -xf "$liberation" -C "$work" "$lib"/{LICENSE,LiberationSans-Regular.ttf}
cp "$work/wine-11.22/COPYING.LIB" "$out/Wine-COPYING.LIB"
cp "$work/wine-11.22/fonts/tahoma.sfd" "$out/Wine-tahoma.sfd"
cp "$work/$lib/LICENSE" "$out/Liberation-OFL.txt"
python3 - "$work" "$out" "$lib" <<'PY'
from fontTools.ttLib import TTFont
from pathlib import Path
import sys

work, out = map(Path, sys.argv[1:3])
faces = (
    (work / "wine-11.22/fonts/tahoma.ttf", "WinumProbeTahoma", "probe-tahoma.ttf"),
    (work / sys.argv[3] / "LiberationSans-Regular.ttf", "WinumProbeSans", "probe-sans.ttf"),
)
for path, family, name in faces:
    with TTFont(path, recalcTimestamp=False) as font:
        for record in font["name"].names:
            if record.nameID in (1, 3, 4, 6, 16, 18, 21, 25):
                record.string = family
        if name == "probe-tahoma.ttf":
            assert font["gasp"].gaspRange == {8: 0, 16: 1, 65535: 3}
        elif "gasp" in font:
            del font["gasp"]
        font.save(out / name)
PY
cat >> "$out/README.txt" <<'INFO'

probe-tahoma.ttf is renamed from Wine 11.22 fonts/tahoma.ttf, LGPL 2.1
or later. The gasp table and bitmaps are preserved. Wine-tahoma.sfd is
its original source. probe-sans.ttf is renamed from Liberation Sans
Regular 2.1.5, SIL OFL 1.1, with its gasp table omitted as a control.
Rebuild with prepare-fonts.sh in the source checkout.
Font sources: https://dl.winehq.org/wine/source/11.x/wine-11.22.tar.xz
https://github.com/liberationfonts/liberation-fonts/releases/tag/2.1.5
Use a disposable offline Windows snapshot. The probe saves and restores
the original smoothing settings. Return only result-*.txt and exitcodes.txt.
A complete run prints matrices=8 and restored=1 on each architecture.
INFO
(cd "$out" && sha256sum -- ./* 2>/dev/null | awk '$2 != "./SHA256SUMS"' > "$work/SHA256SUMS")
cp "$work/SHA256SUMS" "$out/SHA256SUMS"
(cd "$out" && sha256sum -c SHA256SUMS)
