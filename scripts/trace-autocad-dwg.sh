#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
set -euo pipefail
umask 077

if [[ $# != 2 || ! "$1" =~ ^(candidate|baseline)$ || ! "$2" =~ ^(save|open)-[01]$ ]]; then
    echo 'Usage: trace-autocad-dwg.sh candidate|baseline save-0|open-0|save-1|open-1' >&2
    exit 2
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
test -f "$ACAD_PREFIX/drive_c/Program Files/Autodesk/AutoCAD 2027/acad.exe"
export WINEPREFIX="$ACAD_PREFIX"
export INV_PREFIX="$ACAD_PREFIX"
export INV_WINE=system
export PATH="$ACAD_WINE_BIN:$PATH"
export WINE="$ACAD_WINE_BIN/wine"
export WINESERVER="$ACAD_WINE_BIN/wineserver"
export WINELOADER="$WINE"
export ACAD_LOGDIR="$(mktemp -d "$work/dwg-$1-$2.XXXXXXXX")"
export WINEDEBUG='-all,+pid,acad.exe:+seh,acad.exe:+loaddll,acad.exe:+module'
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

printf 'Starting %s. Set FILEDIA to %s and try %s once. Close AutoCAD after the trial.\n' \
    "$1" "${2##*-}" "${2%%-*}"
rc=0
bash scripts/launch-autocad.sh > "$ACAD_LOGDIR/launcher.txt" 2>&1 || rc=$?
printf 'variant=%s trial=%s launcher_exit=%s\n' "$1" "$2" "$rc"
shopt -s nullglob
logs=("$ACAD_LOGDIR"/acad-*.log)
if [[ "${#logs[@]}" != 1 ]]; then
    printf 'acad_logs=%s\n' "${#logs[@]}"
    exit "$rc"
fi

# Only print module names and numeric exception fields. In particular,
# keep the Windows paths and debugger preamble inside the private run directory.
python3 - "${logs[0]}" <<'PY'
from collections import deque
import re
import sys

prefix = re.compile(r"^([0-9a-f]+):([0-9a-f]+):", re.I)
mapped = re.compile(r'\bmapping PE file L?"[^"]*[\\/]([a-z0-9_.-]+)" at (0x[0-9a-f]+)-(0x[0-9a-f]+)', re.I)
loaded = re.compile(r'\bLoaded L?"([^"]*[\\/]([a-z0-9_.-]+))" at (?:0x)?([0-9a-f]+): (\w+)', re.I)
unloaded = re.compile(r'\b(?:unloading|unloaded module) L?"([^"]+)"', re.I)
known_modules = {"shell32.dll", "d3dcompiler_47_cor3.dll", "wpfgfx_cor3.dll"}
unhandled = re.compile(r"Unhandled exception code ([0-9a-f]+) flags ([0-9a-f]+) addr (?:0x)?([0-9a-f]+)", re.I)
exception = re.compile(r"\bcode=(?:0x)?([0-9a-f]+) flags=(?:0x)?([0-9a-f]+) addr=(?:0x)?([0-9a-f]+)", re.I)
stack = re.compile(r"^\s*(?:=>)?\s*\d+\s+0x[0-9a-f]+.*\bin ([a-z0-9_.-]+)"
                   r"(?: \(\+0x([0-9a-f]+)\))? \(0x[0-9a-f]+\)\s*$", re.I)

modules = {}
regions = {}
active = {}
recent = deque(maxlen=8)
fatal = deque(maxlen=4)
frames = []
stack_dispatch_failed = False
debugger_unhandled = False

with open(sys.argv[1], encoding="utf-8", errors="replace") as log:
    for line in log:
        ids = prefix.match(line)
        pid, tid = ids.groups() if ids else ("?", "?")
        if ":trace:module:map_image_into_view " in line and (match := mapped.search(line)):
            name, start, end = match.groups()
            regions[pid, int(start, 16)] = (name.lower(), int(end, 16))
        if ":trace:loaddll:" in line:
            if match := loaded.search(line):
                path, name, base, kind = match.groups()
                name, base = name.lower(), int(base, 16)
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
        if line.lstrip().lower().startswith(("wine: unhandled", "unhandled exception", "unhandled page fault")):
            debugger_unhandled = True
        if len(frames) < 8 and (match := stack.search(line)):
            frames.append(match.groups())

for (pid, module), (base, kind) in sorted(modules.items()):
    print(f"loaded pid={pid} module={module} base=0x{base:x} kind={kind}")
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
print(f"stack_dispatch_failed={int(stack_dispatch_failed)} debugger_unhandled={int(debugger_unhandled)}")
PY
exit "$rc"
