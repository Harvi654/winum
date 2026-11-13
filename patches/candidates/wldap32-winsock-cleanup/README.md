# wldap32 WinSock cleanup: candidate

AI-assisted (Claude), original code. Not submitted upstream. The
`libs/ldap/libldap/init.c` change is under the OpenLDAP Public License 2.8
(the license of that file, `libs/ldap/LICENSE` in Wine's source); the test in
`dlls/ws2_32/tests/sock.c` is LGPL-2.1-or-later.

**Status:** candidate. Not in `patches/series` on `main`. Waiting for: a
build (L0), L1 on Windows 10, L2, cross review by a second assistant, and L4
(AutoCAD 2027, whose L4 is otherwise deferred since 2026-11-09; Inventor as
the other program that loads wldap32).

## What it fixes

AutoCAD 2027 under winum-11.20-2 opens its main window and then stops with
"The License manager is not functioning or is improperly installed",
before any sign-in. Its sign-in component cannot open a socket to
Identity Manager (`IdServices.log`: `Failed to connect to server at port
45321. Error: system_error:10093`, WSANOTINITIALISED).

Wine's wldap32 is the bundled OpenLDAP libldap. Its library destructor
(`ldap_int_destroy_global_options()`) runs at every unload of wldap32.dll
and always calls `WSACleanup()`; `ldap_int_initialize()` calls
`WSAStartup()` only when LDAP is used. AutoCAD's main thread loads and
unloads wldap32 eight times without using LDAP, so wldap32 gives back eight
WinSock references it never took and the process count reaches 0.

The patch remembers whether `ldap_int_initialize()` had its own successful
`WSAStartup()` and calls `WSACleanup()` in the destructor only then.

## How it was found (2026-11-13, a 2012 MacBook, Arch, winum-11.20-2)

The prefix and the commands are in Autodesk-Inventor-for-Linux,
`docs/arch-hyprland.md`, section "AutoCAD 2027 experiment". In short, with
the Identity Manager and the licensing service started as `launch-inventor.sh`
does:

```bash
export WINEDEBUG=+pid,+winsock
wine acad.exe > ~/acad-trace.txt 2>&1
grep -aE 'startup count|not initialised' ~/acad-trace.txt   # per process
```

AutoCAD's process: WSAStartup to 1, 2, then balanced pairs, then
`WSACleanup` from 2 and from 1 on the main thread, then
`warn:winsock:WSASocketW not initialised` on the sign-in thread, then more
`WSACleanup` from 0. With `RelayInclude=ws2_32.WSAStartup;ws2_32.WSACleanup`
and `WINEDEBUG=+pid,+relay,+loaddll`, the return addresses map (by module
load addresses in the same trace) to: WSAStartup from netapi32, cer.dll,
DeviceSettingsProvider.dll and IdSDKPlugin.dll; WSACleanup from
DeviceSettingsProvider.dll (balanced) and eight times from WLDAP32.dll
(the same offset each time); WLDAP32.dll loaded and unloaded eight times.
Only Wine's own debug channels were used; nothing inside Autodesk binaries.

## Test

`test_WSAStartup_dll_unload` in `dlls/ws2_32/tests/sock.c`, run before
`Init()`: one `WSAStartup()`, wldap32 loaded and unloaded three times, then
`socket()` must succeed. It lives in ws2_32's tests because wldap32's own test
program imports wldap32, which then can never be unloaded. Skipped if
wldap32 is already loaded in the process. Expected value: the Windows 10
result (L1, pending); on vanilla Wine the socket fails with 10093.

## Rebuild / resume

The patch applies to Wine 11.20 with `--fuzz=0`, alone or after
`patches/series` (no series patch touches `libs/ldap` or `ws2_32`).
