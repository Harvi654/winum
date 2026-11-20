#!/usr/bin/env python3
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
import argparse
from collections import Counter
import os
from pathlib import Path
import re


RUNS = (
    ('baseline-idle', '', 'idle'),
    ('baseline-open', '', 'open-activex'),
    ('baseline-save', '', 'save-activex'),
    ('dotnet-10.0.15-open', 'dotnet-10.0.15', 'open-activex'),
    ('dotnet-10.0.15-save', 'dotnet-10.0.15', 'save-activex'),
)
PREFIX = re.compile(r'^([a-f0-9]+):([a-f0-9]+):(?:trace|warn|err|fixme):(\w+):\w+ (.*)$', re.I)
IMAGE = re.compile(r'Loaded L?"[^"\n]*[\\/]acad\.exe" at ', re.I)
FATAL = re.compile(r'Unhandled exception code (?:0x)?([a-f0-9]{8})\b', re.I)
MODULE_NAME = re.compile(r'[a-z0-9_.+-]{1,120}\.(?:dll|exe|ocx|drv|arx|dbx|crx|hdi|so)', re.I)
LOAD = re.compile(r'Failed to load module (.+?); status=(?:0x)?([a-f0-9]{8})\b', re.I)
IMPORT = re.compile(r'(?:Loading library|Library) (.+?) \(which is needed by (.+?)\) '
                    r'(?:not found|failed \(error (?:0x)?([a-f0-9]{8})\))', re.I)
EXPORT = re.compile(r'(?:"([a-z0-9_?@$]+)"|\(null\)) \(ordinal ([0-9]+)\) not found in (.+)$', re.I)
STUB = re.compile(r'No implementation for (\S+)\.([a-z0-9_?@$]+)'
                  r'(?: imported from (.+?), (?:setting to|allocating stub) [a-f0-9x]+)?', re.I)
INIT = re.compile(r'Initialization of (.+?) failed\s*$', re.I)
PRIVATE_NAMES = {value.lower() for value in (os.getenv('USER'), os.getenv('LOGNAME'), Path.home().name) if value}


def public_token(value):
    if any(name in value.lower() for name in PRIVATE_NAMES):
        return '<redacted>'
    return value


def module_name(path):
    name = re.split(r'[\\/]', path.strip().rstrip('.').removeprefix('L"').strip('"'))[-1].lower()
    return public_token(name) if MODULE_NAME.fullmatch(name) else '<unrecognized>'


def loader_failure(message):
    if match := LOAD.fullmatch(message):
        return f'load module={module_name(match[1])} status=0x{match[2].lower()}'
    if match := IMPORT.fullmatch(message.rstrip('.')):
        status = match[3].lower() if match[3] else 'c0000135'
        return f'import module={module_name(match[1])} importer={module_name(match[2])} status=0x{status}'
    if match := EXPORT.fullmatch(message):
        symbol = public_token(match[1][:96]) if match[1] else match[2]
        return f'export module={module_name(match[3])} symbol={symbol}'
    if match := STUB.fullmatch(message):
        return f'stub module={module_name(match[1])} symbol={public_token(match[2][:96])}'
    if match := INIT.fullmatch(message):
        return f'init module={module_name(match[1])}'
    return None


def scan(log):
    pids, fatal, counts = set(), {}, Counter()
    with log.open(encoding='utf-8', errors='replace') as stream:
        for line in stream:
            match = PREFIX.match(line.rstrip('\r\n'))
            if not match:
                continue
            pid, tid, channel, message = match.groups()
            if channel == 'loaddll' and IMAGE.match(message):
                pids.add(pid)
            elif channel == 'seh' and (match := FATAL.search(message)):
                fatal.setdefault(pid, (tid, match[1].lower()))
            elif channel == 'module' and pid not in fatal and (failure := loader_failure(message)):
                counts[pid, tid, failure] += 1

    events, on_fault_thread = Counter(), Counter()
    for (pid, tid, failure), count in counts.items():
        if pid in pids:
            events[failure] += count
            if pid in fatal and tid == fatal[pid][0]:
                on_fault_thread[failure] += count
    codes = sorted({fatal[pid][1] for pid in pids if pid in fatal})
    return len(pids), codes, events, on_fault_thread


def main():
    parser = argparse.ArgumentParser(description='Summarize saved AutoCAD Wine loader traces without relaunching.')
    parser.add_argument('work', type=Path, help='original AutoCAD test directory')
    args = parser.parse_args()
    idle = None
    for label, subdir, trial in RUNS:
        try:
            logs = (args.work / subdir).glob(f'dwg-baseline-{trial}.*/acad-*.log')
            log = max(logs, key=lambda path: path.stat().st_mtime_ns, default=None)
            if log is None:
                print(f'loader_run={label} log=missing')
                continue
            processes, codes, events, on_fault_thread = scan(log)
        except OSError:
            print(f'loader_run={label} log=unreadable')
            continue
        if trial == 'idle' and processes:
            idle = events
        code_list = ','.join(f'0x{code}' for code in codes) or 'none'
        print(f'loader_run={label} processes={processes} fatal={code_list} '
              f'messages={sum(events.values())} distinct={len(events)}')
        order = sorted(events, key=lambda item: (not on_fault_thread[item], idle is not None and item in idle,
                                                 -events[item], item))
        for item in order[:8]:
            idle_seen = 'unknown' if idle is None else str(int(item in idle))
            print(f'loader_event={item} count={events[item]} fault_thread={on_fault_thread[item]} idle_seen={idle_seen}')
        if len(order) > 8:
            print(f'loader_omitted={len(order) - 8}')


if __name__ == '__main__':
    main()
