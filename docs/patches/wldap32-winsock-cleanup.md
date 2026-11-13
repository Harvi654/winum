# wldap32-winsock-cleanup.patch

*Status page. Levels as in winum's AGENTS.md. Candidate: not in
`patches/series` on `main`; the `winum-test` marker is added after L1.*

- **DLL:** `wldap32` (bundled OpenLDAP in `libs/ldap`); test in `ws2_32`
- **Origin:** Autodesk-Inventor-for-Linux, AutoCAD 2027 experiment, 2026-11-13
- **Candidate:** `patches/candidates/wldap32-winsock-cleanup/`

## Problem as the user saw it

AutoCAD 2027 opens its main window and a drawing, then "The License manager
is not functioning or is improperly installed. AutoCAD will shut down now."
No sign-in window appears.

## Cause and fix

AutoCAD's sign-in component cannot create a socket: WSANOTINITIALISED
(10093). AutoCAD loads and unloads `wldap32.dll` eight times; each unload
runs libldap's destructor, which calls `WSACleanup()` although libldap
never called `WSAStartup()` (it does so only when LDAP is initialised). The
process's WinSock count reaches 0. The patch makes the destructor call
`WSACleanup()` only after libldap's own successful `WSAStartup()`.

## How it was found

`WINEDEBUG=+pid,+winsock` (startup count per process, the first
`WSASocketW not initialised`), then `RelayInclude` limited to
`ws2_32.WSAStartup;ws2_32.WSACleanup` with `+pid,+relay,+loaddll` to see
which module calls them. Commands and results in the candidate README.

## Wine test

`test_WSAStartup_dll_unload` in `dlls/ws2_32/tests/sock.c`.

## Validation

| Level | Result |
| --- | --- |
| L0 | builds with the full series, both architectures: Build Wine run 43390923029 on `topic/wldap32-winsock-cleanup`, 2026-11-13 |
| L1 | pending (Windows 10 x86_64 and i686, `ws2_32_test sock`) |
| L2 | paired, x86_64 and i386, 2026-11-13: `test-run/wldap32-winsock-cleanup-1` (run 43390944991), ws2_32 and wldap32, 10 units, 0 regressions (the existing `sock.c` failures moved by +36 lines; the new test passes). The run's g2-g5 jobs failed at once because the branch's workflow edit left their matrix entries without a variant; they ran no tests |
| L3 | - |
| L4 | AutoCAD 2027, MacBook 2012 (Arch, HD 4000), the run-43390923029 build, 2026-11-13: partial - `IdServices.log` has no more `Failed to connect ... 10093`; AutoCAD's sign-in component completes `Handshake` and `IsLoggedIn` (false) with Identity Manager. AutoCAD still stops with "License manager is not functioning" for a different, not yet understood reason (its licensing SDK never contacts the licensing service). Inventor: pending |
| Cross review | pending |

## Open points

- Whether Windows' wldap32 takes or holds WinSock references on load is not
  measured; the test asserts only that the process's own reference survives.
- After this fix AutoCAD stops at the next problem (above). The patch fixes a
  measured Wine defect but not yet everything the user saw; it stays a
  candidate until AutoCAD gets further or the maintainer decides.

## Notes for a Wine bug report

AutoCAD 2027 loads and unloads wldap32 several times; after that its
sockets fail with WSANOTINITIALISED. A synthetic reproducer: WSAStartup
once, LoadLibrary/FreeLibrary("wldap32.dll") three times, socket() ->
INVALID_SOCKET, 10093 on Wine; the Windows result from L1. The libldap
destructor's unconditional WSACleanup() is in `libs/ldap/libldap/init.c`.

## History

- 2026-11-13: candidate written (Claude); L0, L2 and a partial L4 the same day.
