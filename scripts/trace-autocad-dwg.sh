#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
set -euo pipefail
umask 077

usage() {
    echo 'Usage: trace-autocad-dwg.sh candidate|baseline idle|save-0|open-0|save-1|open-1' >&2
    echo '       trace-autocad-dwg.sh candidate|baseline startup-open|startup-safe|startup-wpf-software|core-open|open-events|open-0 KNOWN_GOOD_DWG' >&2
    exit 2
}

[[ $# -ge 2 && $# -le 3 && "$1" =~ ^(candidate|baseline)$ && "$2" =~ ^(idle|(save|open)-[01]|startup-(open|safe|wpf-software)|core-open|open-events)$ ]] || usage
if [[ "$2" == startup-* || "$2" == core-open || "$2" == open-events ]]; then
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

launch_args=()
launcher_script=scripts/launch-autocad.sh
probe_dwg=
open_events=
if [[ $# == 3 ]]; then
    test -f "$3" && test -s "$3" || { echo 'A nonempty local DWG is required.' >&2; exit 2; }
    test "$(realpath -e "$ACAD_PREFIX/drive_c")" = "$ACAD_PREFIX/drive_c"
    probe_dir="$(mktemp -d "$ACAD_PREFIX/drive_c/winum-dwg.XXXXXXXX")"
    probe_dwg="$probe_dir/probe.dwg"
    cp -- "$3" "$probe_dwg"
    probe_hash="$(sha256sum < "$probe_dwg")"
    probe_path="C:\\${probe_dir##*/}\\probe.dwg"
    python3 - "$probe_dwg" <<'PY'
from pathlib import Path
import re
import sys

file = Path(sys.argv[1])
with file.open("rb") as stream:
    signature = stream.read(6)
version = signature.decode("ascii") if re.fullmatch(rb"AC[0-9]{4}", signature) else "other"
print(f"dwg_header={version} dwg_bytes={file.stat().st_size}")
PY
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
elif [[ "$2" == open-events ]]; then
    cp -- "${ACAD_OPEN_EVENTS_LISP:?Set the path to dwg-open-events.lsp}" "$probe_dir/open-events.lsp"
    open_events="$probe_dir/open-events.txt"
    probe_slash="C:/${probe_dir##*/}"
    echo 'Create a blank drawing, set FILEDIA to 0, then paste this AutoLISP line:'
    printf '(progn (load "%s/open-events.lsp") (winum-open-trace "%s/open-events.txt"))\n' "$probe_slash" "$probe_slash"
    printf 'Run ._REGEN, then ._OPEN with this path: %s\n' "$probe_path"
elif [[ "$2" == idle ]]; then
    printf 'Starting %s. Stay on the AutoCAD start screen for 60 seconds, then close AutoCAD.\n' "$1"
else
    printf 'Starting %s. Set FILEDIA to %s and try %s once. Close AutoCAD after the trial.\n' \
        "$1" "${2##*-}" "${2%%-*}"
    if [[ -n "$probe_dwg" ]]; then printf 'Path for OPEN: %s\n' "$probe_path"; fi
fi
eventpipe_dir=
if [[ "${ACAD_EVENTPIPE:-0}" == 1 ]]; then
    test "$(realpath -e "$ACAD_PREFIX/drive_c")" = "$ACAD_PREFIX/drive_c"
    eventpipe_dir="$(mktemp -d "$ACAD_PREFIX/drive_c/winum-eventpipe.XXXXXXXX")"
    export DOTNET_EnableEventPipe=1
    export DOTNET_EventPipeOutputStreaming=1
    export DOTNET_EventPipeCircularMB=20
    export DOTNET_EventPipeConfig='Microsoft-Windows-DotNETRuntime:8000:5'
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
if [[ -n "$open_events" ]]; then
    if [[ -s "$open_events" ]]; then
        awk '{
            sub(/\r$/, "")
            event = tolower($0)
            if (event ~ /^(reactors_ready|control_regen|command_open|:vlr-(begindwgopen|dwgfileopened|enddwgopen|documentcreated|documenttobeactivated|documentbecamecurrent))$/) {
                print "open_event=" event
                seen = 1
            }
        } END { if (!seen) print "open_event=unconfirmed" }' "$open_events"
    else
        echo 'open_event=unconfirmed'
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
python3 - "${logs[0]}" "$eventpipe_dir" "$2" <<'PY'
from collections import Counter, deque
import mmap
from pathlib import Path
import re
import sys

prefix = re.compile(r"^([0-9a-f]+):([0-9a-f]+):", re.I)
mapped = re.compile(r'\bmapping PE file L?"[^"]*[\\/]([a-z0-9_.-]+)" at (0x[0-9a-f]+)-(0x[0-9a-f]+)', re.I)
loaded = re.compile(r'\bLoaded L?"([^"]*[\\/]([a-z0-9_.-]+))" at (?:0x)?([0-9a-f]+): (\w+)', re.I)
unloaded = re.compile(r'\b(?:unloading|unloaded module) L?"([^"]+)"', re.I)
known_modules = {"coreclr.dll", "clr.dll", "shell32.dll", "msxml3.dll",
                 "d3dcompiler_47.dll", "d3dcompiler_47_cor3.dll", "wpfgfx_cor3.dll"}
reg_load_appkey = re.compile(r":fixme:reg:RegLoadAppKey[AW]\b")
idle_trace_types = (
    "System.IO.FileNotFoundException",
    "System.ServiceModel.EndpointNotFoundException",
    "Autodesk.DesktopConnector.API.DesktopConnectorNotInstalledException",
)
unhandled = re.compile(r"Unhandled exception code ([0-9a-f]+) flags ([0-9a-f]+) addr (?:0x)?([0-9a-f]+)", re.I)
exception = re.compile(r"\bcode=(?:0x)?([0-9a-f]+) flags=(?:0x)?([0-9a-f]+) addr=(?:0x)?([0-9a-f]+)", re.I)
stack = re.compile(r"^\s*(?:=>)?\s*\d+\s+0x[0-9a-f]+.*\bin ([a-z0-9_.-]+)"
                   r"(?: \(\+0x([0-9a-f]+)\))? \(0x[0-9a-f]+\)\s*$", re.I)
managed_type = re.compile(r"\b(?:System|Microsoft)(?:\.[A-Za-z_][A-Za-z0-9_]*)*"
                          r"\.(?:[A-Za-z_][A-Za-z0-9_]*)?Exception\b")

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
acad_pids = set()
trueview_pids = set()
clr_pids = set()
reg_loads = Counter()
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

with open(sys.argv[1], encoding="utf-8", errors="replace") as log:
    for line in log:
        if sys.argv[3] == "core-open":
            match = re.fullmatch(r"WINUM_DWG_OPEN=([01]) LINES=([0-9]+)",
                                 line.replace("\x00", "").strip().lstrip("\ufeff"))
            if match:
                console_dwg = match.groups()
        ids = prefix.match(line)
        pid, tid = ids.groups() if ids else ("?", "?")
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
                if name in known_modules:
                    modules[pid, name] = (base, kind)
                mapped_name, end = regions.get((pid, base), (None, base))
                if mapped_name == name:
                    active[pid, path.lower()] = (name, base, end)
        if (":trace:loaddll:free_modref " in line or ":trace:module:free_modref " in line):
            if match := unloaded.search(line):
                active.pop((pid, match[1].lower()), None)
        if ":seh:" in line and (match := unhandled.search(line)):
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
