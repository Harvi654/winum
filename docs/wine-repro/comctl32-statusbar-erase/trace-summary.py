#!/usr/bin/env python3
# Summarise a WINEDEBUG=+statusbar,+win log: which invalidations and paints
# reach the status bars, and whether each WM_PAINT had an erase.
# Original, AI-assisted (Claude); LGPL-2.1-or-later.
#
# Usage: trace-summary.py inventor-<timestamp>.log [timeline-length]
#
# Prints only message numbers, flags and counts - never status bar text,
# window titles or paths - so the output can be shared.
import re
import sys
from collections import Counter, defaultdict

MSG = {
    0x0005: "WM_SIZE", 0x000b: "WM_SETREDRAW", 0x000f: "WM_PAINT",
    0x0014: "WM_ERASEBKGND", 0x0030: "WM_SETFONT", 0x0085: "WM_NCPAINT",
    0x002b: "WM_DRAWITEM", 0x0318: "WM_PRINTCLIENT", 0x031a: "WM_THEMECHANGED",
    0x0401: "SB_SETTEXTA", 0x0404: "SB_SETPARTS", 0x0407: "SB_SETBORDERS?",
    0x0408: "SB_SETMINHEIGHT", 0x0409: "SB_SIMPLE", 0x040b: "SB_SETTEXTW",
    0x040f: "SB_SETICON", 0x2001: "SB_SETBKCOLOR",
}
# messages that only report or query (not interesting for repainting)
QUIET = {0x0084, 0x0020, 0x0200, 0x0201, 0x0202, 0x02a3, 0x0402, 0x0403,
         0x0406, 0x040c, 0x040d, 0x0413, 0x0414, 0x0087, 0x0281, 0x0282}

sb_re = re.compile(r"^([0-9a-f]+):trace:statusbar:\w+ hwnd ([0-9a-fx]+), msg ([0-9a-f]+)")
rdw_re = re.compile(r"^([0-9a-f]+):trace:win:NtUserRedrawWindow ([0-9a-fx]+) (.*)$")
bp_re = re.compile(r"^([0-9a-f]+):trace:win:NtUserBeginPaint .*fErase = (\d)")


def name(m):
    return MSG.get(m, "0x%04x" % m)


def flags_of(rest):
    f = re.findall(r"RDW_[A-Z]+", rest)
    return " ".join(x[4:] for x in f) or "?"


def main():
    path = sys.argv[1]
    keep = int(sys.argv[2]) if len(sys.argv) > 2 else 60
    hwnds = set()
    # first pass: the status bar windows
    with open(path, errors="replace") as fh:
        for line in fh:
            m = sb_re.match(line)
            if m:
                hwnds.add(m.group(2))
    print("status bar windows:", len(hwnds))

    msgs = Counter()
    redraws = Counter()
    paints = Counter()
    last_sb_msg = defaultdict(lambda: None)   # tid -> last status bar msg
    pending_paint = {}                        # tid -> hwnd awaiting BeginPaint
    timeline = []
    with open(path, errors="replace") as fh:
        for line in fh:
            m = sb_re.match(line)
            if m:
                tid, hwnd, msg = m.group(1), m.group(2), int(m.group(3), 16)
                if msg in QUIET:
                    continue
                msgs[name(msg)] += 1
                last_sb_msg[tid] = msg
                if msg == 0x000f:
                    pending_paint[tid] = hwnd
                timeline.append("%s sb%d %s" % (tid, sorted(hwnds).index(hwnd), name(msg)))
                continue
            m = rdw_re.match(line)
            if m and m.group(2) in hwnds:
                tid, hwnd, rest = m.groups()
                ctx = last_sb_msg[tid]
                key = "%s (after %s)" % (flags_of(rest), name(ctx) if ctx is not None else "-")
                redraws[key] += 1
                timeline.append("%s sb%d   RedrawWindow %s" % (tid, sorted(hwnds).index(hwnd), flags_of(rest)))
                continue
            m = bp_re.match(line)
            if m and m.group(1) in pending_paint:
                tid, erase = m.groups()
                hwnd = pending_paint.pop(tid)
                paints["fErase=%s" % erase] += 1
                timeline.append("%s sb%d   BeginPaint fErase=%s" % (tid, sorted(hwnds).index(hwnd), erase))

    print("\nmessages to the status bars (without queries and mouse):")
    for k, v in msgs.most_common():
        print("  %6d %s" % (v, k))
    print("\nRedrawWindow/InvalidateRect on a status bar, by flags (and the last status bar message on that thread):")
    for k, v in redraws.most_common(25):
        print("  %6d %s" % (v, k))
    print("\nstatus bar WM_PAINT -> BeginPaint result:")
    for k, v in paints.most_common():
        print("  %6d %s" % (v, k))
    print("\nlast %d events:" % keep)
    for t in timeline[-keep:]:
        print("  " + t)


if __name__ == "__main__":
    main()
