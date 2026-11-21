#!/usr/bin/env python3
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
"""Summarize AutoCAD's Winsock activity on licensing-service sockets."""

import argparse
from collections import Counter, defaultdict
from ipaddress import ip_address
from pathlib import Path
import re


HEADER = re.compile(r"^\s*(\d+)\.(\d{3}):([\da-f]+):([\da-f]+):(.*)$", re.I)
WINSOCK = re.compile(r"^trace:winsock:(\w+)\s+(.*)$")
RELAY = re.compile(r"^(Call|Ret)\s+ws2_32(?:\.dll)?\.(\w+)\((.*)$", re.I)
RETURN = re.compile(r"^\) retval=(?:0x)?([\da-f]+)\b", re.I)
SOCKET = re.compile(r"^(?:socket\s+)?(?:0x)?([\da-f]+)\b", re.I)
ADDRESS = re.compile(r"\baddress ([^, ]+),[^}]*\bport (\d+)\b")
STATUS = re.compile(r"\bstatus (0x[\da-f]+|0)\b", re.I)
SERVICE_PID = re.compile(r"^service_windows_pid=((?:0x[\da-f]+,?)+)$", re.M | re.I)
CALLER = " licensing_dialog caller=0 "
CONNECT = {"connect", "WS2_ConnectEx"}
SOCKET_STATUS = CONNECT | {"WS2_recv_base", "WS2_sendto", "WSAGetOverlappedResult", "shutdown", "WS2_DisconnectEx"}


def records(path):
    with path.open(encoding="utf-8", errors="replace") as stream:
        for line in stream:
            if match := HEADER.match(line.rstrip()):
                seconds, milliseconds, pid, tid, message = match.groups()
                yield 1000 * int(seconds) + int(milliseconds), int(pid, 16), int(tid, 16), message


def listen_ports(paths, service_pids):
    ports = set()
    for path in paths:
        pending, bound = {}, {}
        for _, pid, tid, text in records(path):
            if pid not in service_pids or not (match := WINSOCK.match(text)):
                continue
            api, body = match.groups()
            if api not in {"bind", "listen"}:
                continue
            thread = pid, tid, api
            if sock := SOCKET.match(body):
                pending[thread] = int(sock[1], 16)
            elif thread in pending:
                key = pid, pending.pop(thread)
                if api == "bind" and (addr := ADDRESS.search(body)) and body.startswith("successfully bound"):
                    bound[key] = int(addr[2])
                elif api == "listen" and key in bound and (status := STATUS.search(body)) and int(status[1], 0) == 0:
                    ports.add(bound[key])
    return ports - {0}


def loopback(address):
    try:
        ip = ip_address(address)
    except ValueError:
        return False
    return (getattr(ip, "ipv4_mapped", None) or ip).is_loopback


def relay_result(api, args, result):
    if api == "wsagetoverlappedresult":
        return "success" if result else "false"
    if result == 0xffffffff:
        return "socket_error"
    if api == "recv":
        if result:
            return "bytes_read"
        return "eof" if len(args) > 2 and int(args[2], 16) else "zero_length_read"
    if api == "send" and result:
        return "bytes_sent"
    return "success" if result == 0 else f"return_0x{result:x}"


def client_events(path, pid, dialog_tick, ports):
    sockets, calls, pending = {}, defaultdict(list), {}
    counts, last_time = Counter(), {}
    next_socket = relay_calls = 0

    def add(tick, sock, api, outcome):
        if not sock:
            return
        delta = ((tick - dialog_tick + 2**31) % 2**32 - 2**31) / 1000
        if delta <= 15:
            phase = "earlier" if delta < -15 else "before_15s" if delta < 0 else "after_15s"
            key = sock, phase, api, outcome
            counts[key] += 1
            last_time[key] = delta

    for tick, owner, tid, text in records(path):
        if owner != pid:
            continue
        if match := WINSOCK.match(text):
            api, body = match.groups()
            if api not in SOCKET_STATUS:
                continue
            thread = tid, api
            if arg := SOCKET.match(body):
                handle = int(arg[1], 16)
                if api in CONNECT:
                    if (addr := ADDRESS.search(body)) and int(addr[2]) in ports and loopback(addr[1]):
                        if handle not in sockets:
                            next_socket += 1
                            sockets[handle] = next_socket
                        add(tick, sockets[handle], api, "connect_attempt")
                    elif addr:
                        sockets.pop(handle, None)
                pending[thread] = sockets.get(handle)
            elif (status := STATUS.search(body)) and thread in pending:
                add(tick, pending.pop(thread), api, f"status_0x{int(status[1], 0):x}")
            continue
        if not (match := RELAY.match(text)):
            continue
        phase, api, body = match.groups()
        api = api.lower()
        thread = tid, api
        if phase == "Call":
            args = body.partition(")")[0].split(",")
            if not all(re.fullmatch(r"(?:0x)?[\da-f]+", arg.strip(), re.I) for arg in args):
                continue
            relay_calls += 1
            handle = int(args[0], 16)
            calls[thread].append((handle, sockets.get(handle), args))
        elif (result := RETURN.match(body)) and calls[thread]:
            handle, sock, args = calls[thread].pop()
            value = int(result[1], 16) & 0xffffffff
            if api in {"connect", "wsaconnect"}:
                sock = sockets.get(handle)
            add(tick, sock, api, relay_result(api, args, value))
            if api == "closesocket" and value == 0:
                sockets.pop(handle, None)
    print(f"client_relay_calls={relay_calls} matched_socket_attempts={next_socket}")
    for key, count in sorted(counts.items(), key=lambda item: (item[0][0], last_time[item[0]], item[0])):
        socket, phase, api, outcome = key
        print(f"socket_event socket={socket} phase={phase} api={api} result={outcome} "
              f"count={count} last_from_dialog={last_time[key]:+.3f}s")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace_directory", type=Path)
    run = parser.parse_args().trace_directory
    matches = [(path, pid, tick) for path in sorted(run.glob("acad-*.log"))
               for tick, pid, _, text in records(path) if text.startswith("trace:taskdialog:") and CALLER in text]
    if len(matches) != 1:
        print(f"client_relay_dialog=unresolved entries={len(matches)}")
        return
    path, pid, tick = matches[0]
    match = SERVICE_PID.search((run / "service-summary.txt").read_text(errors="replace"))
    service_pids = {int(part, 16) for part in match[1].split(",") if part} if match else set()
    logs = sorted(run.glob("*.log"))
    if (run / "sc-start.txt").is_file():
        logs.append(run / "sc-start.txt")
    ports = listen_ports(logs, service_pids)
    print(f"client_relay_dialog=found service_processes={len(service_pids)} service_listeners={len(ports)}")
    client_events(path, pid, tick, ports)


if __name__ == "__main__":
    main()
