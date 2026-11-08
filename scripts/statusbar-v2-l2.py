#!/usr/bin/env python3
"""Original, AI-assisted (Codex), LGPL-2.1-or-later. Bounded paired L2 results.

Raw Wine logs stay local. Both variants must use identical final test sources.
Missing coverage, timeout or added failures prevent a passing comparison.
"""
import collections
import json
import os
from pathlib import Path
import re
import subprocess
import sys

SUMMARY = re.compile(r'^([0-9a-f]+):(\w+):\s*(\d+) tests executed \((\d+) marked as todo, (\d+) as flaky, (\d+) failures?\), (\d+) skipped\.')
FAIL = re.compile(r'^(\w+\.c:\d+):\s+Test (?:failed|succeeded inside todo block):')


def parse(text, unit, exit_code, timed_out=False):
    summaries = []
    failures = collections.Counter()
    for line in text.splitlines():
        match = SUMMARY.match(line)
        if match and match[2] == unit:
            summaries.append(dict(zip(('tests', 'todo', 'flaky', 'failures', 'skipped'), map(int, match.groups()[2:]))))
        match = FAIL.match(line)
        if match:
            failures[match[1]] += 1
    result = dict(finished=False, exit=exit_code, timed_out=timed_out,
                  fail_at=dict(sorted(failures.items())), summary_count=len(summaries))
    if len(summaries) == 1:
        result.update(summaries[0])
        # Wine caps test failure exit codes at 255. A process with a summary
        # but an inconsistent exit is not counted as a completed unit.
        result['finished'] = not timed_out and exit_code == min(result['failures'], 255)
    return result


def execute(command, log, timeout):
    with log.open('w') as stream:
        process = subprocess.Popen(command, stdout=stream, stderr=subprocess.STDOUT,
                                   start_new_session=True)
        timed_out = False
        try:
            code = process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            import signal
            timed_out = True
            # Also stop child Windows processes created by the unit.
            os.killpg(process.pid, signal.SIGKILL)
            process.wait()
            code = 124
    return code, timed_out, log.read_text(errors='replace')


def run(source, build, logs, output):
    source, build, logs, output = map(Path, (source, build, logs, output))
    logs.mkdir(parents=True, exist_ok=False)
    units = sorted(re.findall(r'^\s+([a-z0-9_]+)\.c\s*\\?$',
                              (source/'dlls/comctl32/tests/Makefile.in').read_text(), re.M))
    if len(units) < 20 or 'status' not in units or len(units) != len(set(units)):
        raise RuntimeError('Unexpected comctl32 test inventory')
    data = dict(schema=1, expected_units=units, architectures=['x86_64', 'i386'],
                setup_errors=[], results={})
    code, timed_out, _ = execute([str(build/'wine'), 'wineboot', '-i'], logs/'bootstrap.log', 180)
    if code or timed_out:
        data['setup_errors'].append('prefix initialization failed')
    else:
        for arch in data['architectures']:
            exe = build/f'dlls/comctl32/tests/{arch}-windows/comctl32_test.exe'
            if not exe.is_file():
                data['setup_errors'].append(f'{arch}: missing test executable')
                continue
            code, timed_out, listing = execute([str(build/'wine'), str(exe), '--list'], logs/f'{arch}-list.log', 120)
            actual = sorted(re.findall(r'^    ([a-z0-9_]+)\s*$', listing, re.M))
            if code or timed_out or actual != units:
                data['setup_errors'].append(f'{arch}: enumeration does not match source inventory')
                continue
            for unit in units:
                code, timed_out, text = execute([str(build/'wine'), str(exe), unit],
                                               logs/f'{arch}-{unit}.log', int(os.environ.get('UNIT_TIMEOUT', '900')))
                data['results'][f'{arch}.{unit}'] = parse(text, unit, code, timed_out)
                print(f"UNIT {arch}.{unit} finished={int(data['results'][f'{arch}.{unit}']['finished'])}", flush=True)
                if timed_out:
                    # Reset the server and desktop connections before another unit.
                    subprocess.run([str(build/'server/wineserver'), '-k'], check=True, timeout=30)
                    subprocess.run([str(build/'server/wineserver'), '-w'], check=True, timeout=30)
    output.write_text(json.dumps(data, indent=2, sort_keys=True)+'\n')
    return 0 if not data['setup_errors'] and all(r['finished'] for r in data['results'].values()) else 2


def compare(baseline_dir, candidate_dir):
    paths = [Path(baseline_dir), Path(candidate_dir)]
    inputs = [json.loads((p/'inputs.json').read_text()) for p in paths]
    data = [json.loads((p/'results.json').read_text()) for p in paths]
    problems, changes = [], []
    if [i['variant'] for i in inputs] != ['baseline', 'candidate']:
        problems.append('Wrong baseline/candidate variant labels')
    for field in ('commit', 'wine_commit', 'wine_archive_sha256', 'production', 'candidate', 'tests', 'test_policy'):
        if inputs[0][field] != inputs[1][field]:
            problems.append(f'Inputs differ: {field}')
    for i, d in enumerate(data):
        label = ('baseline', 'candidate')[i]
        if d['setup_errors']:
            problems.extend(f'{label}: {e}' for e in d['setup_errors'])
        expected = {f'{arch}.{unit}' for arch in ('x86_64', 'i386') for unit in d['expected_units']}
        if len(d['expected_units']) < 20 or 'status' not in d['expected_units']:
            problems.append(f'{label}: invalid unit inventory')
        if set(d['results']) != expected:
            problems.append(f'{label}: missing or extra unit results')
        for key, result in d['results'].items():
            if not result['finished']:
                problems.append(f'{label} {key}: incomplete (exit {result["exit"]})')
    if data[0]['expected_units'] != data[1]['expected_units']:
        problems.append('Unit inventories differ')
    for key in sorted(set(data[0]['results']) & set(data[1]['results'])):
        a, b = (d['results'][key] for d in data)
        if not a['finished'] or not b['finished']:
            continue
        new = {p: n for p, n in b['fail_at'].items() if n > a['fail_at'].get(p, 0)}
        if new:
            problems.append(f'{key}: added failure locations/counts: {new}')
        if b['failures'] > a['failures']:
            problems.append(f'{key}: failures increased {a["failures"]} -> {b["failures"]}')
        if b['skipped'] > a['skipped']:
            problems.append(f'{key}: skips increased {a["skipped"]} -> {b["skipped"]}')
        if b['tests'] < a['tests']:
            problems.append(f'{key}: checks decreased {a["tests"]} -> {b["tests"]}')
        if a != b:
            changes.append(f'{key}: checks {a["tests"]} -> {b["tests"]}, failures {a["failures"]} -> {b["failures"]}, skips {a["skipped"]} -> {b["skipped"]}')
    print('# Status-bar v2: paired comctl32 L2\n')
    print('Same Wine 11.20 source and production series; identical final candidate test sources in both builds.\n')
    print(f'Expected units per architecture: {len(data[0]["expected_units"])}. Problems requiring review: {len(problems)}.\n')
    print('## Problems\n\n'+ ('\n'.join('- '+p for p in problems) or 'None detected.'))
    print('\n## Changed results\n\n'+ ('\n'.join('- '+c for c in changes) or 'None.'))
    print('\nRaw logs remain on the runner. This compares one run per variant; timing-dependent differences require reruns. Shared baseline failures are not asserted to be correct. Unthemed-v6 coverage and CAD validation remain separate.')
    return 1 if problems else 0


if __name__ == '__main__':
    if len(sys.argv) == 6 and sys.argv[1] == 'run':
        sys.exit(run(*sys.argv[2:]))
    if len(sys.argv) == 4 and sys.argv[1] == 'compare':
        sys.exit(compare(*sys.argv[2:]))
    sys.exit('Usage: statusbar-v2-l2.py run SOURCE BUILD LOGDIR OUTPUT.json | compare BASELINE_DIR CANDIDATE_DIR')
