#!/usr/bin/env python3
# Original AI-assisted (Codex), LGPL-2.1-or-later.
"""Run AutoCAD from a test prefix and report anonymized Profile.aws activity."""
import argparse
from collections import Counter
import os
from pathlib import Path
import re
import subprocess
from threading import Thread
import time
from xml.etree import ElementTree

GROWTH_LIMIT = 64 * 1024 * 1024
MSXML_CALL = re.compile(r":trace:msxml:([a-z_]+)\b", re.IGNORECASE)
LAUNCHER_SETUP = """source "$(dirname "$(readlink -f "$0")")/common-env.sh"
cd "$INVENTOR_DIR"
source scripts/lib/common.sh
"""
TEST_SETUP = r"""source "$(dirname "$(readlink -f "$0")")/common-env.sh" || exit 2
export WINEPREFIX="$WINUM_ACAD_TEST_PREFIX"
export INV_PREFIX="$WINUM_ACAD_TEST_PREFIX"
export INV_WINE=system
export PATH="$WINUM_ACAD_TEST_RUNTIME/bin:$PATH"
export WINE="$WINUM_ACAD_TEST_RUNTIME/bin/wine"
export WINELOADER="$WINE"
export WINESERVER="$WINUM_ACAD_TEST_RUNTIME/bin/wineserver"
export ACAD_LOGDIR="$WINUM_ACAD_TEST_LOGDIR"
export INV_LOGDIR="$ACAD_LOGDIR"
export WINEDEBUG="$WINUM_ACAD_TEST_DEBUG"
cd "$INVENTOR_DIR" || exit 2
source scripts/lib/common.sh || exit 2
if [ "$(readlink -f "$WINEPREFIX")" != "$WINUM_ACAD_TEST_PREFIX" ] ||
   [ "$(readlink -f "$P_C")" != "$WINUM_ACAD_TEST_PREFIX/drive_c" ] ||
   [ "$(type -t wine)" != file ] ||
   [ "$(readlink -f "$(type -P wine)")" != "$(readlink -f "$WINUM_ACAD_TEST_RUNTIME/bin/wine")" ] ||
   [ "$(type -t wineserver)" != file ] ||
   [ "$(readlink -f "$(type -P wineserver)")" != "$(readlink -f "$WINUM_ACAD_TEST_RUNTIME/bin/wineserver")" ] ||
   [ "$(readlink -f "$ACAD_LOGDIR")" != "$WINUM_ACAD_TEST_LOGDIR" ]; then
    echo launcher_test_paths=MISMATCH
    exit 2
fi
echo launcher_test_paths=OK
"""



def profiles(prefix):
    sizes = {}
    for directory in (prefix / "drive_c/users").glob("*/AppData/Roaming/Autodesk/AutoCAD*"):
        if not directory.resolve().is_relative_to(prefix):
            continue
        for profile in directory.rglob("Profile.aws"):
            if not profile.is_symlink():
                try:
                    sizes[profile] = profile.stat().st_size
                except FileNotFoundError:
                    pass
    return sizes


def verify_profiles(prefix):
    found = sorted(profiles(prefix).items())
    print(f"profiles={len(found)}")
    status = 0
    for index, (path, size) in enumerate(found, 1):
        elements = 0
        try:
            for _, element in ElementTree.iterparse(path, events=("end",)):
                elements += element.tag.rsplit("}", 1)[-1] == "AcApData"
                element.clear()
            print(f"profile={index} bytes={size} xml=valid acapdata={elements}")
        except ElementTree.ParseError as error:
            print(f"profile={index} bytes={size} xml=invalid line={error.position[0]} acapdata_seen={elements}")
            status = 1
    return status


def trace_summary(stream, counts):
    with stream:
        for line in stream:
            text = line.lower()
            if text.strip() in ("launcher_test_paths=ok", "launcher_test_paths=mismatch"):
                counts["launcher_test_paths_" + text.strip().split("=", 1)[1]] += 1
            if ":loaddll:" in text and "msxml3.dll" in text:
                dll_kind = text.rsplit(":", 1)[-1].strip()
                if dll_kind in ("builtin", "native"):
                    counts[f"msxml3_load_{dll_kind}"] += 1
            match = MSXML_CALL.search(text)
            if match:
                name = match[1]
                if name in ("domelem_replacechild", "domdoc_save", "domdoc_load"):
                    counts[name] += 1
                if "profile.aws" in text and name in ("domdoc_save", "domdoc_load"):
                    counts[f"profile_aws_{name}"] += 1


def stop_test_server(runtime, env):
    subprocess.run([runtime / "bin/wineserver", "-k"], env=env, check=True,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=10)


def launcher_command(launcher):
    source = launcher.read_text()
    executable = next((line for line in source.splitlines() if line.strip() and not line.lstrip().startswith("#")), "")
    if source.count(LAUNCHER_SETUP) != 1 or executable != LAUNCHER_SETUP.splitlines()[0]:
        raise ValueError("The launcher's initial setup differs from the reviewed AutoCAD launcher")
    # Keep $0 and the working directory as in the original invocation.
    return ["bash", "-c", source.replace(LAUNCHER_SETUP, TEST_SETUP, 1), launcher]


def run(prefix, runtime, launcher, log_dir):
    exe = prefix / "drive_c/Program Files/Autodesk/AutoCAD 2027/acad.exe"
    if not exe.is_file():
        raise ValueError("AutoCAD 2027 executable was not found in the supplied prefix")
    if (prefix / "dosdevices/c:").resolve() != prefix / "drive_c":
        raise ValueError("The supplied prefix's C: drive does not point to its own drive_c")
    if not (runtime / "bin/wine").is_file() or not (runtime / "bin/wineserver").is_file():
        raise ValueError("The supplied runtime is missing wine or wineserver")

    initial = profiles(prefix)
    counts = Counter()
    maximum = max(initial.values(), default=0)
    growth = 0
    reached_limit = False
    env = dict(os.environ, WINEPREFIX=str(prefix), WINE=str(runtime / "bin/wine"),
               WINELOADER=str(runtime / "bin/wine"), WINESERVER=str(runtime / "bin/wineserver"),
               PATH=f"{runtime / 'bin'}{os.pathsep}{os.environ.get('PATH', '')}",
               WINEDEBUG="-all,acad.exe:+pid,acad.exe:+loaddll,acad.exe:+msxml")
    if launcher:
        if not log_dir or not log_dir.is_dir():
            raise ValueError("Supply a fresh --log-dir for the AutoCAD launcher")
        env.update(WINUM_ACAD_TEST_PREFIX=str(prefix), WINUM_ACAD_TEST_RUNTIME=str(runtime),
                   WINUM_ACAD_TEST_LOGDIR=str(log_dir.resolve()), WINUM_ACAD_TEST_DEBUG=env["WINEDEBUG"])
        command = launcher_command(launcher)
        cwd = launcher.parent.parent
    else:
        command = [runtime / "bin/wine", exe]
        cwd = exe.parent

    print(f"profiles_before={len(initial)} largest_before_bytes={maximum}", flush=True)
    with subprocess.Popen(command, env=env, cwd=cwd,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                          text=True, errors="replace") as acad:
        reader = Thread(target=trace_summary, args=(acad.stdout, counts), daemon=True)
        reader.start()
        try:
            while acad.poll() is None:
                current = profiles(prefix)
                maximum = max(maximum, max(current.values(), default=0))
                growth = max(growth, max((size - initial.get(path, 0)
                                         for path, size in current.items()), default=0))
                if growth >= GROWTH_LIMIT:
                    reached_limit = True
                    print("profile_growth_limit=64MiB", flush=True)
                    stop_test_server(runtime, env)
                    break
                time.sleep(0.75)
        except KeyboardInterrupt:
            stop_test_server(runtime, env)
        status = acad.wait()
        reader.join(timeout=2)

    if log_dir and not counts["msxml3_load_builtin"] and not counts["msxml3_load_native"]:
        for log in sorted(log_dir.glob("*.log")):
            if log.is_file() and not log.is_symlink():
                trace_summary(log.open("r", errors="replace"), counts)
    current = profiles(prefix)
    maximum = max(maximum, max(current.values(), default=0))
    growth = max(growth, max((size - initial.get(path, 0) for path, size in current.items()), default=0))
    print(f"profiles_after={len(current)} largest_after_bytes={max(current.values(), default=0)} "
          f"peak_bytes={maximum} peak_growth_bytes={growth} acad_exit={status}")
    if launcher and not counts["msxml3_load_builtin"] and not counts["msxml3_load_native"]:
        print("msxml3_load_trace=missing")
    for name, count in sorted(counts.copy().items()):
        if count:
            print(f"{name}={count}")
    return 1 if reached_limit else status


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("test_prefix", type=Path)
    parser.add_argument("runtime", type=Path, nargs="?")
    parser.add_argument("--profile-only", action="store_true", help="read Profile.aws XML without starting AutoCAD")
    parser.add_argument("--launcher", type=Path, help="start through the project's scripts/launch-autocad.sh")
    parser.add_argument("--log-dir", type=Path, help="fresh launcher log directory to summarize if needed")
    args = parser.parse_args()
    prefix = args.test_prefix.resolve()
    if args.profile_only:
        if not (prefix / "drive_c").is_dir():
            parser.error("no drive_c in the supplied prefix")
        try:
            return verify_profiles(prefix)
        except OSError:
            print("Profile read failed")
            return 2
    if not args.runtime:
        parser.error("a Wine runtime is required for a launch")
    if prefix == (Path.home() / ".wine-autocad2027").resolve():
        parser.error("supply a test copy of .wine-autocad2027")
    try:
        launcher = args.launcher.resolve() if args.launcher else None
        if launcher and not launcher.is_file():
            raise ValueError("The specified launcher was not found")
        return run(prefix, args.runtime.resolve(), launcher, args.log_dir)
    except (ValueError, OSError, subprocess.SubprocessError) as error:
        print(f"Setup or launch failed ({type(error).__name__})")
        if isinstance(error, ValueError):
            print(error)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
