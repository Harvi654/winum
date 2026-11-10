#!/usr/bin/env python3
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
# Exercise fail-closed coverage and download guards without running Wine.
import contextlib
import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location('mshtml_gate', ROOT / 'scripts/check-mshtml-l2.py')
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)


class CoverageTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.output = Path(self.temp.name)
        (self.output / 'logs').mkdir()
        self.results = {}
        for arch, bits in [('x86_64', 64), ('i386', 32)]:
            (self.output / f'mshtml-backend-{arch}.txt').write_text(
                f'MSHTML_BACKEND usable=1 bits={bits} stage=window hr=0\n')
            (self.output / f'mshtml-units-series-{arch}.txt').write_text('dom\nmisc\n')
            for unit in ('dom', 'misc'):
                self.results[f'mshtml.{arch}.{unit}'] = {
                    'finished': True, 'tests': 100, 'todo': 4, 'flaky': 0,
                    'failures': 0, 'skipped': 1, 'exit': None,
                }
                (self.output / 'logs' / f'mshtml.{arch}.{unit}.log').write_text('ordinary suite output\n')

    def run_gate(self):
        path = self.output / 'results-series-g5.json'
        path.write_text(json.dumps(self.results))
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            return gate.check(path, self.output)

    def test_complete_with_optional_feature_skips(self):
        self.assertEqual(self.run_gate(), 0)

    def test_ordinary_failures_are_left_to_paired_comparison(self):
        self.results['mshtml.x86_64.dom'].update(failures=3, exit='3')
        self.assertEqual(self.run_gate(), 0)

    def test_missing_backend_or_wrong_architecture(self):
        for text in ('', 'MSHTML_BACKEND usable=0 bits=32 stage=load hr=1\n',
                     'MSHTML_BACKEND usable=1 bits=64 stage=window hr=0\n'):
            with self.subTest(text=text):
                (self.output / 'mshtml-backend-i386.txt').write_text(text)
                self.assertEqual(self.run_gate(), 1)

    def test_missing_enumeration_or_unit(self):
        (self.output / 'mshtml-units-series-i386.txt').unlink()
        self.assertEqual(self.run_gate(), 1)
        (self.output / 'mshtml-units-series-i386.txt').write_text('dom\nmisc\n')
        del self.results['mshtml.i386.misc']
        self.assertEqual(self.run_gate(), 1)

    def test_timeout_or_unfinished(self):
        for values in ({'finished': False}, {'exit': '124'}, {'exit': '137'}, {'exit': '139'}):
            with self.subTest(values=values):
                before = dict(self.results['mshtml.x86_64.dom'])
                self.results['mshtml.x86_64.dom'].update(values)
                self.assertEqual(self.run_gate(), 1)
                self.results['mshtml.x86_64.dom'] = before

    def test_backend_wide_skip(self):
        for value in self.results.values():
            value.update(tests=0, skipped=1)
        self.assertEqual(self.run_gate(), 1)

    def test_dom_interface_skip(self):
        (self.output / 'logs/mshtml.i386.dom.log').write_text(
            'dom.c:630: Tests skipped: Could not get IHTMLDocument5, probably too old IE\n')
        self.assertEqual(self.run_gate(), 1)


class GeckoGuards(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.cache = self.root / 'cache'
        self.cache.mkdir()
        self.source = self.root / 'source/dlls/appwiz.cpl'
        self.source.mkdir(parents=True)
        self.digest = hashlib.sha256(b'own fixture').hexdigest()
        self.pins = self.root / 'pins.env'
        self.pins.write_text(f'WINE_GECKO_VERSION=2.47.4\nWINE_GECKO_SHA256_X86={self.digest}\n'
                             f'WINE_GECKO_SHA256_X86_64={self.digest}\n')
        self.source_file = self.source / 'addons.c'
        self.source_file.write_text(
            '#define GECKO_VERSION "2.47.4"\n#ifdef __i386__\n#define GECKO_ARCH "x86"\n'
            f'#define GECKO_SHA "{self.digest}"\n#elif defined(__x86_64__)\n'
            f'#define GECKO_ARCH "x86_64"\n#define GECKO_SHA "{self.digest}"\n')
        for arch in ('x86', 'x86_64'):
            (self.cache / f'wine-gecko-2.47.4-{arch}.msi').write_bytes(b'own fixture')

    def fetch(self):
        # A mock transport can only write our temporary fixture. It never
        # contacts a network endpoint and cannot produce a valid test MSI.
        command = '''
source "$1"
curl() {
    : > "$GECKO_TEST_CALLED"
    while [ "$1" != -o ]; do shift; done
    printf 'corrupt download' > "$2"
}
wine_fetch_gecko "$2" "$3" "$4"
'''
        return subprocess.run(['bash', '-c', command, 'gecko-guard',
                               str(ROOT / 'scripts/lib/wine-gecko.sh'),
                               str(self.root / 'source'), str(self.cache), str(self.pins)],
                              env=dict(os.environ, GECKO_TEST_CALLED=str(self.root / 'curl-called')),
                              capture_output=True, text=True)

    def test_verified_cache_does_not_download(self):
        self.assertEqual(self.fetch().returncode, 0)
        self.assertFalse((self.root / 'curl-called').exists())

    def test_source_version_mismatch_stops_before_download(self):
        self.source_file.write_text(self.source_file.read_text().replace('2.47.4', '0.0'))
        self.assertNotEqual(self.fetch().returncode, 0)
        self.assertFalse((self.root / 'curl-called').exists())

    def test_corrupt_cache_is_rejected(self):
        (self.cache / 'wine-gecko-2.47.4-x86.msi').write_bytes(b'wrong cache')
        self.assertNotEqual(self.fetch().returncode, 0)
        self.assertFalse((self.root / 'curl-called').exists())

    def test_download_hash_mismatch_never_promotes_partial_file(self):
        final = self.cache / 'wine-gecko-2.47.4-x86.msi'
        final.unlink()
        self.assertNotEqual(self.fetch().returncode, 0)
        self.assertTrue((self.root / 'curl-called').exists())
        self.assertFalse(final.exists())


class CleanupTests(unittest.TestCase):
    def test_failed_setup_cleans_both_prefixes_without_leaking_child_export(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / 'server').mkdir()
            server = root / 'server/wineserver'
            server.write_text('#!/bin/sh\nprintf "%s\\n" "$WINEPREFIX" >> "$GECKO_TEST_TRACE"\n')
            server.chmod(0o700)
            source = (ROOT / 'scripts/test-series.sh').read_text()
            begin = source.index('cleanup_test_processes()\n')
            end = source.index('\n', source.index('trap cleanup_test_processes EXIT', begin))
            cleanup = source[begin:end]
            display = subprocess.Popen(['sleep', '30'])
            try:
                command = '''set -eu
export WINEPREFIX="$PWD/normal"
mshtml_prefix="$PWD/html"
XVFB_PID="$1"
''' + cleanup + '''
(
    trap - EXIT
    export WINEPREFIX="$mshtml_prefix"
    exit 7
)
'''
                result = subprocess.run(['bash', '-c', command, 'cleanup-test', str(display.pid)],
                                        cwd=root, capture_output=True, text=True,
                                        env=dict(os.environ, GECKO_TEST_TRACE=str(root / 'trace')))
                self.assertEqual(result.returncode, 7)
                self.assertEqual((root / 'trace').read_text().splitlines(),
                                 [str(root / 'normal'), str(root / 'html')])
                self.assertNotEqual(display.wait(timeout=3), 0)
            finally:
                if display.poll() is None:
                    display.terminate()
                    display.wait(timeout=3)


if __name__ == '__main__':
    unittest.main()
