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

GROWTH_LIMIT = 64 * 1024 * 1024
MSXML_CALL = re.compile(r":trace:msxml:([a-z_]+)\b", re.IGNORECASE)


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


def trace_summary(stream, counts):
    with stream:
        for line in stream:
            text = line.lower()
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


def run(prefix, runtime):
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
    env = dict(os.environ, WINEPREFIX=str(prefix), WINELOADER=str(runtime / "bin/wine"),
               WINESERVER=str(runtime / "bin/wineserver"), WINEDEBUG="-all,acad.exe:+pid,acad.exe:+loaddll,acad.exe:+msxml")

    print(f"profiles_before={len(initial)} largest_before_bytes={maximum}", flush=True)
    with subprocess.Popen([runtime / "bin/wine", exe], env=env, cwd=exe.parent,
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

    current = profiles(prefix)
    maximum = max(maximum, max(current.values(), default=0))
    growth = max(growth, max((size - initial.get(path, 0) for path, size in current.items()), default=0))
    print(f"profiles_after={len(current)} largest_after_bytes={max(current.values(), default=0)} "
          f"peak_bytes={maximum} peak_growth_bytes={growth} acad_exit={status}")
    for name, count in sorted(counts.copy().items()):
        print(f"{name}={count}")
    return 1 if reached_limit else status


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("test_prefix", type=Path)
    parser.add_argument("runtime", type=Path)
    args = parser.parse_args()
    prefix = args.test_prefix.resolve()
    if prefix == (Path.home() / ".wine-autocad2027").resolve():
        parser.error("supply a test copy of .wine-autocad2027")
    try:
        return run(prefix, args.runtime.resolve())
    except (ValueError, OSError, subprocess.SubprocessError) as error:
        print(f"Setup or launch failed ({type(error).__name__})")
        if isinstance(error, ValueError):
            print(error)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
