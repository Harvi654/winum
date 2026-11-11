#!/usr/bin/env bash
# Collect L3 records on the maintainer's Arch/KDE Wayland desktop.
# Usage: bash scripts/run-l3-local.sh RUN_ID [SUMMARY_FILE]
set -euo pipefail
unset GH_TOKEN
run=${1:?usage: run-l3-local.sh RUN_ID [SUMMARY_FILE]}
[[ $run =~ ^[0-9]+$ ]] || { echo 'Invalid run ID' >&2; exit 1; }
summary=${2:-l3-summary-$run.txt}
[ ! -e "$summary" ] || { echo 'Summary already exists; choose a new file' >&2; exit 1; }
for tool in gh python3 tar sha256sum timeout wine wineserver; do
    command -v "$tool" >/dev/null || { echo "Missing $tool" >&2; exit 1; }
done
[ "${XDG_SESSION_TYPE:-}" = wayland ] && [ -n "${DISPLAY:-}" ] || {
    echo 'Run in the normal Wayland desktop with Xwayland DISPLAY available' >&2; exit 1;
}
system_wine=$(command -v wine)
system_server=$(command -v wineserver)
unset LD_PRELOAD WINUM_XI_FAULT WINE_X11_SHAPE_ALPHA WINE_OPENGL_REQUIRE_ALPHA
unset WINEPREFIX WINELOADER WINESERVER WINEARCH WINE_VER_OVERRIDE WINE_SHOWHIDE_MOUSEMOVE_MS
export WINEDLLOVERRIDES='mscoree,mshtml='
export WINEDEBUG=-all
work=$(mktemp -d "${TMPDIR:-/tmp}/winum-l3.XXXXXXXX")
server= prefix=
stop_server() {
    if [ -n "$server" ] && [ -n "$prefix" ]; then
        WINEPREFIX="$prefix" timeout 15s "$server" -k >/dev/null 2>&1 || true
        WINEPREFIX="$prefix" timeout 15s "$server" -w >/dev/null 2>&1 || true
    fi
}
cleanup() { stop_server; rm -rf -- "$work"; }
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
repo=Harvi654/winum
gh run view "$run" -R "$repo" --json conclusion,status,headSha > "$work/run.json"
commit=$(python3 - "$work/run.json" <<'PY'
import json, sys
r=json.load(open(sys.argv[1]))
if r['status']!='completed' or r['conclusion']!='success':
    sys.exit('L3 build run must have completed successfully')
print(r['headSha'])
PY
)
gh run download "$run" -R "$repo" -p 'l3-*' -D "$work/artifacts"
for v in full without-putimage without-shape without-xinput fixtures; do
    dir="$work/artifacts/l3-$v"
    (cd "$dir" && sha256sum --quiet -c SHA256SUMS)
    [ "$(cat "$dir/COMMIT")" = "$commit" ] || { echo 'Mixed artifact commits' >&2; exit 1; }
    if [ "$v" != fixtures ]; then
        [ "$(cat "$dir/VARIANT")" = "$v" ] || exit 1
        mkdir "$work/$v"
        archives=("$dir"/winum-*-x86_64.tar.xz)
        [ "${#archives[@]}" = 1 ] && [ -f "${archives[0]}" ] || exit 1
        tar -xJf "${archives[0]}" -C "$work/$v"
    fi
done
python3 - "$work/artifacts" <<'PYPAIR'
import pathlib, sys
root=pathlib.Path(sys.argv[1])
full=(root/'l3-full/series.txt').read_text().splitlines(keepends=True)
assert (root/'l3-full/OMITTED').read_text().strip() == 'none'
for label, patch in [('putimage', 'winex11-flush-putimage.patch'),
                     ('shape', 'winex11-shape-alpha-xwayland.patch'),
                     ('xinput', 'winex11-xinput-bad-device.patch')]:
    d=root/('l3-without-'+label)
    assert (d/'OMITTED').read_text().strip() == patch
    assert sum(l.split('#',1)[0].strip()==patch for l in full)==1
    expected=''.join(l for l in full if l.split('#',1)[0].strip()!=patch)
    assert (d/'series.txt').read_text()==expected
PYPAIR
fixtures="$work/artifacts/l3-fixtures"
# Artifact permissions do not survive upload/download; helpers are loaded as data.
chmod +x "$fixtures"/*.exe
: > "$summary"
summary=$(realpath "$summary")
record() { printf '%s\n' "$*" | tee -a "$summary"; }
record "L3 run=$run commit=$commit session=$XDG_SESSION_TYPE"
select_runtime() {
    stop_server
    label=$1
    if [ "$label" = system ]; then
        winebin=$system_wine; server=$system_server
    else
        bins=("$work/$label"/winum-*/bin/wine)
        winebin=${bins[0]}; server=${winebin%/wine}/wineserver
    fi
    prefix="$work/prefix-$label-$2"
    export WINEPREFIX="$prefix"
    "$winebin" --version > "$work/version" 2>&1
    record "RUNTIME variant=$label version=$(cat "$work/version")"
    WINEDEBUG=-all timeout 120s "$winebin" reg add 'HKCU\Software\Wine\Drivers' /v Graphics /t REG_SZ /d x11 /f > "$work/setup" 2>&1 || {
        record "ERROR prefix_setup variant=$label"; exit 1;
    }
    stop_server
}
collect() {
    name=$1; shift
    record "RUN variant=$label scenario=$name"
    rc=0
    timeout --kill-after=5s 60s "$winebin" "$@" > "$work/raw" 2>&1 || rc=$?
    grep -E '^(REPRO|DRAW|CONTROL|SAMPLE|RECOVERY|SUMMARY|INCONCLUSIVE|XPUT|XFLUSH|OBSERVER_ERROR|XI_HOOK|XI_CALL|FAULT|RAW|CLICK|ALIVE|ERROR|MEASURE|L3 |EXPORT|CALL|SERVER|DISPATCH|CLEANUP)|shaped window under Xwayland|using an ARGB visual|XI_BadDevice|identity.*mapping|no button mapping|NDR64 syntax is not supported' "$work/raw" | tee -a "$summary" || true
    record "EXIT variant=$label scenario=$name code=$rc"
    if [ "$rc" = 124 ] || [ "$rc" = 137 ]; then record 'INCONCLUSIVE external timeout'; fi
    # No helper may leak into prefix shutdown or the next scenario.
    unset LD_PRELOAD WINUM_XI_FAULT
    stop_server
}
for variant in without-putimage full; do
    select_runtime "$variant" putimage
    for render in N Y; do
        WINEDEBUG=-all "$winebin" reg add 'HKCU\Software\Wine\AppDefaults\putimage.exe\X11 Driver' /v ClientSideWithRender /t REG_SZ /d "$render" /f > "$work/setup" 2>&1
        stop_server
        export WINEDEBUG=-all
        collect "putimage-$render-plain" "$fixtures/putimage.exe"
        collect "putimage-$render-control" "$fixtures/putimage.exe" --gdi-flush
        export LD_PRELOAD="$fixtures/observe.so"
        collect "putimage-$render-observed" "$fixtures/putimage.exe"
    done
done
for variant in without-shape full; do
    select_runtime "$variant" shape
    WINEDEBUG=-all "$winebin" reg add 'HKCU\Software\Wine\AppDefaults\shape.exe\X11 Driver' /v ClientSideGraphics /t REG_SZ /d Y /f > "$work/setup" 2>&1
    stop_server
    export WINEDEBUG=-all,+x11drv
    for mode in default ordinary visible disabled visible-optin; do
        unset WINE_X11_SHAPE_ALPHA
        args=()
        case "$mode" in
            ordinary) args=(--ordinary) ;;
            visible) args=(--late-region) ;;
            disabled) export WINE_X11_SHAPE_ALPHA=0 ;;
            visible-optin) export WINE_X11_SHAPE_ALPHA=2; args=(--late-region) ;;
        esac
        echo "Shape $variant/$mode: keep windows uncovered; click red and missing quadrants in each phase."
        collect "shape-$mode" "$fixtures/shape.exe" "${args[@]}"
        read -r -p 'Visual result (transparent/opaque, click targets; enter inconclusive if uncertain): ' observation
        record "OBSERVATION variant=$variant scenario=shape-$mode $observation"
    done
    unset WINE_X11_SHAPE_ALPHA
done
for variant in without-xinput full; do
    select_runtime "$variant" xinput
    export WINEDEBUG=-all,+x11drv,+event
    for mode in observe open map; do
        echo "XInput $variant/$mode: move inside and physically click three times."
        export WINUM_XI_FAULT="$mode"
        export LD_PRELOAD="$fixtures/fault-xinput.so"
        collect "xinput-$mode" "$fixtures/raw-input.exe"
    done
done
for variant in system full; do
    select_runtime "$variant" rpc
    export WINEDEBUG=-all,+rpc
    for arch in x86_64 i686; do
        for mode in both ndr64-only fallback; do
            collect "rpc-$arch-$mode" "$fixtures/measure-bundle/ndrstub-l3-$mode-$arch.exe"
        done
    done
done
record 'COLLECTION complete=1 verdict=manual (completion is not a runtime pass)'
echo "Summary: $summary"
