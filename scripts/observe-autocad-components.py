#!/usr/bin/env python3
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
"""Sample the test prefix while the existing licensing trace runs."""

from collections import defaultdict
from datetime import datetime
from functools import partial
from pathlib import Path
from threading import Thread
import os
import re
import subprocess
import sys
import time

IMAGES = re.compile(
    r"^(?:acad|services|rpcss|lsass|svchost|adskidentitymanager|"
    r"adsklicensing(?:service|agent|insthelper)|autodeskgenuine[\w-]*|"
    r"adskgenuine[\w-]*|agsservice|genuine[\w-]*|adskaccess[\w-]*|adodis[\w-]*|adappmgr[\w-]*|"
    r"fnplicensingservice(?:64)?|lmgrd|msedgewebview2|qtwebengineprocess|adp[\w-]*|cer[\w-]*|upi)\.exe\Z",
    re.I,
)
SERVICE_SECTION = re.compile(
    r"^\[System\\\\(ControlSet\d+|CurrentControlSet)\\\\Services\\\\([^\\\]]+)\]",
    re.I,
)
SERVICE_NAMES = re.compile(r"autodesk|adsk|genuine|adodis|flexnet|fnplicens|agsservice", re.I)
SERVICE_START = re.compile(r'^"Start"=dword:([0-9a-f]+)', re.I)


def list_components(prefix):
    expected = b"WINEPREFIX=" + os.fsencode(prefix)
    seen = set()
    for proc in Path("/proc").iterdir():
        if not proc.name.isdecimal():
            continue
        try:
            args = (proc / "cmdline").read_bytes().split(b"\0", 2)[:2]
            image = next((name.lower() for arg in args
                          if IMAGES.fullmatch(name := re.split(r"[\\/]", os.fsdecode(arg))[-1])), None)
            if image and expected in (proc / "environ").read_bytes().split(b"\0"):
                seen.add((image, int(proc.name)))
        except OSError:
            continue
    return seen


def configured_services(prefix):
    system = prefix / "system.reg"
    if not system.is_file():
        return []
    results = []
    service = None
    with system.open(errors="replace") as stream:
        for line in stream:
            if line.startswith("["):
                section = SERVICE_SECTION.match(line)
                service = section.groups() if section and SERVICE_NAMES.search(section[2]) else None
            elif service and (start := SERVICE_START.match(line)):
                results.append((*service, int(start[1], 16)))
                service = None
    return sorted(set(results))


def report(samples, prefix, marker, capture_end, stream):
    emit = partial(print, file=stream)
    start = samples[0][0]
    sightings = defaultdict(list)
    for stamp, entries in samples:
        for entry in entries:
            sightings[entry].append(stamp)
    emit("SCM service configuration (registry Start values)")
    for control_set, name, start_type in configured_services(prefix):
        emit(f"control_set={control_set} service={name} start={start_type}")
    emit(f"\nLinux process samples={len(samples)} sample_interval_seconds=1")
    by_image = defaultdict(list)
    for (image, pid), stamps in sightings.items():
        by_image[image].append((pid, stamps[0], stamps[-1]))
    for image, instances in sorted(by_image.items()):
        first = min(item[1] for item in instances)
        last = max(item[2] for item in instances)
        emit(f"image={image} instances={len(instances)} first={first-start:+.1f}s last={last-start:+.1f}s")
    if marker is None:
        return
    emit("\nProcess presence near the marker (relative seconds and Linux PID)")
    window = [sample for sample in samples if sample[0] >= marker - 20 and sample[0] <= capture_end]
    previous = None
    for stamp, entries in window:
        if previous is None:
            emit(f"{stamp-marker:+.1f}s present=" +
                 (",".join(f"{image}:{pid}" for image, pid in sorted(entries)) or "none"))
        else:
            for image, pid in sorted(entries - previous):
                emit(f"{stamp-marker:+.1f}s appeared={image}:{pid}")
            for image, pid in sorted(previous - entries):
                emit(f"{stamp-marker:+.1f}s disappeared={image}:{pid}")
        previous = entries


def main():
    if len(sys.argv) != 3:
        sys.exit("Usage: observe-autocad-components.py TEST_DIRECTORY TRACE_SCRIPT")
    work = Path(sys.argv[1]).resolve(strict=True)
    prefix = (work / "candidate-prefix").resolve(strict=True)
    trace = Path(sys.argv[2]).resolve(strict=True)
    output_dirs = []
    samples = [(time.time(), list_components(prefix))]
    proc = subprocess.Popen(["bash", str(trace), str(work)], stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True, errors="replace")

    def relay_output():
        for line in proc.stdout:
            print(line, end="", flush=True)
            if line.startswith("Output directory: "):
                directory = Path(line.removeprefix("Output directory: ").strip())
                if directory.parent == work and directory.name.startswith("licensing-service-trace."):
                    output_dirs.append(directory)

    reader = Thread(target=relay_output, daemon=True)
    reader.start()
    while proc.poll() is None:
        samples.append((time.time(), list_components(prefix)))
        time.sleep(1)
    reader.join(timeout=3)
    if output_dirs:
        run = output_dirs[-1]
        marker_file = run / "marker-time.txt"
        marker = datetime.fromisoformat(marker_file.read_text().strip()).timestamp() if marker_file.exists() else None
        end_file = run / "end-lines.txt"
        capture_end = end_file.stat().st_mtime if end_file.exists() else samples[-1][0]
        with (run / "component-summary.txt").open("w", encoding="utf-8") as stream:
            report(samples, prefix, marker, capture_end, stream)
        print(f"Component summary: {run / 'component-summary.txt'}")
    return proc.returncode


if __name__ == "__main__":
    sys.exit(main())
