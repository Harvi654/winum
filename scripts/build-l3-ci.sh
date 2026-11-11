#!/usr/bin/env bash
# Build the production series, omitting exactly one patch in a private worktree.
set -euo pipefail
here=$(cd "$(dirname "$0")/.." && pwd)
variant=${1:?usage: build-l3-ci.sh VARIANT OUT [--prepare-only]}
out=$(mkdir -p "${2:?output directory required}" && realpath "$2")
case "$variant" in
    full) omit= ;;
    without-putimage) omit=winex11-flush-putimage.patch ;;
    without-shape) omit=winex11-shape-alpha-xwayland.patch ;;
    without-xinput) omit=winex11-xinput-bad-device.patch ;;
    *) echo 'Unknown L3 variant' >&2; exit 1 ;;
esac
commit=$(git -C "$here" rev-parse HEAD)
scratch=$(mktemp -d)
cleanup() { git -C "$here" worktree remove --force "$scratch/recipe" >/dev/null 2>&1 || true; rm -rf "$scratch"; }
trap cleanup EXIT
git -C "$here" worktree add --detach "$scratch/recipe" "$commit"
python3 - "$scratch/recipe/patches/series" "$omit" <<'PY'
import pathlib, sys
p = pathlib.Path(sys.argv[1]); omit = sys.argv[2]
lines = p.read_text().splitlines(keepends=True)
if omit:
    assert sum(l.split('#', 1)[0].strip() == omit for l in lines) == 1
    p.write_text(''.join(l for l in lines if l.split('#', 1)[0].strip() != omit))
PY
cp "$scratch/recipe/patches/series" "$out/series.txt"
printf '%s\n' "$commit" > "$out/COMMIT"
printf '%s\n' "$variant" > "$out/VARIANT"
printf '%s\n' "${omit:-none}" > "$out/OMITTED"
if [ "${3:-}" = --prepare-only ]; then exit 0; fi
export WORK="$scratch/work"
bash "$scratch/recipe/scripts/build-wine.sh" "$out"
# Corresponding patched source, including notices; never a release/tag.
source "$here/wine/version.env"
tar --exclude='./build' -C "$WORK/wine-$WINE_VERSION" -cJf "$out/source.tar.xz" .
(cd "$out" && sha256sum COMMIT VARIANT OMITTED series.txt *.tar.xz > SHA256SUMS)
