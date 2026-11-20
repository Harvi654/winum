#!/usr/bin/env python3
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
import argparse
from collections import Counter, defaultdict, deque
from dataclasses import dataclass
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
EXCEPTION_ADDRESS = re.compile(r'\baddr(?:=| )(?:0x)?([a-f0-9]+)(?: \(([^()]+)\+(?:0x)?([a-f0-9]+)\))?', re.I)
EXCEPTION_HEADER = re.compile(r'^code=(?:0x)?([0-9a-f]{8})(?: \([^)]*\))? flags=(?:0x)?([0-9a-f]+) addr=', re.I)
EXCEPTION_ARG0 = re.compile(r'^\s*info\[0\]=(?:0x)?([0-9a-f]{1,16})\s*$', re.I)
FATAL = re.compile(r'^Unhandled exception code (?:0x)?([0-9a-f]{8})\b', re.I)
MARKER = re.compile(r'/winum-dwg\.[a-z0-9]+/open-events\.txt\.(dbx|activex)_open_(enter|return)$')
SAVE_MARKER = re.compile(r'(/winum-dwg\.[a-z0-9]+/)open-events\.txt\.(save_command_enter|save_begin|save_command_exit)$')
FILE_PATH = re.compile(r'L"([^"\r\n]+)"')
PATH_RESOLUTION = re.compile(r'^L"[^"\r\n]+" -> ret ([a-f0-9]{1,8})\b', re.I)
SAVE_PATH_FUNCTIONS = {'MoveFileWithProgressW', 'ReplaceFileW', 'DeleteFileW'}
SAVE_LOOKUP_WARNINGS = {'NtCreateFile', 'NtQueryAttributesFile', 'NtQueryFullAttributesFile'}
DISPOSITION = ('supersede', 'open', 'create', 'open_if', 'overwrite', 'overwrite_if')
CHAIN_RUNS = (
    ('baseline-open', '', 'open-activex'),
    ('baseline-save', '', 'save-activex'),
    ('dotnet-10.0.15-open', 'dotnet-10.0.15', 'open-activex'),
    ('dotnet-10.0.15-save', 'dotnet-10.0.15', 'save-activex'),
)


@dataclass
class Dispatch:
    code: str
    flags: str
    module: str
    offset: str
    function: str
    arg0: str = '-'


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


def scan_save_io(log):
    processes = set()
    scratch, phase, fault = {}, {}, {}
    events = Counter()

    def scope(path, folder):
        if folder in path:
            name = path.rsplit('/', 1)[-1]
            if name.startswith('open-events.txt'):
                return None
            if name == 'saved.dwg':
                return 'target'
            return 'scratch_lock' if name.endswith(('.dwl', '.dwl2')) else 'scratch_other'
        return 'temp' if path_group(path) == 'temp' else None

    with log.open(encoding='utf-8', errors='replace') as stream:
        for line in stream:
            if not (match := LINE.match(line.rstrip('\r\n'))):
                continue
            pid, tid, level, channel, function, message = match.groups()
            if channel == 'loaddll' and (loaded := LOADED.match(message)) and basename(loaded[1]) == 'acad.exe':
                processes.add(pid)
            if pid not in processes or phase.get(pid) in {'ended', 'fault'}:
                continue
            if channel == 'seh' and pid in phase and (fatal := FATAL.match(message)):
                fault[pid] = (tid, fatal[1].lower())
                phase[pid] = 'fault'
            if channel != 'file':
                continue
            paths = [re.sub(r'[\\/]+', '/', path).lower() for path in FILE_PATH.findall(message)]
            if not paths:
                continue
            if level == 'trace' and function == 'NtCreateFile' and (marker := SAVE_MARKER.search(paths[0])):
                folder, marker_name = marker.groups()
                if marker_name == 'save_command_enter':
                    scratch.setdefault(pid, folder)
                    phase.setdefault(pid, 'command')
                elif scratch.get(pid) == folder and phase.get(pid) in {'command', 'saving'}:
                    phase[pid] = 'saving' if marker_name == 'save_begin' else 'ended'
                continue
            if phase.get(pid) not in {'command', 'saving'}:
                continue
            scopes = tuple(dict.fromkeys(group for path in paths if (group := scope(path, scratch[pid]))))
            if not scopes:
                continue
            if level == 'trace' and function == 'NtCreateFile' and (request := REQUEST.search(message)):
                mode = int(request[2])
                detail = f'disposition:{DISPOSITION[mode]}' if mode < len(DISPOSITION) else 'disposition:unknown'
                if int(request[3], 16) & 0x1000:  # FILE_DELETE_ON_CLOSE
                    detail += ',delete_on_close'
            elif level == 'warn' and function in SAVE_LOOKUP_WARNINGS and (warning := WARNING.fullmatch(message)):
                detail = f'lookup_warning:0x{warning[2].lower()}'
            elif level == 'trace' and function == 'get_nt_and_unix_names' and (resolved := PATH_RESOLUTION.match(message)):
                detail = f'path_status:0x{int(resolved[1], 16):08x}'
            elif level == 'trace' and function in SAVE_PATH_FUNCTIONS:
                detail = 'request'
            else:
                continue
            events[pid, phase[pid], function, '+'.join(scopes), detail, tid] += 1

    if len(phase) != 1:
        print('save_io_window=unconfirmed')
        return
    pid, window = next(iter(phase.items()))
    fault_tid, code = fault.get(pid, (None, None))
    terminal = f'0x{code}' if code else 'none'
    print(f'save_io_window={window} terminal={terminal}')
    counts, on_fault = Counter(), Counter()
    for (process, stage, function, scopes, detail, tid), count in events.items():
        if process == pid:
            key = stage, function, scopes, detail
            counts[key] += count
            if tid == fault_tid:
                on_fault[key] += count
    for key, count in counts.items():
        stage, function, scopes, detail = key
        print(f'save_io={stage} api={function} scope={scopes} detail={detail} count={count} fault_thread={on_fault[key]}')


def public_module(name):
    if not MODULE.fullmatch(name) or any(private in name for private in PRIVATE_NAMES):
        return '<redacted>'
    return name


def exception_site(message, images):
    match = EXCEPTION_ADDRESS.search(message)
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


def keep_dispatch(message, function, images, entries):
    if match := EXCEPTION_HEADER.match(message):
        code, flags = match.groups()
        module, offset = exception_site(message, images)
        entries.append(Dispatch(code.lower(), flags.lower(), module, offset, function))
        return True
    if entries and entries[-1].function == function and (match := EXCEPTION_ARG0.fullmatch(message)):
        event = entries[-1]
        value = int(match[1], 16)
        if event.code == 'e0434352' and value >> 32 in (0, 0xffffffff) and value & 0x80000000:
            event.arg0 = f'0x{value & 0xffffffff:08x}'
        elif event.code == 'c0000409' and value <= 0xffff:
            event.arg0 = f'0x{value:x}'
    return False


def scan(log, probe, fault_only=False):
    processes = set()
    entered, returned = Counter(), Counter()
    active, phase, fault = {}, {}, {}
    requests, lookups, paths = {}, Counter(), defaultdict(set)
    exceptions = Counter()
    sites = Counter()
    mappings, images = {}, defaultdict(dict)
    chain = defaultdict(lambda: deque(maxlen=8))
    dispatches = Counter()
    termination = {}
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
                if fault_only and keep_dispatch(message, function, images[pid], chain[pid, tid]):
                    dispatches[pid, tid] += 1
                if AV.match(message):
                    stage, same_thread = phase.get(pid, 'before'), tid == active.get(pid)
                    exceptions[stage, same_thread] += 1
                    module, offset = exception_site(message, images[pid])
                    sites[pid, tid, stage, same_thread, module, offset] += 1
                if match := FATAL.match(message):
                    fault[pid] = (match[1].lower(), tid)
                    if fault_only:
                        termination[pid] = exception_site(message, images[pid])
            if fault_only or channel != 'file' or function != 'NtCreateFile':
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

    if fault_only:
        if not fault:
            print(f'fault_chain={probe} terminal=missing')
        for pid, (code, tid) in sorted(fault.items()):
            module, offset = termination[pid]
            entries = chain[pid, tid]
            print(f'fault_chain={probe} terminal=0x{code} module={module} offset={offset} '
                  f'dispatches={dispatches[pid, tid]} shown={len(entries)}')
            for event in entries:
                print(f'fault_dispatch=0x{event.code} flags=0x{event.flags} module={event.module} '
                      f'offset={event.offset} arg0={event.arg0}')
        return

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
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--fault-chain', action='store_true', help='show exception dispatches only on the fatal thread')
    mode.add_argument('--save-io', action='store_true', help='summarize file requests from the latest candidate Save As trace')
    args = parser.parse_args()
    if args.save_io:
        try:
            logs = args.work.glob('dwg-candidate-save-events.*/acad-*.log')
            log = max(logs, key=lambda path: path.stat().st_mtime_ns, default=None)
            if log is None:
                print('save_io_window=unconfirmed log=missing')
            else:
                scan_save_io(log)
        except OSError:
            print('save_io_window=unconfirmed log=unreadable')
        return
    runs = CHAIN_RUNS if args.fault_chain else (('dbx', '', 'open-dbx'), ('activex', '', 'open-activex'))
    label = 'fault_chain' if args.fault_chain else 'file_detail'
    for probe, directory, trial in runs:
        logs = (args.work / directory).glob(f'dwg-baseline-{trial}.*/acad-*.log')
        try:
            log = max(logs, key=lambda path: path.stat().st_mtime_ns, default=None)
            if log is None:
                print(f'{label}={probe} log=missing')
                continue
            scan(log, probe, args.fault_chain)
        except OSError:
            print(f'{label}={probe} log=unreadable')


if __name__ == '__main__':
    main()
