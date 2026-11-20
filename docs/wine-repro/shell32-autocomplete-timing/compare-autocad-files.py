#!/usr/bin/env python3
# Original, AI-assisted (Codex), LGPL-2.1-or-later.

import argparse
from collections import Counter
from pathlib import Path
import re


VERSION = 'winum-autocad-file-inventory-v1'
GROUPS = ('acad', 'dotnet', 'program-files')
PRIMARY = {
    'acad/acad.exe', 'acad/accoreconsole.exe', 'acad/acjab.dll',
    'acad/admigrator.xml', 'acad/support/accommandweight.xml',
    'program-files/acjab.dll', 'program-files/admigrator.xml',
    'program-files/support/accommandweight.xml', 'dotnet/dotnet.exe',
}
RUNTIME_COMPONENT = re.compile(
    r'dotnet/(?:host/fxr|shared/microsoft\.(?:netcore|windowsdesktop)\.app)/'
    r'10\.[\da-z.-]+/(?:hostfxr|hostpolicy|coreclr|clrjit|wpfgfx_cor3|d3dcompiler_47_cor3)\.dll'
)
ROOTS = {
    'acad', 'dotnet/host/fxr', 'dotnet/shared/microsoft.netcore.app',
    'dotnet/shared/microsoft.windowsdesktop.app',
}
RUNTIME_ROOT = re.compile(
    r'dotnet/(?:host/fxr|shared/microsoft\.(?:netcore|windowsdesktop)\.app)/10\.[\da-z.-]+'
)


def load_inventory(path):
    lines = path.read_text(encoding='utf-8').splitlines()
    if not lines or lines[0] != VERSION or lines[-1] != 'end':
        raise ValueError('missing inventory header or completion marker')
    roots, files = {}, {}
    for line in lines[1:-1]:
        fields = line.split('\t')
        if len(fields) == 3 and fields[0] == 'root':
            name, state = fields[1].lower(), fields[2]
            if (name in roots or state not in ('present', 'missing') or
                    (name not in ROOTS and not RUNTIME_ROOT.fullmatch(name))):
                raise ValueError('invalid or duplicate directory record')
            roots[name] = state
        elif len(fields) == 4 and fields[0] == 'file':
            name, size, digest = fields[1].lower(), fields[2], fields[3]
            if (name in files or name.partition('/')[0] not in GROUPS or
                    not re.fullmatch(r'[0-9]+', size) or not re.fullmatch(r'[a-f0-9]{64}', digest)):
                raise ValueError('invalid or duplicate file record')
            files[name] = (int(size), digest)
        else:
            raise ValueError('invalid inventory line')
    if not ROOTS.issubset(roots):
        raise ValueError('missing inventory directory records')
    return roots, files


def file_status(windows, prefix):
    if windows is None:
        return 'prefix_only' if prefix is not None else 'missing_both'
    if prefix is None:
        return 'windows_only'
    return 'same' if windows == prefix else 'different'


def compare(windows_roots, windows, prefix_roots, prefix, details):
    for name in sorted(windows_roots.keys() | prefix_roots.keys()):
        print(f'root={name} windows={windows_roots.get(name, "missing")} '
              f'prefix={prefix_roots.get(name, "missing")}')

    counts = Counter()
    components = set(PRIMARY)
    with details.open('x', encoding='utf-8') as report:
        report.write('status\trelative_path\twindows_bytes\tprefix_bytes\twindows_sha256\tprefix_sha256\n')
        for name in sorted(windows.keys() | prefix.keys()):
            state = file_status(windows.get(name), prefix.get(name))
            counts[name.partition('/')[0], state] += 1
            if state != 'same':
                win_size, win_hash = windows.get(name, ('-', '-'))
                prefix_size, prefix_hash = prefix.get(name, ('-', '-'))
                report.write(f'{state}\t{name}\t{win_size}\t{prefix_size}\t{win_hash}\t{prefix_hash}\n')
            if RUNTIME_COMPONENT.fullmatch(name):
                components.add(name)

    for group in GROUPS:
        print(f'group={group} ' + ' '.join(f'{state}={counts[group, state]}' for state in
                                          ('same', 'different', 'windows_only', 'prefix_only')))
    for name in sorted(components):
        print(f'component={name} status={file_status(windows.get(name), prefix.get(name))}')


def main():
    parser = argparse.ArgumentParser(description='Compare the Windows and Wine installation inventories.')
    parser.add_argument('windows_inventory', type=Path)
    parser.add_argument('prefix_inventory', type=Path)
    parser.add_argument('local_details', type=Path, help='new file for the full difference list')
    args = parser.parse_args()
    try:
        compare(*load_inventory(args.windows_inventory), *load_inventory(args.prefix_inventory), args.local_details)
    except ValueError as error:
        parser.exit(2, f'Invalid inventory: {error}\n')
    except OSError:
        parser.exit(2, 'Could not read an inventory or create the new local detail file.\n')


if __name__ == '__main__':
    main()
