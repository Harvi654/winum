#!/usr/bin/env python3
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
"""Check that missing coverage and hidden failures cannot produce an L2 pass."""
import contextlib
import copy
import importlib.util
import io
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('l2', Path(__file__).with_name('statusbar-v2-l2.py'))
l2 = importlib.util.module_from_spec(spec)
spec.loader.exec_module(l2)


class L2Tests(unittest.TestCase):
    def test_summary_and_todo_success(self):
        result = l2.parse('status.c:52: Test succeeded inside todo block: message\n'
                          '00ac:status: 120 tests executed (1 marked as todo, 0 as flaky, 1 failure), 2 skipped.\n', 'status', 1)
        self.assertTrue(result['finished'])
        self.assertEqual(result['fail_at'], {'status.c:52': 1})
        self.assertEqual(result['skipped'], 2)

    def test_wrong_unit_crash_and_timeout(self):
        line = '00ac:status: 120 tests executed (0 marked as todo, 0 as flaky, 0 failures), 0 skipped.\n'
        self.assertFalse(l2.parse(line, 'button', 0)['finished'])
        self.assertFalse(l2.parse(line, 'status', 124, True)['finished'])
        self.assertFalse(l2.parse(line, 'status', 3)['finished'])
        self.assertFalse(l2.parse('', 'status', 0)['finished'])
        self.assertFalse(l2.parse(line+line, 'status', 0)['finished'])

    def compare_case(self, change=None):
        units = ['status'] + ['unit'+str(i) for i in range(20)]
        inputs = dict(schema=1, variant='baseline', commit='same', wine_commit='same',
                      wine_archive_sha256='same', production={}, candidate={}, tests={'status.c': 'same'}, test_policy='same')
        result = dict(finished=True, exit=0, timed_out=False, fail_at={}, summary_count=1,
                      tests=100, todo=0, flaky=0, failures=0, skipped=0)
        baseline = dict(schema=1, setup_errors=[], architectures=['x86_64', 'i386'], expected_units=units,
                        results={f'{a}.{u}': copy.deepcopy(result) for a in ('x86_64', 'i386') for u in units})
        candidate = copy.deepcopy(baseline)
        other_inputs = copy.deepcopy(inputs)
        other_inputs['variant'] = 'candidate'
        if change:
            change(candidate, other_inputs)
        with tempfile.TemporaryDirectory() as root:
            dirs = [Path(root)/'baseline', Path(root)/'candidate']
            for p, i, d in zip(dirs, (inputs, other_inputs), (baseline, candidate)):
                p.mkdir()
                (p/'inputs.json').write_text(json.dumps(i))
                (p/'results.json').write_text(json.dumps(d))
            with contextlib.redirect_stdout(io.StringIO()):
                return l2.compare(*dirs)

    def test_clean_pair(self):
        self.assertEqual(self.compare_case(), 0)

    def test_missing_coverage_and_different_sources(self):
        self.assertEqual(self.compare_case(lambda d, i: d['results'].pop('i386.status')), 1)
        self.assertEqual(self.compare_case(lambda d, i: d['setup_errors'].append('bootstrap')), 1)
        self.assertEqual(self.compare_case(lambda d, i: i['tests'].update({'status.c': 'different'})), 1)

    def test_regressions_and_lost_checks(self):
        for change in (dict(fail_at={'status.c:5': 1}), dict(failures=1),
                       dict(skipped=1), dict(tests=99), dict(finished=False)):
            with self.subTest(change=change):
                self.assertEqual(self.compare_case(lambda d, i: d['results']['x86_64.status'].update(change)), 1)


if __name__ == '__main__':
    unittest.main()
