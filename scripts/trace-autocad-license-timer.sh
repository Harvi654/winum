#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
set -euo pipefail
umask 077
[[ $# == 1 ]] || { echo 'Usage: bash trace-autocad-license-timer.sh TEST_DIRECTORY' >&2; exit 2; }
summary="$(dirname -- "$(realpath -e -- "${BASH_SOURCE[0]}")")/summarize-autocad-license-timer.py"
test -f "$summary"
work="$(realpath -e -- "$1")"
export ACAD_PREFIX="$work/candidate-prefix" ACAD_WINE_BIN="$work/wine/bin"
export WINEPREFIX="$ACAD_PREFIX" WINESERVER="$ACAD_WINE_BIN/wineserver"
wine="$ACAD_WINE_BIN/wine"
test "$(realpath -e -- "$ACAD_PREFIX")" = "$ACAD_PREFIX"
test -f "$ACAD_PREFIX/system.reg"
test -x "$wine" && test -x "$WINESERVER"
launcher="${ACAD_REPO:-$HOME/autocad-on-linux}/scripts/launch-autocad.sh"
test -f "$launcher"
export ACAD_LOGDIR="$(mktemp -d "$work/license-timer.XXXXXXXX")"
printf 'Output directory: %s\n' "$ACAD_LOGDIR"
key='HKCU\Software\Wine\Debug'
reg() {
    WINEDEBUG=-all timeout 15s "$wine" reg "$@" </dev/null
}
rc=0
reg query "$key" /v RelayInclude > "$ACAD_LOGDIR/relay-config.txt" 2>&1 || rc=$?
case "$rc" in
    0)
        prior=$(awk '$1 == "RelayInclude" && ($2 == "REG_SZ" || $2 == "REG_EXPAND_SZ") {
            sub(/\r$/, ""); sub(/^[[:space:]]*RelayInclude[[:space:]]+/, ""); print; exit
        }' "$ACAD_LOGDIR/relay-config.txt")
        [[ -n "$prior" ]] || { echo 'Could not back up RelayInclude.' >&2; exit 2; }
        type=${prior%%[[:space:]]*}
        value=$(sed -E 's/^[^[:space:]]+[[:space:]]*//' <<< "$prior")
        restore=(add "$key" /v RelayInclude /t "$type" /d "$value" /f)
        ;;
    1) restore=(delete "$key" /v RelayInclude /f) ;;
    *) echo 'Could not query RelayInclude.' >&2; exit 2 ;;
esac
restore_relay() {
    if reg "${restore[@]}" >> "$ACAD_LOGDIR/relay-config.txt" 2>&1; then
        echo 'timer_relay_filter=restored'
    else
        echo 'timer_relay_filter=restore_failed' >&2
    fi
}
trap restore_relay EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
reg add "$key" /v RelayInclude /t REG_SZ /d 'user32.SetTimer;user32.KillTimer' /f >> "$ACAD_LOGDIR/relay-config.txt" 2>&1
export WINEDEBUG='-all,+timestamp,+pid,acad.exe:+relay,acad.exe:+msg,acad.exe:+taskdialog'
rc=0
bash "$launcher" || rc=$?
printf 'launcher_exit=%s\n' "$rc"
python3 -I "$summary" "$ACAD_LOGDIR" | tee "$ACAD_LOGDIR/timer-summary.txt"
exit "$rc"
