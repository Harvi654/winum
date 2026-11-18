# Candidate: AF_UNIX stream sockets (ws2_32, server)

Work in progress, not in a normal build. `series`: the test patch, then
the fix in three parts (server, ntdll, ws2_32).

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

## Fix v1 (2026-11-17)

- `server-afunix-stream-sockets.patch` - `server/sock.c`, `server/fd.c`,
  `include/wine/afd.h` (`struct afd_unix_addr`).
- `ntdll-afunix-paths-reparse.patch` - NT path -> Unix path for bind and
  connect; socket inodes reported as `IO_REPARSE_TAG_AF_UNIX` reparse
  points.
- `ws2_32-afunix-stream-sockets.patch` - catalogue entry; bind, connect
  and ConnectEx pass path addresses as NT paths; the test's todo_wine go.

Differences from the draft design: no new ioctl; a path address travels
as `struct afd_unix_addr` (longer than a SOCKADDR_UN, so it cannot be
taken for one), the NT path is worked out in ws2_32 (current directory
of the PE side, also under WoW64), the Unix path in ntdll. The socket
file needs no stored data: a socket inode is the reparse point; it opens
only with `FILE_OPEN_REPARSE_POINT` (O_PATH on Linux), which is what
`DeleteFile` uses.

Checks so far (local, x86_64 only, container without IPv6):

- L0-ish: server, ntdll.so, ws2_32 build without warnings; the whole
  series applies to winum-11.22-1's source with `--fuzz=0`.
- `ws2_32_test sock`: `test_afunix_winum` and the upstream `test_afunix`
  / `test_afunix_path` (relative, absolute and `\??\` NT paths, socket
  file attributes, `CreateFile` with and without
  `FILE_FLAG_OPEN_REPARSE_POINT`, `FSCTL_GET_REPARSE_POINT`, `DeleteFile`)
  pass with no failures.
- Paired run, test patch alone vs. whole candidate, same build tree:
  `ws2_32_test sock`, `afd`, `protocol`, `ntdll_test file`,
  `kernel32_test file` have identical failing lines (all pre-existing,
  IPv6/ICMP/network in the container); `sock` has 8 todos fewer.
- Next: the workflow `test-afunix.yml` on this branch (both
  architectures, test patch alone vs. candidate), cross review by a
  second assistant, L4 in RobotStudio (CodeMeter's event server) and a
  smoke test in Inventor (ws2_32, ntdll and the server are shared).

## CI, run 44009624034 (2026-11-17, commit e50aea0)

Both architectures, patches/series + test patch alone (`tests`) vs. the
whole candidate (`candidate`), same workflow; failures per unit (the
exit code of the test program):

| Unit | tests | candidate |
| --- | --- | --- |
| ws2_32 afd, x86_64 / i386 | 0 / 0 | 0 / 0 |
| ws2_32 protocol, x86_64 / i386 | 0 / 0 | 0 / 0 |
| ws2_32 sock, x86_64 / i386 | 5 / 5 | 5 / 5 |
| ntdll file, x86_64 / i386 | 8 / 8 | 8 / 8 |
| kernel32 file, x86_64 / i386 | 7 / 7 | 7 / 7 |

No unit has more failures with the fix; in the candidate run the AF_UNIX
tests are not skipped but run, and add none. (The run's summary lines
were not parsed, because Wine prefixes them with a process id; the
script now lists the failing lines per variant too.) The artifact
`afunix-candidate` holds `winum-11.22-1-afunix-x86_64.tar.xz` for L4.

## Fix v2 (2026-11-17)

L4 of v1 in RobotStudio: CodeMeter's socket binds, then
`SetNamedSecurityInfoA` on the socket file fails (1920). Measured on
Windows (MEASURE 2): `CreateFile` without the reparse flag fails there
too, but Get/SetNamedSecurityInfo work on the socket file. v2:

- new `advapi32-security-unhandled-reparse.patch`: `get_security_file`
  opens the file again with `FILE_OPEN_REPARSE_POINT` when the normal
  open fails with `STATUS_IO_REPARSE_TAG_NOT_HANDLED` (symbolic links
  and mount points unchanged);
- server patch v2: `file_set_sd` changes the mode of a socket file open
  with O_PATH through `/proc/self/fd` (fchmod rejects O_PATH);
- test patch v2: `test_afunix_security` (Get/SetNamedSecurityInfoA,
  connect afterwards) with the measured values; `advapi32` imported;
- ws2_32 patch v2: one `todo_wine` - Wine reports 3 ACEs after setting
  a 1-ACE DACL, because it keeps a socket file's DACL as mode bits (no
  xattr on a socket inode); the access CodeMeter grants (Everyone, full)
  becomes mode 0777, which is what connecting needs.

Local checks (x86_64): the series applies with `--fuzz=0` to
winum-11.22-1's source; `ws2_32 sock` has no failure in the AF_UNIX
tests (328 pre-existing elsewhere, as before), one todo more;
`advapi32 security` has the same 6 failing lines with and without the
advapi32 patch; `ntdll file` 8 and `kernel32 file` 7 as before. The
workflow adds `advapi32:security` to its paired run.

- **L1 passed** for the v2 test body (commit 8abb414), Windows 10 22H2
  (19045), offline VM, 2026-11-17: `checks=150 failures=0 todos=0
  skipped=0`, exit 0, x86_64 and i686
  (`docs/wine-repro/ws2_32-afunix/l1-v2-windows10-19045-2026-11-17/`).
- **Paired CI run of v2** (run 44062842130, commit 818bba9, both
  architectures): the candidate fails exactly the lines the test-only
  build fails - `ws2_32 sock` 13774, 13777, 14270, 14338, 14622;
  `ntdll file` 5041, 5045, 6049, 6064, 6068, 6073; `kernel32 file` 371,
  4297, 4299, 4315, 4326, 6323, 6324; `advapi32 security` 3999, 4008,
  4018, 4051 - none of them in AF_UNIX code. The test-only build also
  failed `sock.c` 4597 and 4598 on x86_64 (`test_select`, a TCP pair
  after filling the send buffer; timing, not AF_UNIX), the candidate did
  not. `ws2_32 sock` runs 389 todos and skips 4 with the fix against
  397 and 10 without: the AF_UNIX tests run instead of skipping. The
  job packed `winum-11.22-1-afunix-x86_64` (CANDIDATE.txt names 818bba9)
  for L4.
- **L4 of v2 in RobotStudio** (maintainer's machine, 2026-11-17, artifact
  of run 44062842130): CodeMeter 9.10's event server binds its socket and
  the service log has no error any more ("Server ready"); before the
  candidate it logged 10047, with v1 `SetNamedSecurityInfoA failed`. The
  socket file has mode `srwxrwxr-x`. RobotStudio itself still exits with
  103 (CodeMeter-protected start), so the candidate fixes CodeMeter's
  event server but not, alone, the program's start. AGENTS.md: a patch
  that does not fix the user's problem is not added - the candidate stays
  here until the cause of 103 is known and shows whether the event server
  is needed for it (Harvi654/ABB-RobotStudio-on-Linux,
  `docs/robotstudio-2026-first-try.md`).
- Still open before any promotion: cross review; L4 in Inventor (smoke
  test, the server and ntdll parts touch every file open).

## Fix v3 (2026-11-17)

L4 of v2 showed a second problem on every start after the first:
CodeMeter removes the socket file of its previous run before it binds,
and that failed ("remove: Invalid parameter"), so its event server
stopped. MEASURE 3 (`measure-del.c`): on Windows `DeleteFile`,
`FileDispositionInfo`, `FileDispositionInfoEx` and delete-on-close all
remove a socket file, with or without a listener, and `MoveFileEx`
renames it; under v2 the two disposition calls failed with
`ERROR_INVALID_PARAMETER` because `set_fd_disposition()` in
`server/fd.c` refuses anything but regular files and directories.

- test patch v3: `test_afunix_delete` with the measured values;
- server patch v3: `set_fd_disposition` also deletes a socket file
  (`S_ISSOCK`), which is unlinked on close as a regular file is.

Local checks (x86_64): the series applies with `--fuzz=0`; without the
server change `test_afunix_delete` fails 7 times (error 87, the file
stays), with it the AF_UNIX tests pass (`ws2_32 sock` keeps its 328
failures outside AF_UNIX in this sandbox, as before).

- **Paired CI run of v3** (run 44080539901, commit c11a4f8, both
  architectures): the candidate fails exactly the lines the test-only
  build fails (`ws2_32 sock` 13774, 13777, 14270, 14338, 14622; `ntdll
  file` 6, `kernel32 file` 7, `advapi32 security` 4 lines, as in v2).
  Artifact `afunix-candidate` 14330808278 (CANDIDATE.txt names c11a4f8)
  for L4.
- **L1 passed** for the v3 test body: `checks=169 failures=0`, x86_64
  and i686 (`docs/wine-repro/ws2_32-afunix/l1-v3-windows10-19045-2026-11-17/`).
- **L4 of v3** (maintainer's machine, 2026-11-17, artifact of run
  44080539901, RobotStudio prefix `try-11-winmgmt`): two prefix starts in
  a row with the wineserver kept alive (`wineserver -p`), without
  removing the socket file in between. Both times CodeMeter 9.10 creates
  a new socket file (16:13:03, then 16:13:44 - the old one is removed,
  as on Windows) and logs "Server ready" with no error. CodeMeter's event
  server works at every start. RobotStudio itself still exits with 103
  for another reason (Harvi654/ABB-RobotStudio-on-Linux, steps 9-12), so
  the user's problem is not solved by this candidate alone (AGENTS.md:
  it stays a candidate until the program needs nothing else, or the
  maintainer decides).
- Open: cross review of the five patches by a second assistant; L4 in
  Inventor (the server and ntdll parts touch every file open).
- Also part of the combined build of `test-run/robotstudio-codemeter`
  (run 44113630473, with the two iphlpapi candidates): no new failure in
  the paired run. With that build RobotStudio connects to the Windows
  CmLogger's AF_UNIX event socket and exchanges messages with it
  (2026-11-17); it still exits with 103 for reasons outside this
  candidate (Harvi654/ABB-RobotStudio-on-Linux, steps 13-16).

## v4 review (2026-11-18)

The [Windows measurements](../../../docs/wine-repro/ws2_32-afunix/v4.md)
use Windows 10 19045.6466 in the offline VM on x86_64 and i686.
The first v4 revision makes three changes:

- Preserve the AF_UNIX reparse tag in handle metadata in ntdll.
- Keep named-path conversion errors distinct from raw address
  pass-through in ws2_32.
- Validate short AF_UNIX connect addresses in the server, preserve
  the client's local bind, and complete ConnectEx through the pending
  path with a full-sized peer address.

The security fallback is unchanged because named DACL operations on
the private synthetic tag succeeded on both Windows and v3. Wine
still represents a socket inode's DACL through its POSIX mode.

Incoming connections from explicitly bound clients still return an
unnamed peer to the listener. Windows returns the original client
spelling, including a relative path, for both accept and getpeername.
Linux can report the resolved path used at bind, which may be a
temporary /proc/self/fd path. A lookup by pathname alone can also
select the wrong socket when a name is unlinked and reused. Both
assertions stay todo pending a design that associates the name with
the actual peer. The unbound host-client probe for SpaceMouse uses a
different path and passes on v3. Repeat it with v4 before hardware L4.

**L1 passed** for v4 (commit `c3fc029`, Windows 10 offline VM,
2026-11-18). Both x86_64 and i686 ran 225 checks with zero failures,
skips or todos and exit 0. See the [L1 record and bundle](../../../docs/wine-repro/ws2_32-afunix/v4.md#l1-on-the-offline-windows-10-vm-2026-11-18).

Wine 11.22 L0, paired L2 and native host transport are tracked in
[run 44398256194](https://github.com/Harvi654/winum/actions/runs/44398256194)
on `test-run/ws2_32-af-unix-v4-1`. L4 and independent review are open.
The candidate is outside the production series.
