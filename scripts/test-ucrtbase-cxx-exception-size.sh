#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
set -euo pipefail
repo="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
bundle="$(realpath -e -- "${1:?BUILD_BUNDLE required}")"
out="$(mkdir -p -- "${2:?OUTDIR required}" && cd -- "$2" && pwd)"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT

(cd "$bundle" && sha256sum -c SHA256SUMS)
release=winum-11.22-1-x86_64
runtime="$work/$release"
curl -fLsS --retry 3 -o "$runtime.tar.xz" \
    "https://github.com/Harvi654/winum/releases/download/winum-11.22-1/$release.tar.xz"
printf '%s  %s\n' 058051281c876d3bb0919c93f9143c20667f5c3ef35365eb97f35d5efeb76299 "$runtime.tar.xz" | sha256sum -c -
tar -xJf "$runtime.tar.xz" -C "$work"
export WINE="$runtime/lib/wine/x86_64-unix/wine"
export WINESERVER="$runtime/bin/wineserver" WINELOADER="$WINE"
export WINEDEBUG=-all WINEDLLOVERRIDES='mscoree,mshtml=;ucrtbase=b'

for variant in baseline candidate; do
    (
        export WINEPREFIX="$work/prefix-$variant" XDG_CACHE_HOME="$work/cache-$variant"
        logs="$out/$variant"
        mkdir -- "$logs"
        cp --remove-destination -- "$bundle/$variant/ucrtbase.dll" "$runtime/lib/wine/x86_64-windows/ucrtbase.dll"
        cp --remove-destination -- "$bundle/$variant/i386/ucrtbase.dll" "$runtime/lib/wine/i386-windows/ucrtbase.dll"
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
        export DISPLAY=":$(< "$work/display-$variant")"
        timeout -k 5 100 "$WINE" wineboot -i > "$logs/boot.txt" 2>&1
        timeout -k 4 45 "$WINESERVER" -w

        for arch in x86_64 i386; do
            case "$arch" in x86_64) system=system32 ;; i386) system=syswow64 ;; esac
            cp --remove-destination -- "$runtime/lib/wine/$arch-windows/ucrtbase.dll" \
                "$WINEPREFIX/drive_c/windows/$system/ucrtbase.dll"
            executable="$bundle/tests/ucrtbase-$arch.exe"
            timeout -k 5 60 "$WINE" "$executable" --list > "$logs/list-$arch.txt" 2>&1
            units="$logs/units-$arch.txt"
            sed -nE 's/^    ([[:alnum:]_]+)\r?$/\1/p' "$logs/list-$arch.txt" > "$units"
            grep -qx cpp "$units"
            while IFS= read -r unit; do
                printf '%s %s %s\n' "$variant" "$arch" "$unit"
                log="$logs/ucrtbase.$arch.$unit.log"
                testdir="$work/$variant-$arch-$unit"
                mkdir -- "$testdir"
                (cd "$testdir" && timeout -k 5 120 "$WINE" "$executable" "$unit") > "$log" 2>&1 ||
                    printf 'exit=%s\n' "$?" >> "$log"
            done < "$units"
        done
        python3 "$repo/scripts/compare-tests.py" summarize "$logs" > "$out/$variant.json"
    )
done

for arch in x86_64 i386; do
    cmp -- "$out/baseline/units-$arch.txt" "$out/candidate/units-$arch.txt"
done
grep -F '__CxxQueryExceptionSize returned 16, expected 32' "$out/baseline/ucrtbase.x86_64.cpp.log"
if grep -F '__CxxQueryExceptionSize returned' "$out/candidate/ucrtbase.x86_64.cpp.log"; then
    exit 1
fi
rc=0
python3 "$repo/scripts/compare-tests.py" compare "$out/baseline.json" "$out/candidate.json" > "$out/l2-report.md" || rc=1
python3 - "$out" <<'PY' >> "$out/l2-report.md" || rc=1
import json
import sys
from pathlib import Path

out = Path(sys.argv[1])
incomplete = []
print("\n## Coverage")
for variant in ("baseline", "candidate"):
    results = json.loads((out / f"{variant}.json").read_text())
    for arch in ("x86_64", "i386"):
        expected = {f"ucrtbase.{arch}.{unit}" for unit in
                    (out / variant / f"units-{arch}.txt").read_text().splitlines()}
        finished = {name for name in expected if results.get(name, {}).get("finished")}
        print(f"- {variant} {arch}: {len(finished)}/{len(expected)} units finished")
        incomplete.extend(f"{variant} {name}" for name in sorted(expected - finished))
for entry in incomplete:
    print(f"- Unfinished: {entry}")
sys.exit(bool(incomplete))
PY
cat "$out/l2-report.md"
exit "$rc"
