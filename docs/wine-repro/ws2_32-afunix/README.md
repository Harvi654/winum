# MEASURE: AF_UNIX stream sockets in Winsock

Original AI-assisted (Claude), LGPL-2.1-or-later. A public-API observation
program, not a Wine test: it prints what Windows returns and judges
nothing. Its Windows results become the expected values of the Wine test
of the candidate `ws2_32-af-unix` (test first; no patch exists yet).

## Why

ABB RobotStudio 2026 (Harvi654/ABB-RobotStudio-on-Linux,
`docs/robotstudio-2026-first-try.md`, steps 7c-8c) ships Wibu-Systems'
CodeMeter runtime 9.10. Its service starts an "event network server" on
an AF_UNIX stream socket at
`C:\ProgramData\CodeMeter/Sockets/wibu-codemeter-runtime-events`, and
CmLogger opens one too. Under Wine 11.22 every
`WSASocketW(AF_UNIX, SOCK_STREAM, 0, ..., WSA_FLAG_OVERLAPPED)` fails with
`WSAEAFNOSUPPORT` (10047), CodeMeter logs "Could not start to listen for
incoming connections on endpoint ... Unknown error (10047)", and the
socket file that exists on Windows never appears. RobotStudio itself
talks to CodeMeter over TCP and then quits (exit code 103); whether that
is because of the missing event server is not shown yet.

Windows 10 (1803 and later) and 11 support AF_UNIX stream sockets in
Winsock. Wine 11.22 has the header (`include/afunix.h`, `SOCKADDR_UN`) but
no implementation: `init_socket()` in `server/sock.c` maps only INET,
INET6, IPX, IRDA and Bluetooth and answers other families with
`WSAEAFNOSUPPORT`.

## What it measures

In a new folder under `%TEMP%` (printed as `<dir>`; removed at the end):

- the Winsock catalogue entries for AF_UNIX (`WSAEnumProtocolsW`);
- `socket()` for stream, datagram, seqpacket, stream+TCP;
  `WSASocketW(..., WSA_FLAG_OVERLAPPED)`; `SO_TYPE`, `SO_PROTOCOL_INFOW`;
  `getsockname` before bind;
- bind to a path: the file it creates (attributes, reparse tag, size),
  `getsockname` (length, family, path), binding the same path again,
  connect before listen, listen, `select` on the listener, connect,
  accept (address), `getpeername`/`getsockname` on both ends, send/recv
  both ways, shutdown, close; the file after close, connect after the
  listener is closed, binding the old path again, deleting the file;
- errors: connect to a missing path, to a regular file; bind on a
  regular file, in a missing directory;
- address forms: family only (empty path), abstract (leading NUL), full
  structure length, length without the NUL, forward slashes (as
  CodeMeter's path has), a path relative to the current directory;
- non-blocking: accept with nothing pending, connect, `select` for
  writable, accept after connect;
- extensions: `AcceptEx`, `GetAcceptExSockaddrs`, `ConnectEx` pointers;
  `AcceptEx` on an AF_UNIX listener with a connecting client, its
  completion, `SO_UPDATE_ACCEPT_CONTEXT`, the addresses it returns;
  `ConnectEx` on an unbound socket.

## Build and run

```sh
bash scripts/build-measure-ws2_32-afunix.sh /path/to/NEW-bundle
```

Requires the MinGW-w64 GCC for x86_64 and i686. Copy the whole bundle
into a new folder in the **offline** Windows 10 VM snapshot (AGENTS.md:
measurements only there) and run `run-measure.bat` as a normal user. It
refuses to overwrite existing results. Return only:

- `result-x86_64.txt`, `result-i686.txt`;
- `exitcodes.txt`.

Exit 0 means every observation ran; 2 means a setup step failed and the
result is incomplete. The program prints no user names or profile paths
(only `<dir>` and the length of the full socket path) and leaves no
files behind.

## Results

- Wine 11.22 (winum-11.22-1), 2026-11-17, before any patch:
  [`wine-11.22-1-2026-11-17/`](wine-11.22-1-2026-11-17/) - no AF_UNIX
  catalogue entry, every AF_UNIX `socket()`/`WSASocketW()` fails with
  10047 (seqpacket 10044), so nothing after it can be measured; exit 2.
- Windows 10 22H2 (10.0.19045), offline VM, 2026-11-17:
  [`windows10-19045-2026-11-17/`](windows10-19045-2026-11-17/) - complete
  (exit 0 for x86_64 and i686), both architectures give the same values.

### Windows 10 in short

| Observation | Windows 10 |
| --- | --- |
| catalogue | one AF_UNIX entry: `SOCK_STREAM`, protocol 0, `dwServiceFlags1` 0x20026, `dwProviderFlags` 0x8, `iMaxSockAddr` 110, `iMinSockAddr` 2, version 2, name `AF_UNIX` |
| socket kinds | stream ok (also `WSASocketW` overlapped); datagram 10047 (`WSAEAFNOSUPPORT`); seqpacket 10044; stream + `IPPROTO_TCP` 10043 (`WSAEPROTONOSUPPORT`) |
| options | `SO_TYPE` 1; `SO_PROTOCOL_INFOW` as the catalogue entry |
| `getsockname` before bind | -1, 10022 (`WSAEINVAL`) |
| bind | creates the file: attributes 0x420 (archive + reparse point), reparse tag 0x80000023 (`IO_REPARSE_TAG_AF_UNIX`), size 0 |
| `getsockname` after bind | length 2 + strlen + 1, the path as passed (`/` kept, relative kept) |
| bind again on the same path | 10048 (`WSAEADDRINUSE`), also after the listener is closed and on a regular file |
| connect | before listen, after close, to a missing path, to a regular file: 10061 (`WSAECONNREFUSED`) |
| bind in a missing directory | 10050 (`WSAENETDOWN`) |
| accept | address length 110, empty path; client `getsockname` 110/empty, client `getpeername` the server path; accepted socket: `getpeername` 110/empty, `getsockname` the server path |
| data | send/recv both ways; recv after the peer's `shutdown(SD_SEND)` 0 |
| file after close | stays (same reparse point); `DeleteFile` removes it |
| address forms | family only (length 2) ok, `getsockname` 110/empty; abstract (leading NUL) ok; full struct length ok (`getsockname` still strlen-based); length without NUL ok; forward slashes ok (file at the backslash path); relative to the current directory ok |
| non-blocking | accept with nothing pending 10035; connect -1/10035 (`WSAEWOULDBLOCK`), then `select` writable; accept succeeds |
| AcceptEx | pointer available; call pending (997), completes when a client connects, 0 bytes; `SO_UPDATE_ACCEPT_CONTEXT` ok; local address length 110 with the path, remote 110 empty |
| ConnectEx | pointer available; on an unbound socket fails with 10022 |

Wine 11.22 also already has upstream tests for AF_UNIX
(`dlls/ws2_32/tests/sock.c`, `test_afunix` and `test_afunix_path`, added
upstream on 2025-10-30 as "ws2_32/tests: Add some tests for AF_UNIX
sockets"); they `todo_wine win_skip` when the socket cannot be created,
so the implementation is the missing part upstream as well.
