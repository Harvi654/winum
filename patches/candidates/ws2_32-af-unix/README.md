# Candidate: AF_UNIX stream sockets (ws2_32, server)

Work in progress, not in a normal build. `series` is empty: test first.

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
