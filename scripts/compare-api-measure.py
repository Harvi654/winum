#!/usr/bin/env python3
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
import argparse
from collections import Counter
from pathlib import Path
import re


FIELD = re.compile(r'[a-z_]+=[a-zA-Z0-9_.-]+')
SPEC = re.compile(r'(?:@|\d+)\s+(\w+)\s+((?:-\S+\s+)*)([^\s(]+)(?:\([^)]*\))?(?:\s+(\S+))?')


def load_measure(path, runtime):
    lines = path.read_text(encoding='ascii').splitlines()
    if not lines or lines[-1] != 'complete=1':
        raise ValueError(f'{runtime}: incomplete probe or failed setup')
    reports = {}
    for number, line in enumerate(lines[:-1], 1):
        fields = line.split()
        if not fields or not all(FIELD.fullmatch(field) for field in fields):
            raise ValueError(f'{runtime}: invalid record on line {number}')
        values = dict(field.split('=', 1) for field in fields)
        if len(values) != len(fields):
            raise ValueError(f'{runtime}: duplicate field on line {number}')
        if number == 1:
            header = values
            expected_runtime = 'windows' if runtime == 'Windows' else 'wine'
            if (header.keys() != {'probe', 'arch', 'runtime', 'reported_os'} or
                    header['runtime'] != expected_runtime or header['arch'] not in {'x86_64', 'i686'}):
                raise ValueError(f'{runtime}: unexpected probe header')
            continue
        if values.keys() == {'export', 'present'} and values['present'] in {'0', '1'}:
            record = 'export/' + values.pop('export')
        elif 'case' in values and 'step' in values and (('hr' in values) != ('skipped' in values)):
            record = values.pop('case') + '/' + values.pop('step')
        else:
            raise ValueError(f'{runtime}: unknown record on line {number}')
        if record in reports:
            raise ValueError(f'{runtime}: duplicate record on line {number}')
        reports[record] = values
    if len(lines) < 3:
        raise ValueError(f'{runtime}: missing probe results')
    return header, reports


def spec_notes(path, arch):
    if path is None:
        return {}
    aliases = {'x86_64', 'win64'} if arch == 'x86_64' else {'i386', 'i686', 'win32'}
    notes = {}
    for line in path.read_text(encoding='utf-8').splitlines():
        match = SPEC.fullmatch(re.split(r'\s+#', line.strip(), maxsplit=1)[0])
        if not match:
            continue
        kind, flags, name, target = match.groups()
        if any(flag.startswith('-arch=') and not aliases.intersection(flag[6:].split(','))
               for flag in flags.split()):
            continue
        if kind == 'stub':
            notes[name] = 'declared_stub'
        elif target and '.' in target:
            notes[name] = 'forwarded=' + target
    return notes


def describe(values):
    return 'not_run' if values is None else ' '.join(f'{key}={value}' for key, value in values.items())


def compare_record(reference, vanilla, winum):
    if any(result is None or 'skipped' in result for result in (reference, vanilla, winum)):
        return 'incomplete'
    if vanilla == reference == winum:
        return 'match'
    if winum == reference:
        return 'fixed_in_winum'
    if vanilla == reference:
        return 'winum_only_difference'
    return 'shared_difference' if vanilla == winum else 'different_on_both'


def compare(args):
    windows, vanilla, winum = (load_measure(path, label) for path, label in
                               zip((args.windows, args.vanilla, args.winum), ('Windows', 'vanilla', 'Winum')))
    probe, arch = windows[0]['probe'], windows[0]['arch']
    if any((header['probe'], header['arch']) != (probe, arch) for header, _ in (vanilla, winum)):
        raise ValueError('use the same probe and architecture in all three runs')
    wine_notes = spec_notes(args.wine_spec, arch)
    winum_notes = spec_notes(args.winum_spec, arch)
    print(f'probe={probe} arch={arch} windows_build={windows[0]["reported_os"]}')
    native, stock, patched = (data for _, data in (windows, vanilla, winum))
    differences = Counter()
    measured = 0
    for key in sorted(native.keys() | stock.keys() | patched.keys()):
        reference, a, b = native.get(key), stock.get(key), patched.get(key)
        outcome = compare_record(reference, a, b)
        differences[outcome] += 1
        if key.startswith('export/'):
            name = key.split('/', 1)[1]
            if any(result is None for result in (reference, a, b)):
                raise ValueError('export lists differ across runs')
            windows_present, stock_present, patched_present = (result['present'] for result in (reference, a, b))
            wine_annotation = wine_notes.get(name, '-')
            winum_annotation = winum_notes.get(name, '-')
            if outcome == 'match' and wine_annotation == winum_annotation == '-':
                continue
            print(f'export={name} windows={windows_present} vanilla={stock_present} winum={patched_present} '
                  f'vanilla_spec={wine_annotation} winum_spec={winum_annotation} outcome={outcome}')
        else:
            measured += 1
            if outcome != 'match':
                print(f'case_step={key} windows=[{describe(reference)}] vanilla=[{describe(a)}] '
                      f'winum=[{describe(b)}] outcome={outcome}')
    exports = sum(key.startswith('export/') for key in native)
    print(f'exports={exports} case_steps={measured} ' + ' '.join(f'{key}={differences[key]}' for key in (
        'match', 'fixed_in_winum', 'winum_only_difference', 'shared_difference', 'different_on_both', 'incomplete')))


def main():
    parser = argparse.ArgumentParser(description='Compare measured Win32 API output across Windows and two Wine builds.')
    parser.add_argument('windows', type=Path)
    parser.add_argument('vanilla', type=Path)
    parser.add_argument('winum', type=Path)
    parser.add_argument('--wine-spec', type=Path, help='optional matching vanilla Wine export specification')
    parser.add_argument('--winum-spec', type=Path, help='optional matching Winum export specification')
    try:
        compare(parser.parse_args())
    except (OSError, ValueError) as error:
        parser.error(str(error))


if __name__ == '__main__':
    main()
