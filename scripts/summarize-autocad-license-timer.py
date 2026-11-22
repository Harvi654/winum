#!/usr/bin/env python3
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
"""Match returned Win32 timer IDs with cancellation and delivery."""
from collections import defaultdict
from pathlib import Path
import re
import sys

HEADER = re.compile(r"^\s*(\d+\.\d+):([\da-f]+):([\da-f]+):(.*)$", re.I)
CALL = re.compile(r"^Call\s+user32(?:\.dll)?\.(SetTimer|KillTimer)\(([^)]*)\)", re.I)
RETURN = re.compile(r"^Ret\s+user32(?:\.dll)?\.(SetTimer|KillTimer)\(\) retval=(?:0x)?([\da-f]+)\b", re.I)
DELIVERY = re.compile(r"\bgot type \d+ msg 113 \(WM_TIMER\) hwnd (\S+) wp ([\da-f]+)\b", re.I)
DIALOG = " licensing_dialog caller=0 "


def handle(value):
    return 0 if value.lower() in {"(nil)", "nil"} else int(value, 16)


def summarize(path):
    pending = defaultdict(list)
    active = {}
    count = 0
    set_returns = kill_returns = delivered = cancelled = 0
    with path.open(errors="replace") as stream:
        for line in stream:
            if not (head := HEADER.match(line)):
                continue
            time, pid, tid, body = head.groups()
            tick = float(time)

            if DIALOG in body:
                print("timer_dialog=seen")
                break
            if match := CALL.match(body):
                name, arguments = match.groups()
                try:
                    args = tuple(int(value, 16) for value in arguments.split(","))
                except ValueError:
                    args = ()
                pending[pid, tid, name.lower()].append(args)
            elif match := RETURN.match(body):
                name, value = match.groups()
                name = name.lower()
                result = int(value, 16)
                stack = pending.get((pid, tid, name))
                args = stack.pop() if stack else ()
                if name == "settimer":
                    set_returns += 1
                    if len(args) != 4 or not result:
                        continue
                    hwnd, requested, interval, _ = args
                    timer_id = requested if hwnd else result
                    key = pid, tid if not hwnd else None, hwnd, timer_id
                    if previous := active.pop(key, None):
                        ordinal, start = previous
                        print(f"timer_event=replaced registration={ordinal} after_s={tick-start:.3f} new_ms={interval}")
                    if interval == 120000:
                        count += 1
                        active[key] = count, tick
                        print(f"timer_event=set registration={count} interval_ms={interval}")
                else:
                    kill_returns += 1
                    if len(args) != 2:
                        continue
                    hwnd, timer_id = args
                    key = pid, tid if not hwnd else None, hwnd, timer_id
                    if entry := active.get(key):
                        ordinal, start = entry
                        print(f"timer_event=kill registration={ordinal} success={bool(result)} after_s={tick-start:.3f}")
                        if result:
                            del active[key]
                            cancelled += 1
            elif match := DELIVERY.search(body):
                hwnd, timer_id = map(handle, match.groups())
                key = pid, tid if not hwnd else None, hwnd, timer_id
                if entry := active.get(key):
                    ordinal, start = entry
                    print(f"timer_event=delivery registration={ordinal} after_s={tick-start:.3f}")
                    delivered += 1
        else:
            print("timer_dialog=not_seen")
    print(f"timer_coverage set_returns={set_returns} kill_returns={kill_returns} "
          f"tracked={count} successful_kills={cancelled} deliveries={delivered}")


if __name__ == "__main__":
    logs = sorted(Path(sys.argv[1]).glob("acad-*.log"))
    if not logs:
        print("timer_coverage=no_acad_log")
    for log in logs:
        summarize(log)
