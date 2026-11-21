#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
# Observe the registered licensing service while the AutoCAD dialog is open.
set -euo pipefail
umask 077

[[ $# == 1 ]] || { echo 'Usage: bash trace-autocad-licensing-service.sh SESSION_TEST_DIRECTORY' >&2; exit 2; }
work="$(realpath -e -- "$1")"
grep -Fxq 'wineserver_variant=services_session_report' "$work/COMBINED-BUILDINFO"
export ACAD_PREFIX="$work/candidate-prefix"
export ACAD_WINE_BIN="$work/wine/bin"
export WINEPREFIX="$ACAD_PREFIX" WINELOADER="$ACAD_WINE_BIN/wine" WINESERVER="$ACAD_WINE_BIN/wineserver"
test "$(realpath -e "$ACAD_PREFIX")" = "$ACAD_PREFIX"
test -f "$ACAD_PREFIX/drive_c/Program Files/Autodesk/AutoCAD 2027/acad.exe"
test -x "$WINELOADER" && test -x "$WINESERVER"
export ACAD_LOGDIR="$(mktemp -d "$work/licensing-service-trace.XXXXXXXX")"
printf 'Output directory: %s\n' "$ACAD_LOGDIR"
cd "${ACAD_REPO:-$HOME/autocad-on-linux}"
launcher="$PWD/scripts/launch-autocad.sh"

if ! bash -c '
    source scripts/common-env.sh || exit 1
    cd "$INVENTOR_DIR" || exit 1
    source scripts/lib/common.sh || exit 1
    test "$WINEPREFIX" = "$ACAD_PREFIX" &&
    test "$(type -P wine)" = "$WINELOADER" &&
    test "$(type -P wineserver)" = "$WINESERVER"
' >"$ACAD_LOGDIR/preflight.txt" 2>&1; then
    printf 'The prefix or Wine runtime check failed. See %s/preflight.txt\n' "$ACAD_LOGDIR" >&2
    exit 1
fi

modified="$(python3 - "$launcher" <<'PY'
from pathlib import Path
import re
import sys

script = Path(sys.argv[1]).read_text()
direct = r'^wine "[^\n]*AdskLicensingService\.exe"[ \t]*>[^\n]*&[ \t]*$'
script, replaced = re.subn(direct, 'start_autocad_scm || exit "$?"', script, flags=re.M)
if replaced != 1:
    sys.exit("Could not identify a unique direct licensing-service launch.")
sys.stdout.write(script)
PY
)"

start_autocad_scm() {
    local service=AdskLicensingService state="$ACAD_LOGDIR/sc-state.txt" start_rc=0 attempt
    WINEDEBUG=-all timeout 8s "$WINELOADER" tasklist /FI 'IMAGENAME eq services.exe' /FO CSV /NH \
        >"$ACAD_LOGDIR/scm-session.txt" 2>&1 || true
    WINEDEBUG=-all timeout 8s "$WINELOADER" sc query "$service" >"$ACAD_LOGDIR/sc-before.txt" 2>&1 || true
    if grep -Eq 'STATE[[:space:]]*:[[:space:]]*4[[:space:]]+RUNNING' "$ACAD_LOGDIR/sc-before.txt"; then
        printf 'sc_start=already_running\n'
        cp -- "$ACAD_LOGDIR/sc-before.txt" "$state"
    else
        WINEDEBUG='+pid,+timestamp,err+all,sc.exe:+service' timeout 35s "$WINELOADER" sc start "$service" \
            >"$ACAD_LOGDIR/sc-start.txt" 2>&1 || start_rc=$?
        printf 'sc_start_unix_exit=%d\n' "$start_rc"
        grep -E 'failed to (start|open) service [[:digit:]]+' "$ACAD_LOGDIR/sc-start.txt" | tail -n 5 || true
        for attempt in {1..10}; do
            WINEDEBUG=-all timeout 5s "$WINELOADER" sc query "$service" >"$state" 2>&1 || true
            if grep -Eq 'STATE[[:space:]]*:[[:space:]]*[14][[:space:]]+(STOPPED|RUNNING)' "$state"; then
                break
            fi
            sleep 1
        done
    fi
    grep -E 'SERVICE_NAME|STATE|WIN32_EXIT_CODE|SERVICE_EXIT_CODE' "$state" || true
    grep -iF 'services.exe' "$ACAD_LOGDIR/scm-session.txt" || true
    if ! grep -Eq 'STATE[[:space:]]*:[[:space:]]*4[[:space:]]+RUNNING' "$state"; then
        printf 'scm_trial=service_did_not_reach_running\n'
        return 1
    fi
    printf 'scm_trial=service_running_starting_autocad\n'
}
export -f start_autocad_scm
export WINEDEBUG='-all,+pid,+timestamp,services.exe:+service,adsklicensingservice.exe:+service,adsklicensingservice.exe:+winsock,adsklicensingservice.exe:+dnsapi,adsklicensingservice.exe:+winhttp,adsklicensingservice.exe:warn+secur32,adsklicensingservice.exe:warn+crypt,adsklicensingservice.exe:err+all'
if grep -Fxq 'dialog_origin_trace=1' "$work/COMBINED-BUILDINFO"; then
    export WINEDEBUG="$WINEDEBUG,acad.exe:+taskdialog"
fi

if [[ ${ACAD_CLIENT_RELAY:-0} == 1 ]]; then
    key='HKCU\Software\Wine\Debug'
    config_log="$ACAD_LOGDIR/client-relay-config.txt"
    query_rc=0
    WINEDEBUG=-all timeout 15s "$WINELOADER" reg query "$key" /v RelayInclude > "$config_log" 2>&1 || query_rc=$?
    case "$query_rc" in
        0)
            prior=$(awk '$1 == "RelayInclude" && ($2 == "REG_SZ" || $2 == "REG_EXPAND_SZ") {
                sub(/\r$/, ""); sub(/^[[:space:]]*RelayInclude[[:space:]]+/, ""); print; exit
            }' "$config_log")
            [[ -n "$prior" ]] || { echo 'Could not back up the prior RelayInclude value.' >&2; exit 2; }
            prior_type=${prior%%[[:space:]]*}
            prior_value=$(sed -E 's/^[^[:space:]]+[[:space:]]*//' <<< "$prior")
            restore=(reg add "$key" /v RelayInclude /t "$prior_type" /d "$prior_value" /f)
            ;;
        1) restore=(reg delete "$key" /v RelayInclude /f) ;;
        *) echo 'Could not query RelayInclude.' >&2; exit 2 ;;
    esac
    restore_client_relay() {
        if WINEDEBUG=-all timeout 15s "$WINELOADER" "${restore[@]}" >> "$config_log" 2>&1; then
            echo 'client_relay_filter=restored'
        else
            echo 'client_relay_filter=restore_failed' >&2
        fi
    }
    trap restore_client_relay EXIT
    trap 'exit 130' INT
    trap 'exit 143' TERM
    apis='ws2_32.connect;ws2_32.WSAConnect;ws2_32.recv;ws2_32.send;ws2_32.WSARecv;ws2_32.WSASend;ws2_32.shutdown;ws2_32.closesocket;ws2_32.WSAGetOverlappedResult'
    if ! WINEDEBUG=-all timeout 15s "$WINELOADER" reg add "$key" /v RelayInclude /t REG_SZ /d "$apis" /f >> "$config_log" 2>&1; then
        echo 'Could not configure the AutoCAD client relay filter.' >&2
        exit 2
    fi
    export WINEDEBUG="$WINEDEBUG,acad.exe:+relay,acad.exe:+winsock"
    echo 'client_relay_filter=enabled'
fi

bash -c "$modified" "$launcher" &
launcher_pid=$!
printf '%s\n' 'When Connection Lost appears, leave the dialog open and press Enter here.' \
    'If the drawing is still open after five minutes, press Enter and record that result.'
shopt -s nullglob
record_log_cutoffs() {
    local log
    for log in "$ACAD_LOGDIR"/*.log "$ACAD_LOGDIR/sc-start.txt"; do
        [[ -f "$log" ]] || continue
        printf '%s %s\n' "$(wc -l < "$log")" "${log##*/}"
    done > "$1"
}
while kill -0 "$launcher_pid" 2>/dev/null; do
    read_rc=0
    read -r -t 1 || read_rc=$?
    if (( read_rc > 128 )); then continue; fi
    if (( read_rc != 0 )); then break; fi
    date -Ins > "$ACAD_LOGDIR/marker-time.txt"
    record_log_cutoffs "$ACAD_LOGDIR/marker-lines.txt"
    WINEDEBUG=-all timeout 8s "$WINELOADER" sc query AdskLicensingService \
        >"$ACAD_LOGDIR/sc-at-marker.txt" 2>&1 || true
    WINEDEBUG=-all timeout 8s "$WINELOADER" tasklist /FI 'IMAGENAME eq AdskLicensingService.exe' /FO CSV /NH \
        >"$ACAD_LOGDIR/sc-pid-at-marker.txt" 2>&1 || true
    printf '%s\n' 'Marker recorded. Keep AutoCAD open until the capture finishes.'
    for _ in {1..45}; do
        kill -0 "$launcher_pid" 2>/dev/null || break
        sleep 1
    done
    record_log_cutoffs "$ACAD_LOGDIR/end-lines.txt"
    if kill -0 "$launcher_pid" 2>/dev/null; then
        WINEDEBUG=-all timeout 8s "$WINELOADER" sc query AdskLicensingService \
            >"$ACAD_LOGDIR/sc-at-end.txt" 2>&1 || true
    fi
    printf '%s\n' 'Capture finished. Close AutoCAD now.'
    break
done
rc=0
wait "$launcher_pid" || rc=$?
printf 'launcher_exit=%s\n' "$rc"

if [[ -s "$ACAD_LOGDIR/marker-time.txt" ]]; then
    python3 - "$ACAD_LOGDIR" "$ACAD_PREFIX" <<'PY' | tee "$ACAD_LOGDIR/service-summary.txt"
from collections import Counter, defaultdict, deque
from datetime import datetime, timezone
from pathlib import Path
import csv
import re
import sys

run = Path(sys.argv[1])
prefix = Path(sys.argv[2])
marker = datetime.fromisoformat((run / "marker-time.txt").read_text().strip())
def cutoffs(name):
    return {log: int(count) for count, log in (line.split(maxsplit=1) for line in
            (run / name).read_text().splitlines())}


limits = cutoffs("marker-lines.txt")
end = cutoffs("end-lines.txt")
header = re.compile(r"^\s*(\d+\.\d+):([0-9a-f]+):[0-9a-f]+:(\w+):(\w+):(\w+)\s*(.*)", re.I)
status = re.compile(r"\b(?:status|error(?: code)?|ret(?:urn(?:ed)?)?)\s*[:=]?\s*(0x[0-9a-f]+|\d+)\b", re.I)
logs = [path for path in (*run.glob("*.log"), run / "sc-start.txt") if path.is_file()]
service_pids = set()
for row in csv.reader((run / "sc-pid-at-marker.txt").read_text(errors="replace").splitlines()):
    if len(row) >= 2 and row[0].casefold() == "adsklicensingservice.exe" and row[1].isdigit():
        service_pids.add(int(row[1]))
if not service_pids:
    for path in logs:
        with path.open(errors="replace") as stream:
            for line in stream:
                hit = header.match(line)
                if hit and hit[5] == "service" and hit[6] in ("StartServiceCtrlDispatcherW", "StartServiceCtrlDispatcherA"):
                    service_pids.add(int(hit[2], 16))

print(f"marker={marker.isoformat()}")
for point in ("marker", "end"):
    state_log = run / f"sc-at-{point}.txt"
    if state_log.is_file():
        print(f"service_state_at_{point}")
        for line in state_log.read_text(errors="replace").splitlines():
            if any(label in line for label in ("STATE", "WIN32_EXIT_CODE", "SERVICE_EXIT_CODE")):
                print(line.strip())
print("service_windows_pid=" + (",".join(f"0x{pid:x}" for pid in sorted(service_pids)) or "unresolved"))

routine_statuses = {0, 0x103, 0x102, 0xc00000a3, 997, 10035}
counts = defaultdict(Counter)
recent = defaultdict(list)
for path in logs:
    cutoff = limits.get(path.name, 0)
    with path.open(errors="replace") as stream:
        for line_no, line in enumerate(stream, 1):
            if line_no > end.get(path.name, 0):
                break
            hit = header.match(line)
            if not hit or int(hit[2], 16) not in service_pids:
                continue
            tick, _, level, channel, api, detail = hit.groups()
            phase = "before_marker" if line_no <= cutoff else "after_marker"
            counts[phase][channel] += 1
            codes = [int(value, 16 if value.lower().startswith("0x") else 10) for value in status.findall(detail)]
            failures = [f"0x{code:x}" for code in codes if code not in routine_statuses]
            event = f"{tick} {level} {channel}:{api}"
            if (channel == "winsock" and api.lower() in ("connect", "wsaconnect", "connectex")
                    and "addr {" in detail and "port " in detail):
                scope = "loopback" if re.search(r"\baddress (?:127\.|::1\b)", detail) else "other"
                recent[phase].append(f"{event} destination={scope}")
            elif failures or level in ("err", "warn"):
                recent[phase].append(event + (f" code={','.join(failures)}" if failures else ""))
for phase in ("before_marker", "after_marker"):
    print(f"\n{phase}")
    print("channels=" + (",".join(f"{name}:{value}" for name, value in sorted(counts[phase].items())) or "none"))
    print("\n".join(sorted(recent[phase], key=lambda row: float(row.split()[0]))[-24:]) or "no selected events")

service_logs = prefix / "drive_c/ProgramData/Autodesk/AdskLicensingService/Log"
clock = re.compile(r"(?<!\d)(\d{2}):(\d{2}):(\d{2})(?:[.,]\d+)?(?!\d)")
tags = re.compile(r"\b(?:timeout|timed out|deadline exceeded|disconnect\w*|refused|unavailable|expired|"
                  r"denied|unauthori[sz]ed|offline|invalid|heartbeat|error|fail(?:ed|ure)?)\b", re.I)
for path in sorted(service_logs.glob("AdskLicensingService*.log")):
    if path.stat().st_mtime < marker.timestamp() - 20:
        continue
    hits = deque(maxlen=25)
    count = 0
    with path.open(errors="replace") as stream:
        for line in stream:
            match = clock.search(line)
            if not match:
                continue
            h, m, s = map(int, match.groups())
            if h > 23 or m > 59 or s > 59:
                continue
            offsets = [(mark.replace(hour=h, minute=m, second=s, microsecond=0) - mark).total_seconds()
                       for mark in (marker, marker.astimezone(timezone.utc))]
            delta = min(offsets, key=abs)
            if -20 <= delta <= 45:
                count += 1
                if matched := tags.findall(line):
                    new_codes = status.findall(line)
                    hits.append(f"{delta:+.0f}s {match[0]} {','.join(dict.fromkeys(matched + new_codes))}")
    print(f"\n{path.name} entries_around_marker={count}")
    print("\n".join(hits) or "no selected events")
PY
    if grep -Fxq 'dialog_origin_trace=1' "$work/COMBINED-BUILDINFO"; then
        grep -hE ':taskdialog:.* licensing_dialog (caller|frame)=' "$ACAD_LOGDIR"/acad-*.log \
            | tee -a "$ACAD_LOGDIR/service-summary.txt" || true
    fi
    printf 'Summary: %s/service-summary.txt\n' "$ACAD_LOGDIR"
fi
printf 'Output directory: %s\n' "$ACAD_LOGDIR"
exit "$rc"
