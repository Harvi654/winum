#!/usr/bin/env python3
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
import argparse
from collections import Counter, defaultdict, deque
from dataclasses import dataclass
import os
from pathlib import Path
import re


LINE = re.compile(r'^([0-9a-f]+):([0-9a-f]+):(trace|warn|err|fixme):(\w+):(\w+) (.*)$', re.I)
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
SAVE_SCRATCH_SUFFIXES = {'.dwg', '.bak', '.dwl', '.dwl2', '.tmp', '.ac$', '.sv$', '.log', '.txt', '.xml', '.ini'}
SAVE_PUBLIC_NAMES = {'saved.dwg', 'saved.bak', 'saved.dwl', 'saved.dwl2', 'saved.dwg.bak', 'saved.dwg.tmp',
                     'saved.cloudinfo', '.autodesk.support'}
COM_CHANNELS = {'ole', 'combase', 'rpc', 'storage'}
COM_STATUS = re.compile(r'\b(?:hr(?:es(?:ult)?)?|status|result|return(?:ed|ing|s)?|failed(?: with)?)'
                        r'(?: (?:with|error|hr))*\s*[=:,]?\s*(?:0x)?([89ac][0-9a-f]{7})\b', re.I)
RELAY_LINE = re.compile(r'^([a-f0-9]+):([a-f0-9]+):(Call|Ret)\s+([a-z0-9_.]+)\((.*)$', re.I)
RELAY_VALUE = re.compile(r'^\) retval=(?:0x)?([a-f0-9]{1,16})\b', re.I)
RELAY_INFORMATION_CLASS = re.compile(r',\s*(?:0x)?([a-f0-9]{1,16})\)\s+ret=', re.I)
RELAY_RETURN_SITE = re.compile(r'\bret=(?:0x)?([a-f0-9]{1,16})\s*$', re.I)
RENAME_CLASSES = {10: 'FileRenameInformation', 65: 'FileRenameInformationEx'}
RELAY_APIS = {
    'comdlg32.getopenfilenamew', 'comdlg32.getsavefilenamew', 'kernelbase.movefilewithprogressw',
    'kernelbase.getfileattributesexw', 'ntdll.ntqueryfullattributesfile', 'ntdll.ntsetinformationfile',
    'combase.cocreateinstance', 'combase.cogetclassobject',
}
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


def scan_save_io(log, *, com_only=False, relay_only=False):
    processes = set()
    scratch, phase, fault = {}, {}, {}
    rename_requested = set()
    events, target_resolution = Counter(), Counter()
    missing_scratch, missing_paths = Counter(), defaultdict(set)
    com_messages, com_signals = Counter(), Counter()
    com_tail = defaultdict(lambda: deque(maxlen=12))
    relay_events = Counter()
    rename_threads, pending_renames = {}, {}
    rename_returns = Counter()

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
            if relay_only and (relay := RELAY_LINE.match(line)):
                pid, tid, action, name, arguments = relay.groups()
                name = name.lower().replace('.dll.', '.')
                if pid in processes and phase.get(pid) in {'command', 'saving'} and name in RELAY_APIS:
                    value_match = RELAY_VALUE.match(arguments) if action == 'Ret' else None
                    value = f'0x{int(value_match[1], 16) & 0xffffffff:08x}' if value_match else '-'
                    relay_events[pid, tid, pid in rename_requested, name, action, value] += 1
                    if (name == 'ntdll.ntsetinformationfile' and rename_threads.get(pid) == tid and
                            (site := RELAY_RETURN_SITE.search(arguments))):
                        key = pid, tid, int(site[1], 16)
                        if action == 'Call' and (info := RELAY_INFORMATION_CLASS.search(arguments)):
                            if label := RENAME_CLASSES.get(int(info[1], 16) & 0xffffffff):
                                pending_renames[key] = label
                        elif value_match and (label := pending_renames.pop(key, None)):
                            rename_returns[pid, label, value] += 1
                continue
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
            if com_only and channel in COM_CHANNELS and phase.get(pid) in {'command', 'saving'}:
                after = pid in rename_requested
                status = COM_STATUS.search(message)
                code = f'0x{status[1].lower()}' if status else '-'
                event = level, channel, function, code
                com_messages[pid, tid, after] += 1
                if status or level in {'err', 'warn', 'fixme'}:
                    com_signals[pid, tid, after, event] += 1
                if after:
                    com_tail[pid, tid].append(event)
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
            if (level == 'trace' and function == 'MoveFileWithProgressW' and len(paths) >= 2 and
                    scope(paths[1], scratch[pid]) == 'target'):
                rename_requested.add(pid)
                rename_threads[pid] = tid
            if com_only or relay_only:
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
            timing = 'after_request' if pid in rename_requested else 'before_request'
            if function == 'get_nt_and_unix_names' and scopes == ('target',):
                target_resolution[pid, timing, detail] += 1
            if scopes == ('scratch_other',) and detail in {'path_status:0xc0000034', 'lookup_warning:0xc0000034'}:
                path = paths[0]
                name = basename(path)
                if name not in SAVE_PUBLIC_NAMES:
                    suffix = Path(name).suffix
                    name = '*' + suffix if suffix in SAVE_SCRATCH_SUFFIXES else '<redacted>'
                key = pid, tid, phase[pid], timing, function, detail, name
                missing_scratch[key] += 1
                missing_paths[key].add(path)

    if len(phase) != 1:
        label = 'save_relay_window' if relay_only else 'save_com_window' if com_only else 'save_io_window'
        print(f'{label}=unconfirmed')
        return
    pid, window = next(iter(phase.items()))
    fault_tid, code = fault.get(pid, (None, None))
    terminal = f'0x{code}' if code else 'none'
    if relay_only:
        print(f'save_relay_window={window} terminal={terminal} rename_request={"seen" if pid in rename_requested else "missing"}')
        counts, on_fault = Counter(), Counter()
        for (process, tid, after, name, action, value), count in relay_events.items():
            if process == pid:
                key = after, name, action, value
                counts[key] += count
                if tid == fault_tid:
                    on_fault[key] += count
        print(f'save_relay_messages={sum(counts.values())} fault_thread={sum(on_fault.values())}')
        print(f'save_relay_rename_returns={sum(count for (process, _, _), count in rename_returns.items() if process == pid)}')
        for (process, label, value), count in rename_returns.items():
            if process == pid:
                print(f'save_relay_rename={label} retval={value} count={count}')
        order = sorted(counts, key=lambda key: (not on_fault[key], not key[0], key))
        for key in order[:24]:
            after, name, action, value = key
            timing = 'after_request' if after else 'before_request'
            print(f'save_relay_event={action.lower()} phase={timing} api={name} retval={value} '
                  f'count={counts[key]} fault_thread={on_fault[key]}')
        if len(order) > 24:
            print(f'save_relay_omitted={len(order) - 24}')
        return
    if com_only:
        print(f'save_com_window={window} terminal={terminal} rename_request={"seen" if pid in rename_requested else "missing"}')
        before = sum(count for (process, _, after), count in com_messages.items() if process == pid and not after)
        after = sum(count for (process, _, after), count in com_messages.items() if process == pid and after)
        print(f'save_com_messages={after} before_request={before} fault_thread={com_messages[pid, fault_tid, True]}')
        counts, on_fault = Counter(), Counter()
        for (process, tid, is_after, event), count in com_signals.items():
            if process == pid and is_after:
                counts[event] += count
                if tid == fault_tid:
                    on_fault[event] += count
        order = sorted(counts, key=lambda event: (not on_fault[event], event[3] == '-', -counts[event], event))
        print(f'save_com_signals={len(counts)} shown={min(len(counts), 20)}')
        for event in order[:20]:
            level, channel, function, status = event
            print(f'save_com_signal={level} site={channel}:{function} status={status} '
                  f'count={counts[event]} fault_thread={on_fault[event]}')
        for level, channel, function, status in com_tail[pid, fault_tid]:
            print(f'save_com_fault_tail={level} site={channel}:{function} status={status}')
        return
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
    print(f'save_io_rename_request={"seen" if pid in rename_requested else "missing"}')
    for (process, timing, detail), count in target_resolution.items():
        if process == pid:
            print(f'save_io_target_resolution={timing} detail={detail} count={count}')
    for key, count in missing_scratch.items():
        process, tid, stage, timing, function, detail, name = key
        if process == pid and tid == fault_tid:
            print(f'save_io_fault_lookup={stage} rename={timing} api={function} detail={detail} '
                  f'file={name} count={count} distinct_paths={len(missing_paths[key])}')


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
    mode.add_argument('--save-com', action='store_true', help='summarize COM messages from the latest candidate Save As trace')
    mode.add_argument('--save-relay', action='store_true', help='summarize relay calls from the latest candidate Save As trace')
    args = parser.parse_args()
    if args.save_io or args.save_com or args.save_relay:
        label = 'save_relay_window' if args.save_relay else 'save_com_window' if args.save_com else 'save_io_window'
        try:
            logs = args.work.glob('dwg-candidate-save-events.*/acad-*.log')
            log = max(logs, key=lambda path: path.stat().st_mtime_ns, default=None)
            if log is None:
                print(f'{label}=unconfirmed log=missing')
            else:
                scan_save_io(log, com_only=args.save_com, relay_only=args.save_relay)
        except OSError:
            print(f'{label}=unconfirmed log=unreadable')
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
