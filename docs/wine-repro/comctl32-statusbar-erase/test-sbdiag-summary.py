#!/usr/bin/env python3
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
# Privacy and cross-thread/phase association matter for a shareable diagnosis.
import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location("sbdiag", Path(__file__).with_name("sbdiag-summary.py"))
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def record(thread, tag, fields):
    return f"001.123:00aa:{thread}:trace:sbdiag:probe {tag} {fields}\n"


class SummaryTests(unittest.TestCase):
    def test_interleaved_calls_and_phase_at_entry(self):
        lines = ["SBMARK before\n",
                 record("0010", "SBSTATE", "hdc=0x1 hwnd=0xa count=0 flags=2"),
                 record("0020", "SBSTATE", "hdc=0x2 hwnd=0xb count=5 flags=0"),
                 record("0020", "SBTEXT", "hdc=0x2 xy=1,2 count=5 flags=0"),
                 record("0020", "SBRETURN", "hdc=0x2 ret=1"),
                 "SBMARK after\n",
                 record("0010", "SBFILL", "hdc=0x1 kind=explicit rect=(0,0)-(10,24) ret=0"),
                 record("0010", "SBRETURN", "hdc=0x1 ret=1")]
        report = module.summarise(lines)
        self.assertIn("complete_calls=2 rejected=0 incomplete=0", report)
        self.assertIn("PHASE before markers=1 calls=2 signatures=2", report)
        self.assertIn("PHASE after markers=1 calls=0 signatures=0", report)
        self.assertIn("SBFILL kind=explicit rect=(0,0)-(10,24) ret=0", report)

    def test_unrelated_strings_and_unknown_fields_never_escape(self):
        secret = "PRIVATE_WINDOW_TITLE"
        lines = [f"vendor message {secret}\n", f"SBMARK {secret}\n",
                 record("0010", "SBSTATE", f"hdc=0x1 count=0 title={secret}"),
                 record("0010", "SBSTATE", f"hdc=0x1 count=0 rect={secret}"),
                 record("0010", "SBUNKNOWN", secret)]
        report = module.summarise(lines)
        self.assertNotIn(secret, report)
        self.assertIn("rejected=3", report)

    def test_wrong_dc_does_not_complete_call(self):
        lines = [record("0010", "SBSTATE", "hdc=0x1 count=0"),
                 record("0010", "SBRETURN", "hdc=0x2 ret=1")]
        report = module.summarise(lines)
        self.assertIn("complete_calls=0", report)
        self.assertIn("incomplete=2", report)


if __name__ == "__main__":
    unittest.main()
