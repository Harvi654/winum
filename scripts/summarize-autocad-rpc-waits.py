#!/usr/bin/env python3
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
"""Count RPC errors and wait results immediately before the AutoCAD dialog."""

from collections import Counter, defaultdict, deque
from pathlib import Path
import re
import sys


HEADER = re.compile(r"^\s*(\d+)\.(\d{3}):([a-f\d]+):([a-f\d]+):(.*)$", re.I)
RELAY = re.compile(r"^(Call|Ret)\s+(kernelbase|user32|rpcrt4)(?:\.dll)?\.(\w+)\((.*)$", re.I)
RETURN = re.compile(r"^\) retval=(?:0x)?([a-f\d]+)\b", re.I)
RPC = re.compile(r"^(warn|err):rpc:(\w+)\b")
WINDOW_MS = 12000


def summarize(log):
    calls = defaultdict(list)
    events = deque()
    with log.open(encoding="utf-8", errors="replace") as stream:
        for line in stream:
            if not (header := HEADER.match(line)):
                continue
            seconds, millis, pid, tid, message = header.groups()
            tick = int(seconds) * 1000 + int(millis)
            while events and tick - events[0][0] > WINDOW_MS:
                events.popleft()
            if message.startswith("trace:taskdialog:") and " licensing_dialog caller=0 " in message:
                return tick, pid, tid, events
            if rpc := RPC.match(message):
                level, name = rpc.groups()
                events.append((tick, pid, tid, "rpc", name, level, None))
            elif relay := RELAY.match(message):
                action, dll, name, detail = relay.groups()
                key = pid, tid, dll.casefold(), name
                if action.lower() == "call":
                    calls[key].append(tick)
                elif result := RETURN.match(detail):
                    duration = (tick - calls[key].pop()) % 2**32 if calls.get(key) else None
                    if calls.get(key) == []:
                        del calls[key]
                    code = int(result[1], 16) & 0xffffffff
                    kind = "rpc_return" if dll.casefold() == "rpcrt4" else "wait"
                    events.append((tick, pid, tid, kind, name, f"0x{code:08x}", duration))
    return None


def main(run):
    dialogs = [result for log in sorted(run.glob("acad-*.log")) if (result := summarize(log))]
    if len(dialogs) != 1:
        print(f"rpc_wait_dialog=unresolved log_matches={len(dialogs)}")
        return
    tick, pid, tid, events = dialogs[0]
    counts, longest, last = Counter(), {}, {}
    for at, process, thread, kind, api, result, elapsed in events:
        offset = ((at - tick + 2**31) % 2**32) - 2**31
        if process != pid or not -WINDOW_MS <= offset <= 0:
            continue
        key = ("dialog" if thread == tid else "other", kind, api, result)
        counts[key] += 1
        if elapsed is not None:
            longest[key] = max(elapsed, longest.get(key, 0))
        last[key] = offset
    print(f"rpc_wait_dialog=found window_ms={WINDOW_MS} observed_events={sum(counts.values())}")
    for key, count in sorted(counts.items(), key=lambda row: (row[0][0] != "dialog", -last[row[0]], row[0])):
        thread, kind, api, result = key
        print(f"rpc_wait_event thread={thread} kind={kind} api={api} result={result} "
              f"count={count} last_ms={last[key]} max_wait_ms={longest.get(key, '-')}")


if __name__ == "__main__":
    main(Path(sys.argv[1]))
