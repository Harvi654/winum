#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
set -euo pipefail
umask 077

usage() {
    echo 'Usage: trace-autocad-dwg.sh candidate|baseline idle|save-0|open-0|save-1|open-1|new-document|save-events|save-activex' >&2
    echo '       trace-autocad-dwg.sh candidate|baseline startup-open|startup-safe|startup-wpf-software|core-open KNOWN_GOOD_DWG' >&2
    echo '       trace-autocad-dwg.sh candidate|baseline open-events|open-activex|open-dbx|open-0 KNOWN_GOOD_DWG' >&2
    exit 2
}

[[ $# -ge 2 && $# -le 3 && "$1" =~ ^(candidate|baseline)$ && "$2" =~ ^(idle|new-document|save-(events|activex)|(save|open)-[01]|startup-(open|safe|wpf-software)|core-open|open-(events|activex|dbx))$ ]] || usage
if [[ "$2" =~ ^(startup-.*|core-open|open-(events|activex|dbx))$ ]]; then
    [[ $# == 3 ]] || usage
elif [[ $# == 3 && "$2" != open-0 ]]; then
    usage
fi

work="$(realpath -e "$ACAD_AUTOCOMPLETE_WORK")"
test -s "$work/base-runtime.txt"
case "$1" in
    candidate)
        export ACAD_PREFIX="$work/candidate-prefix"
        export ACAD_WINE_BIN="$work/wine/bin"
        ;;
    baseline)
        export ACAD_PREFIX="$work/base-prefix"
        export ACAD_WINE_BIN="$(cat "$work/base-runtime.txt")/bin"
        ;;
esac
test "$(realpath -e "$ACAD_PREFIX")" = "$ACAD_PREFIX"
acad_dir="$ACAD_PREFIX/drive_c/Program Files/Autodesk/AutoCAD 2027"
test -f "$acad_dir/acad.exe"
acad_image=acad.exe
if [[ "$2" == core-open ]]; then
    acad_image=accoreconsole.exe
    if [[ ! -f "$acad_dir/$acad_image" ]]; then
        echo 'core_console=not_installed' >&2
        exit 2
    fi
fi
export WINEPREFIX="$ACAD_PREFIX"
export INV_PREFIX="$ACAD_PREFIX"
export INV_WINE=system
export PATH="$ACAD_WINE_BIN:$PATH"
export WINE="$ACAD_WINE_BIN/wine"
export WINESERVER="$ACAD_WINE_BIN/wineserver"
export WINELOADER="$WINE"
export ACAD_LOGDIR="$(mktemp -d "$work/dwg-$1-$2.XXXXXXXX")"
export WINEDEBUG="-all,+pid,$acad_image:+seh,$acad_image:+loaddll,$acad_image:+module,$acad_image:+eventlog,$acad_image:fixme+reg,dwgviewr.exe:+loaddll,dwgviewr.exe:fixme+reg"
cd "${ACAD_REPO:-$HOME/autocad-on-linux}"
test -x "$WINE"
test -f scripts/launch-autocad.sh

if ! bash -c '
    source scripts/common-env.sh || exit 1
    cd "$INVENTOR_DIR" || exit 1
    source scripts/lib/common.sh || exit 1
    test "$(realpath -e "$WINEPREFIX")" = "$(realpath -e "$1")" &&
    test "$(realpath -e "$(type -P wine)")" = "$(realpath -e "$2/wine")" &&
    test "$(realpath -e "$(type -P wineserver)")" = "$(realpath -e "$2/wineserver")"
' bash "$ACAD_PREFIX" "$ACAD_WINE_BIN" > "$ACAD_LOGDIR/preflight.txt" 2>&1; then
    echo 'Launcher prefix or runtime check failed. AutoCAD was not started.' >&2
    exit 1
fi

restore_wpf_rendering() {
    if [[ "$wpf_previous" == absent ]]; then
        if "$WINE" reg delete "$wpf_key" /v DisableHWAcceleration /f > "$ACAD_LOGDIR/wpf-restore.txt" 2>&1; then
            echo 'wpf_registry_restored=absent'
        else
            echo 'wpf_registry_restored=failed'
        fi
    elif "$WINE" reg add "$wpf_key" /v DisableHWAcceleration /t REG_DWORD /d "$wpf_previous" /f > "$ACAD_LOGDIR/wpf-restore.txt" 2>&1; then
        echo 'wpf_registry_restored=0'
    else
        echo 'wpf_registry_restored=failed'
    fi
}

restore_recent_history() {
    local key
    local log="$ACAD_LOGDIR/recent-restore.txt"
    local errors="$ACAD_LOGDIR/recent-restore-stderr.txt"
    if [[ "$recent_previous" == 0 ]]; then
        if "$WINE" reg add "$recent_key" /v NoRecentDocsHistory /t REG_DWORD /d 0 /f > "$log" 2>&1 &&
            "$WINE" reg query "$recent_key" /v NoRecentDocsHistory >> "$log" 2>&1 &&
            grep -Eq "$recent_dword"'0+[[:space:]]*$' "$log"; then
            echo 'recent_history_restored=0'
        else
            echo 'recent_history_restored=failed'
        fi
        return
    fi

    if ! "$WINE" reg delete "$recent_key" /v NoRecentDocsHistory /f > "$log" 2>&1; then
        echo 'recent_history_restored=failed'
        return
    fi
    for key in "${recent_created_keys[@]}"; do
        if "$WINE" reg query "$key" > "$log" 2>> "$errors" &&
            ! grep -q '[^[:space:]]' "$log" &&
            "$WINE" reg delete "$key" /f >> "$log" 2>&1; then
            continue
        fi
        echo 'recent_history_restored=value_removed_key_retained'
        return
    done
    echo 'recent_history_restored=absent'
}

disable_recent_history() {
    local policies='Software\Microsoft\Windows\CurrentVersion\Policies'
    local log="$ACAD_LOGDIR/recent-policy.txt"
    local query_rc=0
    recent_created_keys=()
    recent_dword='^[[:space:]]*NoRecentDocsHistory[[:space:]]+REG_DWORD[[:space:]]+0x'

    # Wine checks HKCU only if it cannot open the Explorer policies key in HKLM.
    recent_key="HKLM\\$policies\\Explorer"
    if ! "$WINE" reg query "$recent_key" > "$log" 2>&1; then
        recent_key="HKCU\\$policies\\Explorer"
        if ! "$WINE" reg query "$recent_key" > "$log" 2>&1; then
            recent_created_keys+=("$recent_key")
            if ! "$WINE" reg query "HKCU\\$policies" > "$log" 2>&1; then
                recent_created_keys+=("HKCU\\$policies")
            fi
        fi
    fi
    "$WINE" reg query "$recent_key" /v NoRecentDocsHistory > "$log" 2>&1 || query_rc=$?
    case "$query_rc" in
        0)
            grep -Eq "$recent_dword"'0+[[:space:]]*$' "$log" || return 1
            recent_previous=0
            ;;
        1) recent_previous=absent ;;
        *) return 1 ;;
    esac

    trap restore_recent_history EXIT
    trap 'exit 130' INT
    trap 'exit 143' TERM
    "$WINE" reg add "$recent_key" /v NoRecentDocsHistory /t REG_DWORD /d 1 /f > "$log" 2>&1 &&
        "$WINE" reg query "$recent_key" /v NoRecentDocsHistory >> "$log" 2>&1 &&
        grep -Eq "$recent_dword"'0*1[[:space:]]*$' "$log" || return 1
    echo 'recent_history_policy=disabled'
}

print_dwg_info() {
    python3 - "$1" <<'PY'
from pathlib import Path
import re
import sys

file = Path(sys.argv[1])
with file.open("rb") as stream:
    signature = stream.read(6)
version = signature.decode("ascii") if re.fullmatch(rb"AC[0-9]{4}", signature) else "other"
print(f"dwg_header={version} dwg_bytes={file.stat().st_size}")
PY
}

launch_args=()
launcher_script=scripts/launch-autocad.sh
probe_dwg=
save_dwg=
open_events=
file_subject=
recovery_subject=
api_subject=
event_label=open_event
if [[ $# == 3 && ( ! -f "$3" || ! -s "$3" ) ]]; then
    echo 'A nonempty local DWG is required.' >&2
    exit 2
fi
if [[ $# == 3 || "$2" =~ ^(new-document|save-(events|activex))$ ]]; then
    test "$(realpath -e "$ACAD_PREFIX/drive_c")" = "$ACAD_PREFIX/drive_c"
    probe_dir="$(mktemp -d "$ACAD_PREFIX/drive_c/winum-dwg.XXXXXXXX")"
fi
if [[ $# == 3 ]]; then
    probe_dwg="$probe_dir/probe.dwg"
    cp -- "$3" "$probe_dwg"
    probe_hash="$(sha256sum < "$probe_dwg")"
    probe_path="C:\\${probe_dir##*/}\\probe.dwg"
    print_dwg_info "$probe_dwg"
fi

if [[ "${ACAD_DWG_FILE_TRACE:-0}" == 1 ]]; then
    case "$2" in
        open-activex|open-dbx)
            file_subject="${probe_dir##*/}"
            export WINEDEBUG="$WINEDEBUG,$acad_image:+file"
            ;;
        *)
            echo 'File tracing requires open-activex or open-dbx.' >&2
            exit 2
            ;;
    esac
fi
if [[ "${ACAD_DWG_RECOVERY_TRACE:-0}" == 1 ]]; then
    if [[ "$2" != save-events ]]; then
        echo 'Drawing Recovery tracing requires save-events.' >&2
        exit 2
    fi
    recovery_subject="${probe_dir##*/}"
    export WINEDEBUG="$WINEDEBUG,$acad_image:+file,$acad_image:+reg"
fi
if [[ "${ACAD_DWG_API_TRACE:-0}" == 1 ]]; then
    if [[ "$2" != save-events ]]; then
        echo 'API tracing requires save-events.' >&2
        exit 2
    fi
    api_subject="${probe_dir##*/}"
    export WINEDEBUG="$WINEDEBUG,$acad_image:fixme+all,$acad_image:err+all,$acad_image:+file"
fi

if [[ "$2" == core-open ]]; then
    # Reuse the launcher's prefix setup and service startup in a private copy.
    if [[ "$(grep -c '^wine acad\.exe ' "$launcher_script")" != 1 ]]; then
        echo 'core_console=unsupported_launcher' >&2
        exit 2
    fi
    launcher_dir="$ACAD_LOGDIR/core-launcher"
    mkdir -- "$launcher_dir"
    ln -s -- "$(realpath -e scripts/common-env.sh)" "$launcher_dir/common-env.sh"
    sed 's/^wine acad\.exe /wine accoreconsole.exe /' "$launcher_script" > "$launcher_dir/launch.sh"
    launcher_script="$launcher_dir/launch.sh"
    printf '%s\r\n' \
        '(progn (setq winum-lines (ssget "_X" (quote ((0 . "LINE"))))) (princ (strcat "\nWINUM_DWG_OPEN=" (if (= (strcase (getvar "DWGNAME")) "PROBE.DWG") "1" "0") " LINES=" (itoa (if winum-lines (sslength winum-lines) 0)) "\n")) (princ))' \
        '_.QUIT' > "$probe_dir/probe.scr"
    launch_args=(/i "$probe_path" /s "C:\\${probe_dir##*/}\\probe.scr" /l en-US)
    echo 'Opening the scratch DWG in AutoCAD Core Console.'
elif [[ "$2" == startup-safe ]]; then
    launch_args=("$probe_path" /safemode)
    echo 'Opening the scratch DWG in desktop AutoCAD with /safemode.'
elif [[ "$2" == startup-wpf-software ]]; then
    launch_args=("$probe_path")
    wpf_key='HKCU\Software\Microsoft\Avalon.Graphics'
    wpf_previous=absent
    if ! "$WINE" reg query 'HKCU\Software\Microsoft' > "$ACAD_LOGDIR/wpf-registry-parent.txt" 2>&1; then
        echo 'wpf_registry=unavailable' >&2
        exit 2
    fi
    if "$WINE" reg query "$wpf_key" /v DisableHWAcceleration > "$ACAD_LOGDIR/wpf-before.txt" 2>&1; then
        wpf_previous="$(awk '$1 == "DisableHWAcceleration" && $2 == "REG_DWORD" { sub(/\r$/, "", $3); print $3 }' "$ACAD_LOGDIR/wpf-before.txt")"
        if [[ "$wpf_previous" != 0x0 ]]; then
            echo 'wpf_registry=baseline_not_default' >&2
            exit 2
        fi
    elif [[ $? != 1 ]]; then
        echo 'wpf_registry=unavailable' >&2
        exit 2
    fi

    trap restore_wpf_rendering EXIT
    if ! "$WINE" reg add "$wpf_key" /v DisableHWAcceleration /t REG_DWORD /d 1 /f > "$ACAD_LOGDIR/wpf-set.txt" 2>&1; then
        echo 'wpf_registry=setup_failed' >&2
        exit 2
    fi
    "$WINESERVER" -k >> "$ACAD_LOGDIR/wpf-set.txt" 2>&1 || true
    if ! "$WINESERVER" -w >> "$ACAD_LOGDIR/wpf-set.txt" 2>&1 ||
       ! "$WINE" reg query "$wpf_key" /v DisableHWAcceleration > "$ACAD_LOGDIR/wpf-verify.txt" 2>&1 ||
       ! grep -Eq '^[[:space:]]*DisableHWAcceleration[[:space:]]+REG_DWORD[[:space:]]+0x1[[:space:]]*$' "$ACAD_LOGDIR/wpf-verify.txt"; then
        echo 'wpf_registry=setup_failed' >&2
        exit 2
    fi
    echo 'wpf_registry=software_only'
    echo 'Opening the scratch DWG in desktop AutoCAD with WPF hardware acceleration disabled.'
elif [[ "$2" == startup-open ]]; then
    launch_args=("$probe_path")
    printf 'Opening a scratch DWG at startup. If the drawing opens, edit it, run QSAVE, then close AutoCAD.\n'
elif [[ "$2" =~ ^(new-document|save-(events|activex)|open-(events|activex|dbx))$ ]]; then
    cp -- "${ACAD_OPEN_EVENTS_LISP:?Set the path to dwg-open-events.lsp}" "$probe_dir/open-events.lsp"
    open_events="$probe_dir/open-events.txt"
    probe_slash="C:/${probe_dir##*/}"
    echo 'Create a blank drawing, set FILEDIA to 0, then paste this AutoLISP line:'
    if [[ "$2" != save-* ]]; then
        printf '(progn (load "%s/open-events.lsp") ' "$probe_slash"
        if [[ -n "$file_subject" ]]; then
            printf '(setq winum-trace-file-window T) '
        fi
        printf '(winum-open-trace "%s/open-events.txt"))\n' "$probe_slash"
    fi
    case "$2" in
        open-events)
            printf 'Run ._REGEN, then ._OPEN with this path: %s\n' "$probe_path"
            ;;
        open-activex|open-dbx)
            echo 'Run ._REGEN, then paste the second AutoLISP line:'
            printf '(winum-%s "%s/open-events.txt" "%s/probe.dwg")\n' "$2" "$probe_slash" "$probe_slash"
            ;;
        new-document)
            echo 'Run ._REGEN, then paste this line once to add and switch drawings:'
            printf '(winum-new-document "%s/open-events.txt")\n' "$probe_slash"
            ;;
        save-events|save-activex)
            save_dwg="$probe_dir/saved.dwg"
            event_label=save_event
            printf '(progn (load "%s/open-events.lsp") ' "$probe_slash"
            if [[ -n "$recovery_subject" || -n "$api_subject" ]]; then
                printf '(setq winum-trace-file-window T) '
            fi
            printf '(winum-save-trace "%s/open-events.txt" "%s/saved.dwg"))\n' "$probe_slash" "$probe_slash"
            if [[ "$2" == save-activex ]]; then
                echo 'Draw one line, run ._REGEN, then paste the ActiveX save call once:'
                printf '(winum-save-activex "%s/open-events.txt" "%s/saved.dwg")\n' "$probe_slash" "$probe_slash"
            else
                printf 'Draw one line, run ._REGEN, then ._SAVEAS with the default DWG format and this path: C:\\%s\\saved.dwg\n' \
                    "${probe_dir##*/}"
            fi
            ;;
    esac
elif [[ "$2" == idle ]]; then
    printf 'Starting %s. Stay on the AutoCAD start screen for 60 seconds, then close AutoCAD.\n' "$1"
else
    printf 'Starting %s. Set FILEDIA to %s and try %s once. Close AutoCAD after the trial.\n' \
        "$1" "${2##*-}" "${2%%-*}"
    if [[ -n "$probe_dwg" ]]; then printf 'Path for OPEN: %s\n' "$probe_path"; fi
fi
shell_subject=
if [[ "${ACAD_DWG_SHELL_TRACE:-0}" == 1 || "${ACAD_DWG_NO_RECENT:-0}" == 1 ]]; then
    case "$2" in
        open-activex|save-activex|save-events)
            shell_subject="${probe_dir##*/}"
            export WINEDEBUG="$WINEDEBUG,$acad_image:+shell"
            ;;
        *)
            echo 'Shell tracing requires open-activex, save-activex, or save-events.' >&2
            exit 2
            ;;
    esac
fi
if [[ "${ACAD_DWG_NO_RECENT:-0}" == 1 ]]; then
    if ! disable_recent_history; then
        echo 'recent_history_policy=setup_failed' >&2
        exit 2
    fi
fi
eventpipe_dir=
if [[ "${ACAD_EVENTPIPE:-0}" == 1 ]]; then
    test "$(realpath -e "$ACAD_PREFIX/drive_c")" = "$ACAD_PREFIX/drive_c"
    eventpipe_dir="$(mktemp -d "$ACAD_PREFIX/drive_c/winum-eventpipe.XXXXXXXX")"
    export DOTNET_EnableEventPipe=1
    export DOTNET_EventPipeOutputStreaming=1
    export DOTNET_EventPipeCircularMB=20
    export DOTNET_EventPipeConfig='Microsoft-Windows-DotNETRuntime:8000:5'
    if [[ "${ACAD_EVENTPIPE_METHODS:-0}" == 1 ]]; then
        export DOTNET_EventPipeConfig='Microsoft-Windows-DotNETRuntime:8018:5'
        printf 'eventpipe_run=%s\n' "${eventpipe_dir##*/}"
    fi
    export DOTNET_EventPipeOutputPath="C:\\${eventpipe_dir##*/}\\events-{pid}.nettrace"
fi
rc=0
launcher=(bash "$launcher_script" "${launch_args[@]}")
dwg_io_trace=
if [[ "${ACAD_DWG_IO:-0}" == 1 ]]; then
    if [[ -z "$probe_dwg" ]] || ! command -v strace >/dev/null; then
        echo 'DWG I/O tracing requires a DWG argument and strace.' >&2
        exit 2
    fi
    dwg_io_trace="$ACAD_LOGDIR/dwg-io.txt"
    launcher=(strace -f -qq -s 0 -P "$probe_dwg"
        -e trace=open,openat,openat2,read,pread64,readv,preadv,preadv2,mmap
        -o "$dwg_io_trace" -- "${launcher[@]}")
fi
if [[ -n "${ACAD_FAILFAST_OBSERVER:-}" ]]; then
    test -f "$ACAD_FAILFAST_OBSERVER"
    "${launcher[@]}" > "$ACAD_LOGDIR/launcher.txt" 2>&1 &
    launcher_pid=$!
    if read -r -p 'Wait for AutoCAD to open, then press Enter here before the file operation. '; then
        observer_rc=0
        "$WINE" "$ACAD_FAILFAST_OBSERVER" --autocad 2>"$ACAD_LOGDIR/observer-stderr.txt" || observer_rc=$?
        printf 'observer_exit=%s\n' "$observer_rc"
    fi
    wait "$launcher_pid" || rc=$?
else
    "${launcher[@]}" > "$ACAD_LOGDIR/launcher.txt" 2>&1 || rc=$?
fi
printf 'variant=%s trial=%s launcher_exit=%s\n' "$1" "$2" "$rc"
if [[ -n "$probe_dwg" ]]; then
    if [[ ! -f "$probe_dwg" ]]; then
        echo 'dwg_copy_after=missing'
    elif [[ "$(sha256sum < "$probe_dwg")" == "$probe_hash" ]]; then
        echo 'dwg_copy_after=unchanged'
    else
        echo 'dwg_copy_after=changed'
    fi
fi
if [[ -n "$save_dwg" ]]; then
    if [[ -f "$save_dwg" ]]; then
        echo 'dwg_saved=present'
        print_dwg_info "$save_dwg"
    else
        echo 'dwg_saved=missing'
    fi
fi
if [[ -n "$open_events" ]]; then
    if [[ -s "$open_events" ]]; then
        awk -v key="$event_label" '{
            sub(/\r$/, "")
            event = tolower($0)
            if (event ~ /^(reactors_ready|control_regen|command_open|activex_(begin|open_enter|open_return|open_expected_dwg|error)|:vlr-(begindwgopen|dwgfileopened|enddwgopen|documentcreated|documenttobeactivated|documentbecamecurrent))$/ ||
                event ~ /^dbx_(begin|object_unavailable|open_enter|open_return|lines=[0-9]+|active_document=(unchanged|changed)|error)$/ ||
                event ~ /^new_(begin|add_enter|add_return|count_delta=-?[0-9]+|add_active=(created|other)|activate_(original|created)_(enter|active|other)|error)$/ ||
                event ~ /^(save_(reactors_ready|begin|target_complete|other_complete)|activex_save_(enter|return|error)|:vlr-command(willstart|ended|cancelled|failed))$/) {
                print key "=" event
                seen = 1
            }
        } END { if (!seen) print key "=unconfirmed" }' "$open_events"
    else
        printf '%s=unconfirmed\n' "$event_label"
    fi
fi
if [[ -n "$dwg_io_trace" ]]; then
    python3 - "$dwg_io_trace" <<'PY'
from pathlib import Path
import re
import sys

trace = Path(sys.argv[1])
if not trace.is_file():
    print("dwg_io_trace=missing")
    raise SystemExit(0)

syscall = re.compile(r"^\s*(?:\d+\s+)?(?:(\w+)\(|<\.\.\. (\w+) resumed>)")
result = re.compile(r"\)\s+=\s+(0x[0-9a-f]+|-?[0-9]+)(?=\s|$)", re.I)
opens = errors = reads = read_bytes = mappings = 0
with trace.open(encoding="utf-8", errors="replace") as stream:
    for line in stream:
        call, status = syscall.match(line), result.search(line)
        if not call or not status:
            continue
        name = call[1] or call[2]
        value = int(status[1], 0)
        if name in {"open", "openat", "openat2"}:
            opens += value >= 0
            errors += value < 0
        elif name == "mmap":
            mappings += value >= 0
        elif value > 0:
            reads += 1
            read_bytes += value
print(f"dwg_open_ok={opens} dwg_open_failed={errors} dwg_read_ok={reads} "
      f"dwg_read_bytes={read_bytes} dwg_mmap_ok={mappings}")
PY
fi
shopt -s nullglob
logs=("$ACAD_LOGDIR"/acad-*.log)
if [[ "${#logs[@]}" != 1 ]]; then
    printf 'acad_logs=%s\n' "${#logs[@]}"
    exit "$rc"
fi

# Only print module names and numeric exception fields. In particular,
# keep the Windows paths and debugger preamble inside the private run directory.
python3 - "${logs[0]}" "$eventpipe_dir" "$2" "$shell_subject" "$file_subject" "$recovery_subject" "$api_subject" <<'PY'
from collections import Counter, deque
import mmap
import os
from pathlib import Path
import re
import sys

prefix = re.compile(r"^([0-9a-f]+):([0-9a-f]+):", re.I)
mapped = re.compile(r'\bmapping PE file L?"[^"]*[\\/]([a-z0-9_.-]+)" at (0x[0-9a-f]+)-(0x[0-9a-f]+)', re.I)
loaded = re.compile(r'\bLoaded L?"([^"]*[\\/]([a-z0-9_.-]+))" at (?:0x)?([0-9a-f]+): (\w+)', re.I)
unloaded = re.compile(r'\b(?:unloading|unloaded module) L?"([^"]+)"', re.I)
known_modules = {"coreclr.dll", "clr.dll", "mscoree.dll", "shell32.dll", "msxml3.dll", "combase.dll", "oleaut32.dll",
                 "d3dcompiler_47.dll", "d3dcompiler_47_cor3.dll", "wpfgfx_cor3.dll"}
runtime_modules = {"coreclr.dll", "hostfxr.dll", "hostpolicy.dll", "wpfgfx_cor3.dll"}
runtime_path = re.compile(r'[\\/]+dotnet[\\/]+(?:host[\\/]+fxr|shared[\\/]+'
                          r'microsoft\.(?:netcore|windowsdesktop)\.app)[\\/]+'
                          r'([0-9]+\.[0-9]+\.[0-9]+)[\\/]+', re.I)
reg_load_appkey = re.compile(r":fixme:reg:RegLoadAppKey[AW]\b")
shell_functions = ("SHAddToRecentDocs", "SHChangeNotify", "SHCreateItemFromParsingName",
                   "SHGetFileInfoW", "SHGetPropertyStoreFromParsingName", "IPersistFile_fnSaveCompleted")
shell_call = re.compile(r"(?:trace|warn|fixme|err):shell:(" + "|".join(shell_functions) + r")\b")
recent_policy_trace = re.compile(r"\s+policy value for NoRecentDocsHistory = 0*1\s*$")
shell_subject = sys.argv[4].lower()
file_subject = sys.argv[5].lower()
recovery_subject = sys.argv[6].lower()
api_subject = sys.argv[7].lower()
recovery_query = re.compile(r':trace:reg:NtQueryValueKey\s+\([^,]+,L"LoggedFileCount",', re.I)
file_markers = {
    'open-activex': ('activex_open_enter', 'activex_open_return', 'activex_error'),
    'open-dbx': ('dbx_open_enter', 'dbx_open_return', 'dbx_error'),
}
file_miss = re.compile(r':warn:file:NtCreateFile L"([^"\r\n]+)" not found \(([a-f0-9]{8})\)', re.I)
file_basename = re.compile(r'[a-z0-9_.+-]{1,96}\.(?:dll|exe|arx|dbx|crx|hdi|ocx|tlb|ini|json|xml|config|manifest|dwt|aws|resx)')
file_private_names = {name.lower() for name in (os.getenv('USER'), os.getenv('LOGNAME'), Path.home().name) if name}
idle_trace_types = (
    "System.IO.FileNotFoundException",
    "System.ServiceModel.EndpointNotFoundException",
    "Autodesk.DesktopConnector.API.DesktopConnectorNotInstalledException",
)
unhandled = re.compile(r"Unhandled exception code ([0-9a-f]+) flags ([0-9a-f]+) addr (?:0x)?([0-9a-f]+)", re.I)
exception = re.compile(r"\bcode=(?:0x)?([0-9a-f]+) flags=(?:0x)?([0-9a-f]+) addr=(?:0x)?([0-9a-f]+)", re.I)
api_message = re.compile(r'(fixme|err):([a-z0-9_]{1,32}):([a-zA-Z_][a-zA-Z0-9_]{0,95})\s')
api_stub_hint = re.compile(r'\b(?:stub|not implemented|unimplemented|E_NOTIMPL|STATUS_NOT_IMPLEMENTED|'
                           r'ERROR_CALL_NOT_IMPLEMENTED)\b', re.I)
api_exception = re.compile(r'\bcode=(?:0x)?(80000100|80004001|80070078|c0000002)'
                           r'(?: \([a-z_0-9]+\))? flags=', re.I)
stack = re.compile(r"^\s*(?:=>)?\s*\d+\s+0x[0-9a-f]+.*\bin ([a-z0-9_.-]+)"
                   r"(?: \(\+0x([0-9a-f]+)\))? \(0x[0-9a-f]+\)\s*$", re.I)
managed_type = re.compile(r"\b(?:System|Microsoft)(?:\.[A-Za-z_][A-Za-z0-9_]*)*"
                          r"\.(?:[A-Za-z_][A-Za-z0-9_]*)?Exception\b")

def summarize_file_path(raw):
    path = re.sub(r'[\\/]+', '/', raw.lower())
    if file_subject in path:
        return 'scratch', '<redacted>'
    if '/program files/autodesk/' in path or '/program files (x86)/common files/autodesk shared/' in path:
        scope = 'autodesk'
    elif '/program files/dotnet/' in path:
        scope = 'dotnet'
    elif '/windows/system32/' in path or '/windows/syswow64/' in path:
        scope = 'system'
    elif '/appdata/' in path:
        return 'appdata', '<redacted>'
    else:
        return 'other', '<redacted>'
    name = path.rsplit('/', 1)[-1]
    if not file_basename.fullmatch(name) or any(private in name for private in file_private_names):
        name = '<redacted>'
    return scope, name

def eventpipe_summary(folder, pid):
    print(f"eventpipe_files={sum(1 for _ in folder.glob('events-*.nettrace'))}")
    if pid is None:
        print("eventpipe_status=no_unique_acad_pid")
        return
    print(f"eventpipe_pid=0x{pid}")
    trace = folder / f"events-{int(pid, 16)}.nettrace"
    if not trace.is_file():
        print("eventpipe_status=missing_for_acad_pid")
        return
    size = trace.stat().st_size
    print(f"eventpipe_trace_bytes={size}")
    if not size:
        print("eventpipe_status=empty")
        return

    # Name hints from the exception-only stream. Match both payload string
    # encodings without printing messages or inferring per-thread ordering.
    matches = []
    with trace.open("rb") as stream, mmap.mmap(stream.fileno(), 0, access=mmap.ACCESS_READ) as data:
        if data[:8] != b"Nettrace":
            print("eventpipe_status=unrecognized_header")
            return
        for encoding in ("ascii", "utf-16-le"):
            nul = "\0".encode(encoding)
            pad = rb"\x00" if encoding == "utf-16-le" else b""
            namespaces = b"|".join(name.encode(encoding) for name in ("System", "Microsoft", "Autodesk"))
            separator = rb"[.+]" + pad
            identifier = rb"[A-Za-z_]" + pad + rb"(?:[A-Za-z0-9_]" + pad + rb")*"
            ending = "Exception".encode(encoding)
            pattern = re.compile(rb"(?:" + namespaces + rb")(?:" + separator + identifier
                                 + rb")*" + separator + rb"(?:" + identifier + rb")?" + ending + nul)
            tail = deque(pattern.finditer(data), maxlen=24)
            matches.extend((item.start(), item[0][:-len(nul)].decode(encoding)) for item in tail)
        for name in idle_trace_types:
            seen = any(data.find((name + "\0").encode(encoding)) >= 0
                       for encoding in ("ascii", "utf-16-le"))
            print(f"eventpipe_watched_hint={name} present={int(seen)}")
    types = list(dict.fromkeys(name for _, name in sorted(matches)))[-12:]
    print(f"eventpipe_exception_hints={len(types)}")
    for name in types:
        print(f"eventpipe_exception_hint={name}")


modules = {}
runtime_versions = set()
acad_pids = set()
trueview_pids = set()
clr_pids = set()
reg_loads = Counter()
shell_lines = Counter()
shell_path_lines = Counter()
shell_no_recent = Counter()
regions = {}
active = {}
recent = deque(maxlen=8)
fatal = deque(maxlen=4)
frames = []
managed = deque(maxlen=8)
eventlog_lines = 0
stack_dispatch_failed = False
debugger_unhandled = False
console_dwg = None
file_started = {}
file_ended = {}
file_misses = Counter()
file_operation_thread = Counter()
recovery_phase = {}
recovery_queries = Counter()
api_phase = {}
api_counts = Counter()
api_fault_thread = {}

with open(sys.argv[1], encoding="utf-8", errors="replace") as log:
    for line in log:
        if sys.argv[3] == "core-open":
            match = re.fullmatch(r"WINUM_DWG_OPEN=([01]) LINES=([0-9]+)",
                                 line.replace("\x00", "").strip().lstrip("\ufeff"))
            if match:
                console_dwg = match.groups()
        ids = prefix.match(line)
        pid, tid = ids.groups() if ids else ("?", "?")
        if api_subject and ids:
            if ':trace:file:NtCreateFile ' in line and api_subject in line.lower():
                if 'open-events.txt.save_command_enter"' in line:
                    api_phase.setdefault(pid, 'command')
                elif api_phase.get(pid) == 'command' and 'open-events.txt.save_begin"' in line:
                    api_phase[pid] = 'saving'
                elif api_phase.get(pid) in {'command', 'saving'} and 'open-events.txt.save_command_exit"' in line:
                    api_phase[pid] = 'ended'
            phase = api_phase.get(pid, 'before')
            if phase in {'before', 'command', 'saving'}:
                if match := api_message.match(line, ids.end()):
                    level, channel, function = match.groups()
                    kind = 'stub_hint' if api_stub_hint.search(line, match.end()) else level
                    api_counts[pid, tid, phase, kind, f'{channel}:{function}'] += 1
                elif ':trace:seh:' in line and (match := api_exception.search(line)):
                    api_counts[pid, tid, phase, 'exception', f'0x{match[1].lower()}'] += 1
        if recovery_subject and ids:
            if ':trace:file:NtCreateFile ' in line and recovery_subject in line.lower():
                if 'open-events.txt.save_command_enter"' in line:
                    recovery_phase[pid] = 'during'
                elif pid in recovery_phase and 'open-events.txt.save_command_exit"' in line:
                    recovery_phase[pid] = 'after'
            if recovery_query.search(line):
                recovery_queries[pid, tid, recovery_phase.get(pid, 'before')] += 1
        if file_subject and pid in acad_pids:
            if ':trace:file:NtCreateFile ' in line and file_subject in line.lower():
                enter, returned, failed = file_markers[sys.argv[3]]
                if 'open-events.txt.' + enter + '"' in line:
                    file_started.setdefault(pid, tid)
                elif pid in file_started:
                    if 'open-events.txt.' + returned + '"' in line:
                        file_ended.setdefault(pid, 'return')
                    elif 'open-events.txt.' + failed + '"' in line:
                        file_ended.setdefault(pid, 'error')
            if pid in file_started and pid not in file_ended and (miss := file_miss.search(line)):
                entry = (*summarize_file_path(miss[1]), miss[2].lower())
                file_misses[entry] += 1
                if tid == file_started[pid]:
                    file_operation_thread[entry] += 1
        if shell_subject and ids and (match := shell_call.match(line, ids.end())):
            name = match[1]
            shell_lines[pid, tid, name] += 1
            if shell_subject in line[match.end():].lower():
                shell_path_lines[pid, name] += 1
            if name == "SHAddToRecentDocs" and recent_policy_trace.match(line, match.end()):
                shell_no_recent[pid] += 1
        if reg_load_appkey.search(line):
            reg_loads[pid] += 1
        if ":trace:module:map_image_into_view " in line and (match := mapped.search(line)):
            name, start, end = match.groups()
            regions[pid, int(start, 16)] = (name.lower(), int(end, 16))
        if ":trace:loaddll:" in line:
            if match := loaded.search(line):
                path, name, base, kind = match.groups()
                name, base = name.lower(), int(base, 16)
                if pid != "?":
                    if name in {"acad.exe", "accoreconsole.exe"}:
                        acad_pids.add(pid)
                    elif name == "dwgviewr.exe":
                        trueview_pids.add(pid)
                    elif name == "coreclr.dll":
                        clr_pids.add(pid)
                if name in known_modules or name.startswith(('libmono-', 'mono-', 'wine-mono')):
                    modules[pid, name] = (base, kind)
                if name in runtime_modules:
                    version = runtime_path.search(path)
                    runtime_versions.add((pid, name, version[1] if version else "unconfirmed"))
                mapped_name, end = regions.get((pid, base), (None, base))
                if mapped_name == name:
                    active[pid, path.lower()] = (name, base, end)
        if (":trace:loaddll:free_modref " in line or ":trace:module:free_modref " in line):
            if match := unloaded.search(line):
                active.pop((pid, match[1].lower()), None)
        if ":seh:" in line and (match := unhandled.search(line)):
            if pid in file_started:
                file_ended.setdefault(pid, 'fault')
            if recovery_phase.get(pid) == 'during':
                recovery_phase[pid] = 'fault'
            if api_phase.get(pid) in {'command', 'saving'}:
                api_phase[pid] = 'fault_after_save_begin' if api_phase[pid] == 'saving' else 'fault_before_save_begin'
                api_fault_thread[pid] = tid
            code, flags, addr = match.groups()
            ip = int(addr, 16)
            images = [(name, ip - base) for (process, _), (name, base, end) in active.items()
                      if process == pid and base <= ip < end]
            fatal.append((pid, tid, code, flags, addr, images[0] if len(images) == 1 else None))
        elif ":seh:" in line and (match := exception.search(line)):
            recent.append((pid, tid, *match.groups()))
        if "Exception frame is not in stack limits" in line:
            stack_dispatch_failed = True
        if ":eventlog:" in line:
            eventlog_lines += 1
        if (":eventlog:" in line or ":seh:OutputDebugString" in line
                or line.lstrip().startswith("Unhandled exception.")):
            managed.extend(managed_type.findall(line))
        if line.lstrip().lower().startswith(("wine: unhandled", "unhandled exception", "unhandled page fault")):
            debugger_unhandled = True
        if len(frames) < 8 and (match := stack.search(line)):
            frames.append(match.groups())

for (pid, module), (base, kind) in sorted(modules.items()):
    print(f"loaded pid={pid} module={module} base=0x{base:x} kind={kind}")
for pid, module, version in sorted(runtime_versions):
    if pid in acad_pids:
        print(f"dotnet_loaded={module} version={version}")
if sys.argv[3] == "core-open":
    if console_dwg:
        print(f"core_console_dwg={console_dwg[0]} core_console_lines={console_dwg[1]}")
    else:
        print("core_console_dwg=unconfirmed")
print(f"trueview_processes={len(trueview_pids)}")
print(f"reg_load_appkey_acad={sum(reg_loads[pid] for pid in acad_pids)} "
      f"reg_load_appkey_trueview={sum(reg_loads[pid] for pid in trueview_pids)}")
for pid, tid, code, flags, addr in recent:
    print(f"exception pid={pid} tid={tid} code=0x{code} flags=0x{flags} address=0x{addr}")
for pid, tid, code, flags, addr, image in fatal:
    print(f"unhandled pid={pid} tid={tid} code=0x{code} flags=0x{flags} address=0x{addr}")
    if image:
        name, offset = image
        print(f"fault_module={name}+0x{offset:x}")
        if code.lower() == "c0000409" and image == ("d3dcompiler_47_cor3.dll", 0x11922D):
            print("matches_prior_fault=d3dcompiler_47_cor3.dll+0x11922d")
    else:
        print("fault_module=unknown")
if shell_subject:
    fault_thread = next(((pid, tid) for pid, tid, *_ in reversed(fatal) if pid in acad_pids), None)
    print(f"shell_trace_acad_processes={len(acad_pids)} shell_trace_fault_thread={int(fault_thread is not None)}")
    print(f"shell_recent_policy_skip_lines={sum(shell_no_recent[pid] for pid in acad_pids)}")
    for name in shell_functions:
        process_lines = sum(count for (pid, _, api), count in shell_lines.items()
                            if pid in acad_pids and api == name)
        path_lines = sum(shell_path_lines[pid, name] for pid in acad_pids)
        thread_lines = shell_lines[(*fault_thread, name)] if fault_thread else 0
        print(f"shell_api={name} process_lines={process_lines} fault_thread_lines={thread_lines} "
              f"scratch_path_lines={path_lines}")
if file_subject:
    windows = ','.join(file_ended.get(pid, 'unconfirmed') for pid in sorted(file_started)) or 'missing'
    print(f"file_window={windows} lookups={sum(file_misses.values())} distinct={len(file_misses)}")
    order = sorted(file_misses, key=lambda item: (not file_operation_thread[item], -file_misses[item], item))
    for scope, name, status in order[:10]:
        entry = scope, name, status
        print(f"file_lookup={scope} file={name} status=0x{status} count={file_misses[entry]} "
              f"operation_thread={file_operation_thread[entry]}")
    if len(order) > 10:
        print(f"file_omitted={len(order) - 10}")
if recovery_subject:
    windows = ','.join(recovery_phase[pid] for pid in sorted(acad_pids & recovery_phase.keys())) or 'unconfirmed'
    print(f"recovery_save_window={windows}")
    fault_thread = next(((pid, tid) for pid, tid, *_ in reversed(fatal) if pid in acad_pids), None)
    for phase in ('before', 'during', 'after'):
        total = sum(count for (pid, _, stage), count in recovery_queries.items()
                    if pid in acad_pids and stage == phase)
        on_fault_thread = recovery_queries[(*fault_thread, phase)] if fault_thread else 0
        print(f"recovery_query=LoggedFileCount phase={phase} count={total} fault_thread={on_fault_thread}")
if api_subject:
    observed = {pid: api_phase[pid] for pid in acad_pids if pid in api_phase}
    window = next(iter(observed.values())) if len(observed) == 1 else 'unconfirmed'
    print(f"api_save_window={window}")
    if len(observed) == 1:
        pid = next(iter(observed))
        fault_tid = api_fault_thread.get(pid)
        by_phase = Counter()
        on_fault = Counter()
        for (process, tid, phase, kind, label), count in api_counts.items():
            if process == pid:
                by_phase[(kind, label), phase] += count
                if phase != 'before' and tid == fault_tid:
                    on_fault[kind, label] += count
        signals = {key for key, phase in by_phase if phase != 'before'}
        order = sorted(signals, key=lambda key: (
            not on_fault[key], key[0] not in {'exception', 'stub_hint'},
            bool(by_phase[key, 'before']), not by_phase[key, 'saving'], key))
        print(f"api_save_signals={len(order)} shown={min(len(order), 20)}")
        for key in order[:20]:
            kind, label = key
            print(f"api_signal={kind} site={label} before={by_phase[key, 'before']} "
                  f"command={by_phase[key, 'command']} saving={by_phase[key, 'saving']} "
                  f"fault_thread={on_fault[key]}")
for module, offset in frames:
    print(f"frame module={module}" + (f" offset=0x{offset}" if offset else ""))
for name in dict.fromkeys(managed):
    print(f"managed_exception={name}")
print(f"eventlog_lines={eventlog_lines}")
print(f"stack_dispatch_failed={int(stack_dispatch_failed)} debugger_unhandled={int(debugger_unhandled)}")
if sys.argv[2]:
    pid = fatal[-1][0] if fatal and fatal[-1][0] != "?" else None
    if pid is None:
        # This log enables loaddll only for acad.exe. Prefer the executable
        # entry and require a unique match before selecting a trace file.
        candidates = acad_pids or clr_pids
        if len(candidates) == 1:
            pid = next(iter(candidates))
    eventpipe_summary(Path(sys.argv[2]), pid)
PY
exit "$rc"
