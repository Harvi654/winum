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
- Windows 10: pending.
