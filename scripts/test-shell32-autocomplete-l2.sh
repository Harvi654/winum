#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
# Rebuild both shell32 variants with the same toolchain and use one set of
# test executables. Every other module comes from the same winum release.
set -euo pipefail
repo="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
out="$(mkdir -p -- "${1:?OUTDIR required}" && cd -- "$1" && pwd)"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
source "$repo/wine/version.env"
source "$repo/scripts/lib/wine-source.sh"
source "$repo/scripts/lib/wine-patches.sh"
WINE_PINNED_VERSION="$WINE_VERSION"
test "$WINE_VERSION" = 11.22
src="$work/wine-$WINE_VERSION"
tarball="$src.tar.xz"
release=winum-11.22-1-x86_64
runtime="$work/$release"
candidate="$repo/patches/candidates/shell32-autocomplete-responsive/shell32-autocomplete-responsive.patch"

curl -fsSL --retry 3 -o "$tarball" "https://dl.winehq.org/wine/source/11.x/wine-$WINE_VERSION.tar.xz"
wine_verify_tarball "$tarball" "$WINE_VERSION"
tar -xf "$tarball" -C "$work"
curl -fsSL --retry 3 -o "$runtime.tar.xz" \
    "https://github.com/Harvi654/winum/releases/download/winum-11.22-1/$release.tar.xz"
printf '%s  %s\n' 058051281c876d3bb0919c93f9143c20667f5c3ef35365eb97f35d5efeb76299 "$runtime.tar.xz" |
    sha256sum -c
tar -xf "$runtime.tar.xz" -C "$work"

wine_apply_patches "$src" "$repo/patches" "$repo" HEAD
patch -d "$src" -p1 --fuzz=0 < "$repo/patches/tests-first/shell32-tests-autocomplete-thread.patch"
mkdir "$src/build"
cd "$src/build"
../configure --enable-archs=x86_64,i386 --enable-tests \
    --without-x --without-wayland --without-vulkan --without-opengl \
    --without-freetype --without-fontconfig --without-alsa --without-pulse \
    > "$work/configure.log" 2>&1 || { tail -n 50 "$work/configure.log"; exit 1; }

dlldir=dlls/shell32
targets=("$dlldir"/{x86_64,i386}-windows/shell32.dll
         "$dlldir"/tests/{x86_64,i386}-windows/shell32_test.exe)
make -j"$(nproc)" "${targets[@]}" > "$work/build-baseline.log" 2>&1 ||
    { tail -n 60 "$work/build-baseline.log"; exit 1; }
for arch in x86_64 i386; do
    cp "$dlldir/$arch-windows/shell32.dll" "$work/baseline-$arch.dll"
done
patch -d "$src" -p1 --fuzz=0 < "$candidate"
make -j"$(nproc)" "${targets[@]}" > "$work/build-candidate.log" 2>&1 ||
    { tail -n 60 "$work/build-candidate.log"; exit 1; }

export WINE="$runtime/lib/wine/x86_64-unix/wine"
export WINESERVER="$runtime/bin/wineserver"
export WINELOADER="$WINE"
export WINEDEBUG=-all
export WINEDLLOVERRIDES='mscoree,mshtml='
export XDG_CACHE_HOME="$work/cache"
source_id="$(git -C "$repo" rev-parse HEAD)"
printf 'source=%s\nruntime=%s\n' "$source_id" "$release" > "$out/BUILDINFO"
sha256sum "$candidate" | awk '{print "candidate_sha256=" $1}' >> "$out/BUILDINFO"

for variant in baseline candidate; do
    (
        export WINEPREFIX="$work/prefix-$variant"
        logs="$work/logs-$variant"
        mkdir "$logs"
        for arch in x86_64 i386; do
            dll="$dlldir/$arch-windows/shell32.dll"
            [[ "$variant" == candidate ]] || dll="$work/baseline-$arch.dll"
            cp --remove-destination "$dll" "$runtime/lib/wine/$arch-windows/shell32.dll"
        done
        Xvfb -displayfd 3 -screen 0 1280x1024x24 -nolisten tcp 3>"$work/display-$variant" &
        xvfb_pid=$!
        stop_variant() {
            timeout -k 3 10 "$WINESERVER" -k >/dev/null 2>&1 || true
            kill "$xvfb_pid" 2>/dev/null || true
        }
        trap stop_variant EXIT
        for _ in $(seq 50); do
            [[ -s "$work/display-$variant" ]] && break
            sleep 0.2
        done
        test -s "$work/display-$variant"
        export DISPLAY=":$(tr -d '\n' < "$work/display-$variant")"
        timeout -k 5 100 "$WINE" wineboot -i > "$work/boot-$variant.log" 2>&1 ||
            { tail -n 15 "$work/boot-$variant.log"; exit 1; }
        timeout -k 4 45 "$WINESERVER" -w
        for arch in x86_64 i386; do
            prefix_dll=system32
            [[ "$arch" == x86_64 ]] || prefix_dll=syswow64
            cp --remove-destination "$runtime/lib/wine/$arch-windows/shell32.dll" \
                "$WINEPREFIX/drive_c/windows/$prefix_dll/shell32.dll"
            cmp "$runtime/lib/wine/$arch-windows/shell32.dll" \
                "$WINEPREFIX/drive_c/windows/$prefix_dll/shell32.dll"
            executable="$dlldir/tests/$arch-windows/shell32_test.exe"
            units="$out/units-$variant-$arch.txt"
            timeout -k 5 60 "$WINE" "$executable" --list > "$work/list-$variant-$arch.log" 2>&1
            sed -nE 's/^    ([[:alnum:]_]+)\r?$/\1/p' "$work/list-$variant-$arch.log" > "$units"
            grep -qx autocomplete "$units"
            grep -qx autocomplete_thread "$units"
            while IFS= read -r unit; do
                echo "$variant $arch $unit"
                log="$logs/shell32.$arch.$unit.log"
                timeout -k 5 120 "$WINE" "$executable" "$unit" > "$log" 2>&1 || printf 'exit=%s\n' "$?" >> "$log"
            done < "$units"
        done
        python3 "$repo/scripts/compare-tests.py" summarize "$logs" > "$out/results-$variant-shell32.json"
    )
done

for arch in x86_64 i386; do
    cmp "$out/units-baseline-$arch.txt" "$out/units-candidate-$arch.txt"
done
rc=0
python3 "$repo/scripts/compare-tests.py" compare "$out/results-baseline-shell32.json" \
    "$out/results-candidate-shell32.json" > "$out/l2-report.md" || rc=$?
python3 - "$out" <<'PY' >> "$out/l2-report.md" || rc=$?
import json
import sys
from pathlib import Path

out = Path(sys.argv[1])
incomplete = []
print("\n## Coverage")
for variant in ("baseline", "candidate"):
    results = json.loads((out / f"results-{variant}-shell32.json").read_text())
    for arch in ("x86_64", "i386"):
        expected = {f"shell32.{arch}.{unit}" for unit in
                    (out / f"units-{variant}-{arch}.txt").read_text().splitlines()}
        finished = {name for name in expected if results.get(name, {}).get("finished")}
        print(f"- {variant} {arch}: {len(finished)}/{len(expected)} units finished")
        incomplete.extend(f"{variant} {name}" for name in sorted(expected - finished))
for entry in incomplete:
    print(f"- Unfinished: {entry}")
sys.exit(bool(incomplete))
PY
cat "$out/l2-report.md"
exit "$rc"
