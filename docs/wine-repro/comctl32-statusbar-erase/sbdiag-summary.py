#!/usr/bin/env python3
"""Summarise +sbdiag numeric records; never echo unrelated Wine log lines.

Original, AI-assisted (Codex), LGPL-2.1-or-later.
"""
import argparse
from collections import Counter, defaultdict
import re

PHASES = ("before", "rt_start", "after", "restored", "remax")
LINE = re.compile(r"([0-9a-f]+):trace:sbdiag:\w+\s+(SB\w+) (.*)$", re.I)
MARK = re.compile(r"^SBMARK (before|rt_start|after|restored|remax)\s*$")
# Fixed fields only. A whole input line, text argument or unknown field is never printed.
FIELDS = {
    "SBSTATE": "hwnd hdc count flags xy rect dcx vis device wnd vp map gm layout bk text bkmode align matrix backend",
    "SBREGION": "hdc kind present count hash box",
    "SBRECT": "hdc kind index rect",
    "SBSURFACE": "hdc window_driver surface dummy hwnd rect bitmap dibrect stride",
    "SBFILL": "hdc kind rect ret",
    "SBTEXT": "hdc xy count flags",
    "SBRETURN": "hdc ret",
}
KINDS = {"visible", "app", "meta", "effective", "dib", "explicit", "implicit"}
NUMBER = r"(?:-?\d+(?:\.\d+)?(?:e[+-]?\d+)?|0x[0-9a-f]+|[0-9a-f]+)"
VALUE = re.compile(r"(?:" + NUMBER + r"|\(nil\)|\(null\)|"
                   r"\(-?\d+,-?\d+\)-\(-?\d+,-?\d+\)|"
                   r"" + NUMBER + r"(?:[,/]" + NUMBER + r")+)$", re.I)


def parse_fields(tag, payload):
    fields = {}
    for item in payload.split():
        key, sep, value = item.partition("=")
        if not sep or key not in FIELDS[tag].split() or key in fields:
            return None
        if (key == "kind" and value not in KINDS) or (key != "kind" and not VALUE.fullmatch(value)):
            return None
        fields[key] = value
    if "hdc" not in fields:
        return None
    return fields


def summarise(lines, limit=12):
    phase = "unmarked"
    pending = {}
    signatures = defaultdict(Counter)
    markers = Counter()
    rejected = 0
    incomplete = 0
    for line in lines:
        mark = MARK.fullmatch(line.strip())
        if mark:
            phase = mark[1]
            markers[phase] += 1
            continue
        match = LINE.search(line.rstrip())
        if not match:
            continue
        thread, tag, payload = match.groups()
        if tag not in FIELDS:
            rejected += 1
            continue
        fields = parse_fields(tag, payload)
        if fields is None:
            rejected += 1
            continue
        hdc = fields["hdc"]
        if tag == "SBSTATE":
            if "count" not in fields:
                rejected += 1
                continue
            if thread in pending:
                incomplete += 1
            kind = "fill-only" if fields["count"] == "0" else "text"
            # Length varies with the status text; do not make it a new state signature.
            state = " ".join(f"{k}={v}" for k, v in fields.items() if k not in ("hdc", "count"))
            pending[thread] = (phase, hdc, [f"SBSTATE op={kind} {state}"])
            continue
        call = pending.get(thread)
        if not call or call[1] != hdc:
            incomplete += 1
            continue
        # Return value and all region records stay attached to the real calling thread/DC.
        values = " ".join(f"{k}={v}" for k, v in fields.items() if k not in ("hdc", "count") or tag == "SBREGION" and k == "count")
        call[2].append(f"{tag} {values}")
        if tag == "SBRETURN":
            signatures[call[0]][tuple(call[2])] += 1
            del pending[thread]
    incomplete += len(pending)
    out = [f"SBREPORT complete_calls={sum(sum(c.values()) for c in signatures.values())} rejected={rejected} incomplete={incomplete}"]
    for name in ("unmarked",) + PHASES:
        counts = signatures[name]
        out.append(f"PHASE {name} markers={markers[name]} calls={sum(counts.values())} signatures={len(counts)}")
        for i, (signature, count) in enumerate(counts.most_common(limit), 1):
            out.append(f"SIGNATURE {i} calls={count}")
            out.extend("  " + item for item in signature)
        if len(counts) > limit:
            out.append(f"OMITTED signatures={len(counts) - limit}; increase --limit locally")
    if not signatures["before"] or not signatures["after"]:
        out.append("CHECK missing before/after calls: verify the diagnostic Wine, +sbdiag and phase markers")
    return "\n".join(out) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log")
    parser.add_argument("--limit", type=int, default=12)
    args = parser.parse_args()
    if args.limit < 1:
        parser.error("--limit must be positive")
    with open(args.log, errors="replace") as stream:
        print(summarise(stream, args.limit), end="")


if __name__ == "__main__":
    main()
