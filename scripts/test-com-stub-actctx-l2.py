#!/usr/bin/env python3
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
"""Reject incomplete pairs, shared conformance failures and misleading counts."""
import contextlib
import copy
import importlib.util
import io
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('pair', Path(__file__).with_name('com-stub-actctx-l2.py'))
pair = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pair)


class ComparisonTests(unittest.TestCase):
    def compare_case(self, mutate=None):
        meta = dict(schema=1, variant='without', commit='same', wine_commit='same', archive_sha256='same',
                    production={}, candidate='same', tests={'stub_actctx.c': 'same'}, policy='same',
                    omitted=['combase-stub-activation-context.patch'], implementation={'rpc.c': 'old'})
        other = copy.deepcopy(meta)
        other.update(variant='with', omitted=[], implementation={'rpc.c': 'new'})
        unit = dict(finished=True, exit=0, timed_out=False, fail_at={}, summary_count=1,
                    tests=291, todo=0, flaky=0, failures=0, skipped=0, context_failures={})
        data = dict(schema=1, setup_errors=[], results={key: copy.deepcopy(unit) for key in pair.KEYS})
        second = copy.deepcopy(data)
        if mutate: mutate(meta, other, data, second)
        with tempfile.TemporaryDirectory() as root:
            paths = [Path(root)/'without', Path(root)/'with']
            for path, inputs, results in zip(paths, (meta, other), (data, second)):
                path.mkdir()
                (path/'inputs.json').write_text(json.dumps(inputs))
                (path/'results.json').write_text(json.dumps(results))
            output = io.StringIO()
            with contextlib.redirect_stdout(output): code = pair.compare(*paths)
            return code, output.getvalue()

    def test_missing_files_never_pass(self):
        with tempfile.TemporaryDirectory() as root, contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(pair.compare(root, root), 1)

    def test_clean_pair(self):
        self.assertEqual(self.compare_case()[0], 0)

    def test_shared_new_unit_failure_is_not_conformance(self):
        def mutate(_, __, a, b):
            for data in (a, b):
                data['results']['combase.x86_64.stub_actctx'].update(failures=16, fail_at={'stub_actctx.c:120': 16})
        self.assertEqual(self.compare_case(mutate)[0], 1)

    def test_same_counts_can_hide_different_context_failures(self):
        def mutate(_, __, a, b):
            for data in (a, b): data['results']['combase.x86_64.stub_actctx']['failures'] = 1
            a['results']['combase.x86_64.stub_actctx']['context_failures'] = {'Scenario 5 callback 3 mask 0, expected 1.': 1}
            b['results']['combase.x86_64.stub_actctx']['context_failures'] = {'Scenario 5 callback 1 mask 1, expected 0.': 1}
        code, text = self.compare_case(mutate)
        self.assertEqual(code, 1)
        self.assertIn('callback 1 mask 1, expected 0. [0 -> 1]', text)
        self.assertIn('callback 3 mask 0, expected 1. [1 -> 0]', text)

    def test_missing_coverage_and_setup_failure(self):
        self.assertEqual(self.compare_case(lambda i,j,a,b: b['results'].pop('combase.i386.stub_actctx'))[0], 1)
        self.assertEqual(self.compare_case(lambda i,j,a,b: b['setup_errors'].append('bootstrap'))[0], 1)
        for change in ({'finished': False}, {'tests': 290}, {'skipped': 1}, {'todo': 1}, {'flaky': 1}):
            self.assertEqual(self.compare_case(lambda i,j,a,b: b['results']['combase.i386.stub_actctx'].update(change))[0], 1)

    def test_different_inputs_and_wrong_recipe(self):
        self.assertEqual(self.compare_case(lambda i,j,a,b: j['tests'].update({'stub_actctx.c': 'other'}))[0], 1)
        self.assertEqual(self.compare_case(lambda i,j,a,b: j.update(omitted=['another.patch']))[0], 1)
        self.assertEqual(self.compare_case(lambda i,j,a,b: j.update(implementation=i['implementation']))[0], 1)


if __name__ == '__main__': unittest.main()
