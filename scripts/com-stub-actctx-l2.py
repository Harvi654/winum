#!/usr/bin/env python3
"""Original, AI-assisted (Codex), LGPL-2.1-or-later. Isolated COM capture pair.

Reuse the already tested Wine summary/timeout parser. Never hide a shared
failure of the new Windows-tested regression, even when counts are equal.
"""
import collections
import importlib.util
import json
import os
from pathlib import Path
import re
import sys

spec = importlib.util.spec_from_file_location('wine_l2', Path(__file__).with_name('statusbar-v2-l2.py'))
shared = importlib.util.module_from_spec(spec)
spec.loader.exec_module(shared)
UNITS = {'combase': ['roapi', 'string', 'stub_actctx'], 'ole32': ['marshal'],
         'oleaut32': ['tmarshal'], 'rpcrt4': ['cstub']}
KEYS = {f'{dll}.{arch}.{unit}' for dll, units in UNITS.items()
        for arch in ('x86_64', 'i386') for unit in units}


def run(build, logs, output):
    build, logs, output = map(Path, (build, logs, output))
    logs.mkdir(parents=True, exist_ok=False)
    data = dict(schema=1, setup_errors=[], results={})
    code, timed, _ = shared.execute([str(build/'wine'), 'wineboot', '-i'], logs/'bootstrap.log', 180)
    if code or timed:
        data['setup_errors'].append('prefix initialization failed')
    else:
        for dll, units in UNITS.items():
            for arch in ('x86_64', 'i386'):
                exe = build/f'dlls/{dll}/tests/{arch}-windows/{dll}_test.exe'
                if not exe.is_file():
                    data['setup_errors'].append(f'{dll}.{arch}: missing executable')
                    continue
                code, timed, listing = shared.execute([str(build/'wine'), str(exe), '--list'], logs/f'{dll}-{arch}-list.log', 120)
                actual = re.findall(r'^    ([a-z0-9_]+)\s*$', listing, re.M)
                if code or timed or not set(units).issubset(actual):
                    data['setup_errors'].append(f'{dll}.{arch}: missing requested unit')
                    continue
                for unit in units:
                    key = f'{dll}.{arch}.{unit}'
                    code, timed, text = shared.execute([str(build/'wine'), str(exe), unit], logs/f'{key}.log', 300)
                    result = shared.parse(text, unit, code, timed)
                    if unit == 'stub_actctx':
                        # Only our synthetic callback failures: no vendor data/logs.
                        messages = re.findall(r'^stub_actctx\.c:\d+: Test failed: (Scenario \d+ callback \d+ mask \d+, expected \d+\.)$', text, re.M)
                        result['context_failures'] = dict(sorted(collections.Counter(messages).items()))
                    data['results'][key] = result
                    print(f'UNIT {key} finished={int(result["finished"])} failures={result.get("failures", "unknown")}', flush=True)
                    if timed:
                        data['setup_errors'].append(f'{key}: timeout; remaining units not run')
                        output.write_text(json.dumps(data,indent=2,sort_keys=True)+'\n')
                        return 2
    output.write_text(json.dumps(data,indent=2,sort_keys=True)+'\n')
    return 0 if not data['setup_errors'] and set(data['results']) == KEYS and all(r['finished'] for r in data['results'].values()) else 2


def compare(without, with_patch):
    dirs = list(map(Path, (without, with_patch)))
    try:
        inputs = [json.loads((p/'inputs.json').read_text()) for p in dirs]
        results = [json.loads((p/'results.json').read_text()) for p in dirs]
    except (OSError, ValueError):
        print('# COM stub capture: incomplete pair\n\nMissing or invalid input/results JSON; no runtime verdict.')
        return 1
    problems, changes = [], []
    if [i['variant'] for i in inputs] != ['without', 'with']:
        problems.append('Wrong variant labels')
    for field in ('commit', 'wine_commit', 'archive_sha256', 'production', 'candidate', 'tests', 'policy'):
        if inputs[0][field] != inputs[1][field]: problems.append(f'Inputs differ: {field}')
    if inputs[0]['omitted'] != ['combase-stub-activation-context.patch'] or inputs[1]['omitted']:
        problems.append('Wrong isolated recipe')
    if inputs[0]['implementation'] == inputs[1]['implementation']:
        problems.append('Implementation did not change')
    for label, data in zip(('without', 'with'), results):
        problems.extend(f'{label}: {e}' for e in data['setup_errors'])
        if set(data['results']) != KEYS: problems.append(f'{label}: incomplete requested coverage')
        for key, result in data['results'].items():
            if not result['finished']: problems.append(f'{label} {key}: incomplete run')
            if key.endswith('.stub_actctx') and result.get('finished'):
                if result['skipped'] or result['tests'] < 291 or result['todo'] or result['flaky']:
                    problems.append(f'{label} {key}: incomplete assertion coverage')
                if label == 'with' and result['failures']:
                    problems.append(f'{key}: production capture fails the Windows-tested regression')
    for key in sorted(KEYS & results[0]['results'].keys() & results[1]['results'].keys()):
        a, b = (d['results'][key] for d in results)
        if not a['finished'] or not b['finished']: continue
        changes.append(f'{key}: checks {a["tests"]} -> {b["tests"]}, failures {a["failures"]} -> {b["failures"]}, skips {a["skipped"]} -> {b["skipped"]}')
        if b['skipped'] > a['skipped'] or b['tests'] < a['tests']:
            problems.append(f'{key}: lost coverage')
        if any(n > a['fail_at'].get(line, 0) for line, n in b['fail_at'].items()) or b['failures'] > a['failures']:
            problems.append(f'{key}: added failure locations/counts')
        if key.endswith('.stub_actctx'):
            old, new = (r.get('context_failures', {}) for r in (a, b))
            for message in sorted(set(old) | set(new)):
                if old.get(message, 0) != new.get(message, 0):
                    changes.append(f'{key}: {message} [{old.get(message, 0)} -> {new.get(message, 0)}]')
    print('# COM stub capture: isolated paired Wine comparison\n')
    print('Same Wine 11.20, final tests and other production patches. Only the target implementation differs.\n')
    print('## Problems\n\n'+ ('\n'.join('- '+p for p in problems) or 'None detected.'))
    print('\n## Results\n\n'+ ('\n'.join('- '+c for c in changes) or 'No complete paired units.'))
    print('\nShared failures of stub_actctx never count as conformance. This is a focused pair, not the complete neighbouring DLL L2 or CAD validation. Raw logs stay on the runner.')
    return 1 if problems else 0


if __name__ == '__main__':
    if len(sys.argv) == 5 and sys.argv[1] == 'run': sys.exit(run(*sys.argv[2:]))
    if len(sys.argv) == 4 and sys.argv[1] == 'compare': sys.exit(compare(*sys.argv[2:]))
    sys.exit('Usage: com-stub-actctx-l2.py run BUILD LOGDIR OUTPUT.json | compare WITHOUT_DIR WITH_DIR')
