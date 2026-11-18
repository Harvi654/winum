#!/usr/bin/env python3
# Original AI-assisted (Codex), LGPL-2.1-or-later.
"""Compare AF_UNIX CI artifacts for missing or newly failing Wine tests."""
import argparse
from collections import Counter
from pathlib import Path
import re

SUMMARY = re.compile(
    r"^(?:[0-9a-f]+:)?(\w+):(?:\d+\.\d+)? (\d+) tests executed "
    r"\(\d+ marked as todo, \d+ as flaky, (\d+) failures?\), (\d+) skipped\."
)
FAILURE = re.compile(
    r"^(?:[0-9a-f]+:)?([\w.-]+\.c:\d+):(?:\d+\.\d+)? "
    r"(Test failed|Test succeeded inside todo block):"
)


def read_unit(path, unit):
    summary = None
    exit_code = 0
    failures = Counter()
    for line in path.read_text(errors="replace").splitlines():
        match = SUMMARY.match(line)
        if match and match[1] == unit:
            summary = tuple(map(int, match.groups()[1:]))
        match = FAILURE.match(line)
        if match:
            failures[match[1], match[2]] += 1
        if line.startswith("exit="):
            exit_code = int(line[5:])
    if summary is None:
        raise ValueError(f"{path}: no Wine test summary (exit {exit_code})")
    checks, failed, skipped = summary
    if failed != sum(failures.values()) or exit_code != min(failed, 255):
        raise ValueError(f"{path}: Wine reported {failed} failures, "
                         f"captured {sum(failures.values())}, exit {exit_code}")
    return checks, skipped, failures


def read_variant(directory):
    required = ("ws2_32.sock", "ws2_32.afd", "ws2_32.protocol",
                "ntdll.file", "kernel32.file", "advapi32.security")
    results = {}
    for log in sorted((directory / "logs").glob("*.log")):
        dll, arch, unit = log.stem.split(".", 2)
        results[dll, arch, unit] = read_unit(log, unit)
    for arch in ("x86_64", "i386"):
        for name in required:
            dll, unit = name.split(".")
            if (dll, arch, unit) not in results:
                raise ValueError(f"{directory}: missing {dll}.{arch}.{unit}")
    return results


def compare(before, after):
    old, new = read_variant(before), read_variant(after)
    if old.keys() != new.keys():
        diff = old.keys() ^ new.keys()
        raise ValueError("units differ: " + ", ".join(".".join(key) for key in sorted(diff)))
    new_failures = 0
    for key in sorted(old):
        name = ".".join(key)
        old_checks, old_skips, old_failures = old[key]
        new_checks, new_skips, failures = new[key]
        for (location, kind), added in sorted((failures - old_failures).items()):
            print(f"{name} {location}: {kind} (+{added})")
            new_failures += added
        if (old_checks, old_skips) != (new_checks, new_skips):
            print(f"{name}: {old_checks} -> {new_checks} checks, {old_skips} -> {new_skips} skips")
    print(f"{before.name} -> {after.name}: {len(new)} units, {new_failures} new failures")
    return new_failures != 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline", type=Path)
    parser.add_argument("candidate", type=Path)
    args = parser.parse_args()
    try:
        return compare(args.baseline, args.candidate)
    except ValueError as error:
        parser.exit(2, f"{error}\n")


if __name__ == "__main__":
    raise SystemExit(main())
