#!/usr/bin/env python3
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
import argparse
from collections import Counter, defaultdict
import os
from pathlib import Path
import re


LINE = re.compile(r'^([0-9a-f]+):([0-9a-f]+):(trace|warn|err):(\w+):(\w+) (.*)$', re.I)
MAPPED = re.compile(r'mapping PE file L?"([^"\n]+)" at (?:0x)?([a-f0-9]+)-(?:0x)?([a-f0-9]+)$', re.I)
LOADED = re.compile(r'Loaded L?"([^"\n]+)" at (?:0x)?([a-f0-9]+):', re.I)
UNLOADED = re.compile(r'\b(?:unloading|unloaded module) L?"([^"\n]+)"', re.I)
MODULE = re.compile(r'[a-z0-9_.+-]{1,96}\.(?:dll|exe|ocx|drv|arx|dbx|crx|hdi)', re.I)
PRIVATE_NAMES = {value.lower() for value in (os.getenv('USER'), os.getenv('LOGNAME'), Path.home().name) if value}
REQUEST = re.compile(r'\bname=L"([^"\r\n]+)".*\bdisp=(\d+) options=([0-9a-f]+)\b', re.I)
WARNING = re.compile(r'L"([^"\r\n]+)" not found \(([0-9a-f]{8})\)$', re.I)
AV = re.compile(r'^(?:code=|Unhandled exception code )(?:0x)?c0000005\b', re.I)
AV_ADDRESS = re.compile(r'\baddr(?:=| )(?:0x)?([a-f0-9]+)(?: \(([^()]+)\+(?:0x)?([a-f0-9]+)\))?', re.I)
FATAL = re.compile(r'^Unhandled exception code (?:0x)?([0-9a-f]{8})\b', re.I)
MARKER = re.compile(r'/winum-dwg\.[a-z0-9]+/open-events\.txt\.(dbx|activex)_open_(enter|return)$')
DISPOSITION = ('supersede', 'open', 'create', 'open_if', 'overwrite', 'overwrite_if')


def path_group(path):
    if '/winum-dwg.' in path:
        return 'scratch_lock' if path.endswith(('.dwl', '.dwl2')) else 'scratch'
    if '/appdata/local/temp/' in path or '/windows/temp/' in path or '/tmp/' in path:
        return 'temp'
    if '/program files/autodesk/' in path or '/common files/autodesk shared/' in path:
        return 'autodesk'
    if '/program files/dotnet/' in path:
        return 'dotnet'
    if '/windows/system32/' in path or '/windows/syswow64/' in path:
        return 'system'
    if '/appdata/' in path:
        return 'appdata'
    return 'other'


def basename(path):
    return re.split(r'[\\/]', path.lower())[-1]


def public_module(name):
    if not MODULE.fullmatch(name) or any(private in name for private in PRIVATE_NAMES):
        return '<redacted>'
    return name


def exception_site(message, images):
    match = AV_ADDRESS.search(message)
    if not match:
        return 'unresolved', '-'
    address, name, offset = match.groups()
    if name:
        return public_module(basename(name)), f'0x{int(offset, 16):x}'
    address = int(address, 16)
    candidates = [(name, address - base) for name, base, end in images.values() if base <= address < end]
    if len(candidates) == 1:
        name, offset = candidates[0]
        return public_module(name), f'0x{offset:x}'
    return 'unresolved', '-'


def scan(log, probe):
    processes = set()
    entered, returned = Counter(), Counter()
    active, phase, fault = {}, {}, {}
    requests, lookups, paths = {}, Counter(), defaultdict(set)
    exceptions = Counter()
    sites = Counter()
    mappings, images = {}, defaultdict(dict)
    with log.open(encoding='utf-8', errors='replace') as stream:
        for line in stream:
            match = LINE.match(line.rstrip('\r\n'))
            if not match:
                continue
            pid, tid, level, channel, function, message = match.groups()
            if channel == 'module' and function == 'map_image_into_view' and (match := MAPPED.fullmatch(message)):
                path, base, end = match.groups()
                mappings[pid, int(base, 16)] = (basename(path), int(end, 16))
            elif channel == 'loaddll' and (match := LOADED.match(message)):
                path, base = match.groups()
                name, base = basename(path), int(base, 16)
                if name == 'acad.exe':
                    processes.add(pid)
                mapped_name, end = mappings.pop((pid, base), (None, base))
                if name == mapped_name:
                    images[pid][path.lower()] = (name, base, end)
            elif channel in ('module', 'loaddll') and function == 'free_modref' and (match := UNLOADED.search(message)):
                images[pid].pop(match[1].lower(), None)
            if pid not in processes or pid in fault:
                continue
            if channel == 'seh':
                if AV.match(message):
                    stage, same_thread = phase.get(pid, 'before'), tid == active.get(pid)
                    exceptions[stage, same_thread] += 1
                    module, offset = exception_site(message, images[pid])
                    sites[pid, tid, stage, same_thread, module, offset] += 1
                if match := FATAL.match(message):
                    fault[pid] = (match[1].lower(), tid)
            if channel != 'file' or function != 'NtCreateFile':
                continue
            if level == 'trace' and (match := REQUEST.search(message)):
                raw_path, disposition, options = match.groups()
                path = re.sub(r'[\\/]+', '/', raw_path).lower()
                requests[pid, tid] = (path, int(disposition), int(options, 16))
                if (marker := MARKER.search(path)) and marker[1] == probe:
                    if marker[2] == 'enter':
                        entered[pid] += 1
                        active[pid] = tid
                        phase[pid] = 'call'
                    elif pid in active and phase[pid] == 'call':
                        returned[pid] += 1
                        phase[pid] = 'after_return'
            elif level == 'warn' and phase.get(pid) == 'call' and (match := WARNING.fullmatch(message)):
                path = re.sub(r'[\\/]+', '/', match[1]).lower()
                request = requests.pop((pid, tid), None)
                disposition, kind = 'unknown', 'unknown'
                if request and request[0] == path:
                    _, mode, options = request
                    if mode < len(DISPOSITION):
                        disposition = DISPOSITION[mode]
                    kind = 'directory' if options & 1 else 'file' if options & 0x40 else 'unspecified'
                event = (match[2].lower(), path_group(path), kind, disposition)
                lookups[pid, tid, tid == active[pid], event] += 1
                paths[event].add(path)

    codes = ','.join(sorted({f'0x{code}' for code, _ in fault.values()})) or 'none'
    print(f'file_detail={probe} started={sum(entered.values())} returned={sum(returned.values())} fatal={codes}')
    counts, on_call, on_fault = Counter(), Counter(), Counter()
    for (pid, tid, same_thread, event), count in lookups.items():
        counts[event] += count
        if same_thread:
            on_call[event] += count
        if pid in fault and tid == fault[pid][1]:
            on_fault[event] += count
    for (status, scope, kind, disposition), count in counts.most_common(12):
        event = status, scope, kind, disposition
        print(f'file_request=status:0x{status} scope={scope} kind={kind} disposition={disposition} '
              f'count={count} paths={len(paths[event])} call_thread={on_call[event]} fault_thread={on_fault[event]}')
    if len(counts) > 12:
        print(f'file_request_omitted={len(counts) - 12}')
    for stage in ('before', 'call', 'after_return'):
        print(f'access_violation_phase={stage} messages={exceptions[stage, False] + exceptions[stage, True]} '
              f'call_thread={exceptions[stage, True]}')
    site_counts, site_call, site_fault = Counter(), Counter(), Counter()
    for (pid, tid, stage, same_thread, module, offset), count in sites.items():
        site = stage, module, offset
        site_counts[site] += count
        if same_thread:
            site_call[site] += count
        if pid in fault and tid == fault[pid][1]:
            site_fault[site] += count
    order = sorted(site_counts, key=lambda site: (site[0] == 'before', not site_call[site], -site_counts[site], site))
    for stage, module, offset in order[:12]:
        site = stage, module, offset
        print(f'access_violation_site={stage} module={module} offset={offset} messages={site_counts[site]} '
              f'call_thread={site_call[site]} fault_thread={site_fault[site]}')
    if len(order) > 12:
        print(f'access_violation_site_omitted={len(order) - 12}')


def main():
    parser = argparse.ArgumentParser(description='Classify saved AutoCAD file lookups and access violation messages.')
    parser.add_argument('work', type=Path, help='original AutoCAD test directory')
    args = parser.parse_args()
    for probe, trial in (('dbx', 'open-dbx'), ('activex', 'open-activex')):
        logs = args.work.glob(f'dwg-baseline-{trial}.*/acad-*.log')
        try:
            log = max(logs, key=lambda path: path.stat().st_mtime_ns, default=None)
            if log is None:
                print(f'file_detail={probe} log=missing')
                continue
            scan(log, probe)
        except OSError:
            print(f'file_detail={probe} log=unreadable')


if __name__ == '__main__':
    main()
