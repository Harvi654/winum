# Candidate: AF_UNIX stream sockets (ws2_32, server)

Work in progress, not in a normal build. `series` has the test patch
only (`ws2_32-tests-afunix-winum.patch`): test first, no fix yet.

## Problem as the user sees it

ABB RobotStudio 2026 (Harvi654/ABB-RobotStudio-on-Linux, first-try log,
steps 7c-8c): RobotStudio and its Support Tool open a white window and
exit with code 103. They are protected with Wibu-Systems' CodeMeter; the
CodeMeter 9.10 service logs "The event network server could not accept an
incoming connection ... Could not start to listen for incoming
connections on endpoint C:\ProgramData\CodeMeter/Sockets/
wibu-codemeter-runtime-events. Internal error: Unknown error (10047)".
The socket file exists on Windows (comparison with the maintainer's
Windows 10 VM) and never appears under Wine. Whether the missing event
server is the reason for exit 103 is not shown yet; the Winsock gap is.

## Cause in Wine

`WSASocketW(AF_UNIX, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)` fails
with `WSAEAFNOSUPPORT` (10047): `server/sock.c` `init_socket()` maps only
INET, INET6, IPX, IRDA and Bluetooth (`get_unix_family`), and
`sockaddr_to_unix()` / `sockaddr_from_unix()` know no `sockaddr_un`. Wine
11.22 ships `include/afunix.h` but no implementation, and no AF_UNIX entry
in the Winsock catalogue (`WSAEnumProtocols`).

## Plan

1. **Measure Windows** (done 2026-11-17, Windows 10 22H2, complete; see
   the table in `docs/wine-repro/ws2_32-afunix/README.md`): `docs/wine-repro/ws2_32-afunix/` - catalogue
   entry, socket kinds, bind/connect/accept, the socket file, addresses
   returned, error codes, address forms, non-blocking, AcceptEx/ConnectEx
   (CodeMeter's server looks like a Boost.Asio local stream acceptor on
   IOCP, i.e. AcceptEx).
2. **Wine test**: Wine 11.22 already has upstream AF_UNIX tests in
   `dlls/ws2_32/tests/sock.c` (`test_afunix`, `test_afunix_path`, added
   2025-10-30), which skip with `todo_wine` while Wine has no AF_UNIX.
   The candidate adds what they do not cover and what CodeMeter uses,
   with the measured values: the catalogue entry, socket kinds and
   options, the error codes, the address forms (mixed `/` and `\`),
   non-blocking connect/accept, AcceptEx and ConnectEx; `todo_wine` where
   Wine fails; L1 on Windows 10 x86_64 and i686.
3. **Fix**, as small as the test allows: the family and `sockaddr_un`
   translation (Windows path -> Unix path of the prefix through the
   server's/ntdll's DOS-to-Unix name translation, which a socket address
   does not get today), bind creating the socket file where Windows
   creates it, connect, accept/AcceptEx addresses, the catalogue entry.
   Open design questions: how the Windows path reaches a Unix path
   (client side in ws2_32/ntdll vs. server), what `getsockname` returns
   (the Windows path, as measured), behaviour of the file after close.
4. Validation L0-L4 as AGENTS.md says; L4 in RobotStudio (CodeMeter's
   event server starts, and whether exit 103 goes) and a smoke test in
   Inventor (ws2_32 is shared).

Original code (AI-assisted, Claude); not derived from earlier upstream
AF_UNIX implementation work, which was not merged (only the tests were,
2025-10-30). A second model will be asked whether it resembles
a known project before it goes further.

## State (2026-11-17)

- `ws2_32-tests-afunix-winum.patch` adds `test_afunix_winum()` to
  `dlls/ws2_32/tests/sock.c` (after the upstream `test_afunix`, which is
  unchanged). It applies to Wine 11.22 with `--fuzz=0` (checked against
  `dlls/ws2_32/tests/sock.c` of winum-11.22-1's source archive). The
  same code is `docs/wine-repro/ws2_32-afunix/test-body.h`;
  `scripts/build-l1-ws2_32-afunix.sh` checks that both are identical and
  builds `l1.c` (a native harness with Wine's ok/skip/todo_wine
  semantics) for x86_64 and i686.
- Under unpatched winum-11.22-1 the native build gives
  `checks=12 failures=0 todos=7 skipped=5` on both architectures: the
  socket and catalogue checks are todo, the rest skips.
- **L1 passed** on Windows 10 22H2 (19045), offline VM, 2026-11-17, with
  the bundle of commit 1d030a4: `checks=138 failures=0 todos=0
  skipped=0`, exit 0, x86_64 and i686
  (`docs/wine-repro/ws2_32-afunix/l1-windows10-19045-2026-11-17/`).
- L0/L2 (the test inside a Wine build, paired run): with the fix.
- Next: the fix (step 3 of the plan).

## Design of the fix (draft, before code)

Where Wine 11.22 handles a socket: `ws2_32` (PE) turns calls into AFD
ioctls on `\Device\Afd`; `ntdll/unix/socket.c` handles the data ioctls;
`server/sock.c` creates the Unix socket (`init_socket`), binds, connects,
accepts and converts addresses (`sockaddr_to_unix`, `sockaddr_from_unix`).
The server cannot turn a Windows path into a Unix path (drive mapping and
case folding live in ntdll's `nt_to_unix_file_name`). So:

1. **Family**: `server/sock.c` maps `WS_AF_UNIX` to `AF_UNIX` for
   `SOCK_STREAM`, protocol 0 only (datagram stays `WSAEAFNOSUPPORT`,
   `IPPROTO_TCP` becomes `WSAEPROTONOSUPPORT`, as measured).
2. **Path translation in ntdll**: for `IOCTL_AFD_BIND` and the connect
   ioctl on an AF_UNIX socket, `ntdll/unix/socket.c` resolves the
   Windows path (relative to the current directory, `/` and `\` alike)
   to the Unix path of the prefix and hands the server both: the Unix
   path for `bind()`/`connect()`, the Windows address as given for
   `getsockname`/`getpeername`. Family-only and abstract names: autobind
   / Linux abstract namespace.
3. **Addresses**: the server keeps the Windows address of a bound socket
   and of a connect target; `accept`, the client's `getsockname` and the
   accepted socket's `getpeername` return the family with an empty path
   and length `sizeof(SOCKADDR_UN)` (110), as measured; AcceptEx's
   addresses the same way.
4. **Errors**: `EADDRINUSE` -> `WSAEADDRINUSE` (also for an existing file,
   which Linux reports as `EADDRINUSE`), `ENOENT`/`ECONNREFUSED` on connect
   -> `WSAECONNREFUSED`, `ENOENT` on bind (missing directory) ->
   `WSAENETDOWN` (measured, unusual but what Windows returns).
5. **The socket file**: Linux leaves the socket inode after close, as
   Windows leaves its file. ntdll reports an inode with `S_ISSOCK` as a
   reparse point with `IO_REPARSE_TAG_AF_UNIX`, attributes archive +
   reparse point, size 0 (`FindFirstFile`, `GetFileAttributes`), so no
   extra data is stored. `DeleteFile` already unlinks it.
6. **Catalogue**: one `WSAPROTOCOL_INFOW` entry for AF_UNIX in `ws2_32`
   (`dwServiceFlags1` 0x20026, `dwProviderFlags` 0x8, address 2..110,
   version 2, name `AF_UNIX`), also returned by `SO_PROTOCOL_INFOW`.

Probably more than one patch (server, ntdll, ws2_32), each with its part
of the test's todo_wine removed. Built and checked in GitHub Actions on
this topic branch (a workflow like `build-statusbar-v2.yml`: L0 build and
the ws2_32 tests), then L2 on a `test-run/` branch.
