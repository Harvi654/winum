#!/usr/bin/env python3
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
# Distinguish incomplete backend/coverage from ordinary paired test failures.
import json
import re
import sys
from pathlib import Path


def check(results_path, output):
    results = json.loads(results_path.read_text())
    variant = results_path.stem.removeprefix('results-').rsplit('-', 1)[0]
    errors = []
    report = {'variant': variant, 'architectures': {}, 'errors': errors}
    for arch in ('x86_64', 'i386'):
        prefix = f'mshtml.{arch}.'
        listing = output / f'mshtml-units-{variant}-{arch}.txt'
        expected = listing.read_text().split() if listing.is_file() else []
        units = {key[len(prefix):]: value for key, value in results.items() if key.startswith(prefix)}
        probe = output / f'mshtml-backend-{arch}.txt'
        bits = 64 if arch == 'x86_64' else 32
        usable = probe.is_file() and re.search(rf'^MSHTML_BACKEND usable=1 bits={bits} ',
                                               probe.read_text(), re.M) is not None
        if not usable:
            errors.append(f'{arch}: no successful Gecko-backed document probe')
        if not expected or 'dom' not in expected or len(set(expected)) != len(expected):
            errors.append(f'{arch}: missing or invalid mshtml unit enumeration')
        if set(units) != set(expected):
            errors.append(f'{arch}: enumerated and recorded mshtml units differ')
        for unit, result in units.items():
            if (not result['finished'] or result.get('exit') in ('124', '137')
                    or (result.get('exit') not in (None, '0') and not result['failures'])):
                errors.append(f'{arch}:{unit}: incomplete or timed-out test')
        if not units or not any(value['tests'] for value in units.values()):
            errors.append(f'{arch}: backend-wide skip; no test assertions ran')
        dom = units.get('dom')
        dom_log = output / 'logs' / f'{prefix}dom.log'
        if not dom or not dom['tests'] or not dom_log.is_file():
            errors.append(f'{arch}: dom did not exercise its backend')
        elif re.search(r'Tests skipped: Could not get IHTMLDocument5', dom_log.read_text(errors='replace')):
            errors.append(f'{arch}: dom skipped its required document interface')
        report['architectures'][arch] = {
            'backend_usable': bool(usable), 'expected_units': len(expected), 'recorded_units': len(units),
            'checks': sum(value['tests'] for value in units.values()),
            'skipped': sum(value['skipped'] for value in units.values()),
            'ordinary_failures': sum(value['failures'] for value in units.values()),
        }
    (output / f'mshtml-coverage-{variant}.json').write_text(json.dumps(report, indent=2) + '\n')
    for arch, counters in report['architectures'].items():
        print('MSHTML_COVERAGE', arch, json.dumps(counters, sort_keys=True))
    for error in errors:
        print('MSHTML_INCOMPLETE', error, file=sys.stderr)
    return 1 if errors else 0


if __name__ == '__main__':
    if len(sys.argv) != 3:
        sys.exit('Usage: check-mshtml-l2.py RESULTS_JSON OUTPUT_DIRECTORY')
    sys.exit(check(Path(sys.argv[1]), Path(sys.argv[2])))
