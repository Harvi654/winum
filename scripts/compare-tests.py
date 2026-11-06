#!/usr/bin/env python3
# Original AI-assisted (Claude), LGPL-2.1-or-later.
# summarize LOGDIR            -> JSON on stdout: per dll.arch.unit the counts of the
#                                Wine test summary line and the failing source lines
# compare VANILLA.json SERIES.json -> Markdown report on stdout; exit 1 when the
#                                series adds a failure location or a unit stops
#                                finishing (L2 regression), else 0
import json, re, sys
from collections import Counter
from pathlib import Path

SUMMARY = re.compile(r'^[0-9a-f]+:(\w+): (\d+) tests executed \((\d+) marked as todo, (\d+) as flaky, (\d+) failures?\), (\d+) skipped')
FAIL = re.compile(r'^(\w+\.c:\d+): Test (?:failed|succeeded inside todo block)')

def summarize(logdir):
    out = {}
    for log in sorted(Path(logdir).glob('*.log')):
        dll, arch, unit = log.stem.split('.', 2)
        r = {"finished": False, "tests": 0, "todo": 0, "flaky": 0, "failures": 0, "skipped": 0,
             "fail_at": Counter(), "exit": None}
        for line in log.read_text(errors='replace').splitlines():
            m = SUMMARY.match(line)
            if m and m.group(1) == unit:
                r.update(finished=True, tests=int(m.group(2)), todo=int(m.group(3)),
                         flaky=int(m.group(4)), failures=int(m.group(5)), skipped=int(m.group(6)))
            m = FAIL.match(line)
            if m:
                r["fail_at"][m.group(1)] += 1
            if line.startswith('exit='):
                r["exit"] = line[5:]
        r["fail_at"] = dict(r["fail_at"])
        out[f"{dll}.{arch}.{unit}"] = r
    json.dump(out, sys.stdout, indent=1, sort_keys=True)

def compare(vpath, spath):
    v, s = json.load(open(vpath)), json.load(open(spath))
    regress, fixed, changed = [], [], []
    for key in sorted(set(v) | set(s)):
        a, b = v.get(key), s.get(key)
        if a is None or b is None:
            changed.append((key, "only in " + ("series" if a is None else "vanilla")))
            continue
        if a["finished"] and not b["finished"]:
            regress.append((key, "does not finish with the series (exit %s)" % b["exit"]))
            continue
        new_at = {k: n for k, n in b["fail_at"].items() if n > a["fail_at"].get(k, 0)}
        gone_at = {k: n for k, n in a["fail_at"].items() if n > b["fail_at"].get(k, 0)}
        if new_at:
            regress.append((key, "new failures: " + ", ".join(f"{k} x{n}" for k, n in sorted(new_at.items()))))
        if gone_at:
            fixed.append((key, "fixed: " + ", ".join(sorted(gone_at))))
        if a["tests"] != b["tests"] and not new_at:
            changed.append((key, f"checks {a['tests']} -> {b['tests']}"))
    print("# L2: vanilla Wine against the winum series\n")
    print(f"Units: {len(s)}. Regressions: {len(regress)}. Fixed: {len(fixed)}. Other changes: {len(changed)}.\n")
    for title, rows in (("Regressions (new failure locations)", regress), ("Fixed", fixed), ("Other changes", changed)):
        print(f"## {title}\n")
        print("\n".join(f"- `{k}`: {t}" for k, t in rows) or "none")
        print()
    print("A regression may still be a timing-dependent test that fails on either side;")
    print("such units are re-run before a result is recorded on a patch page.")
    return 1 if regress else 0

if __name__ == "__main__":
    if sys.argv[1:2] == ["summarize"] and len(sys.argv) == 3:
        summarize(sys.argv[2])
    elif sys.argv[1:2] == ["compare"] and len(sys.argv) == 4:
        sys.exit(compare(sys.argv[2], sys.argv[3]))
    else:
        sys.exit("usage: compare-tests.py summarize LOGDIR | compare VANILLA.json SERIES.json")
