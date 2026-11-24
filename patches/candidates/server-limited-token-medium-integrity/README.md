# Limited token at Medium integrity

Work in progress; not in a normal build. Written by Claude; the cross
review goes to Codex.

## Problem as the user saw it

Autodesk Inventor Nastran 2027 (companion product in the Inventor 2027.2
prefix) shows a 0x80040154 dialog at Generate Mesh and at Run. With the
four per-user class candidates (wbemdisp-method-invoke,
msado15-stream-charset, combase-per-user-classes,
kernelbase-hkcr-class-reads) Nastran installs, registers, and its
classes are found, but its .NET classes NEWPFUI.NEWPFUIImpl and
NEWPFUI.NEProgressImpl still cannot be created.

## Cause

Nastran registers those classes per user only (HKCU\Software\Classes\CLSID,
InprocServer32 = mscoree.dll), the same as on Windows. The native .NET
Framework 4.8 COM shim (mscoreei.dll) reads the process token's
`TokenIntegrityLevel`; at High it ignores the HKCU registration and only
looks in HKLM (+relay of the native DLL, observed from outside only:
`OpenThreadToken` -> STATUS_NO_TOKEN, `GetTokenInformation(TokenIntegrityLevel)`,
`GetSidSubAuthority`, then the HKCR key is closed and only HKLM tried).
On Windows 10 the same VBScript `CreateObject` works from a normal
console (Medium) and fails with 0x1AD from an elevated one (High).

Wine reports High for every token:

- `NtQueryInformationToken(TokenIntegrityLevel)` in
  `dlls/ntdll/unix/security.c` returned a constant S-1-16-12288;
- the server's token has no integrity level at all; `token_create_admin`
  only gives the token *object* a High mandatory label, also for the
  `TokenElevationTypeLimited` token every process started from Unix gets
  (`server/process.c`).

Wine does not enforce mandatory labels in access checks
(`token_access_check` in `server/token.c` reads only the DACL), so the
level only matters to programs that read it.

## The fix in plain words

1. `server-token-integrity-level.patch` (wineserver): every token gets an
   integrity level. A limited token is Medium, a full or default admin
   token High. Duplicates (primary, impersonation, `ImpersonateSelf`,
   restricted tokens, child processes) keep the level of the token they
   were made from; the linked token is made with the opposite elevation
   type and so gets the other level. `get_token_info` returns it (one
   new reply field, fills existing padding; protocol version 980).
2. `ntdll-token-integrity-level.patch`: `TokenIntegrityLevel` builds the
   label SID from the server's value instead of a constant. Same size,
   same attributes (0x60).

Not changed: the token object's own label (`GetKernelObjectSecurity`
with `LABEL_SECURITY_INFORMATION` still shows High, which Wine's
existing `test_token_label` expects to match between a token and its
linked token), `NtSetInformationToken(TokenIntegrityLevel)` (still a
stub that claims success), and `TokenGroups` (Windows lists the label
there; MEASURE records it). Each is a separate question for the
measurement.

What a user sees change: programs started normally (from a shell,
the launcher, a .desktop file) now report Medium, as on Windows;
programs that Wine elevates (a manifest asking for administrator, the
desktop process explorer.exe and what it starts) stay High.
`TokenElevationType` and `TokenElevation` do not change, so
administrator checks based on them are unaffected. A program that
decides "admin or not" from the integrity level now sees "not admin",
as it would on Windows without elevation; the L4 has to cover the
Autodesk installer (ODIS), Inventor start, add-ins and Vault for that.

## How it was found

~~~bash
export WINEDEBUG=+relay
wine cscript //nologo 'C:\newpfui.vbs' NEWPFUI.NEProgressImpl 2>&1 | grep -E 'OpenThreadToken|GetTokenInformation|GetSidSubAuthority|RegOpenKeyEx' | tail -20
~~~

with the native .NET 4.8 (mscoree native override) in the Nastran
prefix; the same script on Windows 10 in a normal and an elevated
console. Reproducer without Autodesk files:
`docs/wine-repro/server-token-integrity-vbs/`.

## Files

| File | What |
| --- | --- |
| `series` | the two patches, in order |
| `server-token-integrity-level.patch` | server: level per token, `get_token_info` field |
| `ntdll-token-integrity-level.patch` | ntdll: report it |
| `../../tests-first/advapi32-tests-token-integrity-level.patch` | the Wine test (plain `ok()`; fails on vanilla, passes with the candidate) |
| `docs/wine-repro/server-token-integrity/` | MEASURE program, Windows and Wine results |
| `scripts/build-l1-token-integrity.sh` | L1 bundle: the exact test block, standalone, plus MEASURE |

Rebuild: apply `patches/series`, then this `series`, `--fuzz=0`. The
server protocol changes, so wineserver and ntdll must come from the same
build (they always do in a winum runtime).

## Validation

| Level | Status |
| --- | --- |
| L0 | Builds locally (Wine 11.22 + `patches/series` + candidate, x86_64 and i386 PE, `--fuzz=0`, 2026-11-24, reduced configure). Full build in the test-run pending. |
| L1 | MEASURE and the test bundle ready; offline Windows 10 run pending (normal and elevated console). |
| L2 | Local paired run (see Result log); CI paired run on `test-run/token-integrity-level-1` pending. |
| L3 | `docs/wine-repro/server-token-integrity-vbs/` - pending on the maintainer's prefix (needs native .NET 4.8). |
| L4 | Pending: Nastran Generate Mesh / Run; Inventor start, part, assembly, drawing, save/reopen, restart; ODIS installer; AutoCAD (server and ntdll are shared). |
| Cross review | Codex, pending. |

On winum-11.22-2 the L1 test fails 10 checks per architecture (every
Medium expectation; the High linked token passes): see the Result log.

## Open points

- The MEASURE results decide the test's expectations for the derived
  tokens (linked, restricted, impersonation); until they are back, the
  test's values for those are the hypothesis that each token follows its
  own elevation type.
- Token object label: if Windows shows Medium on a limited token's
  object, that is a follow-up (and Wine's `test_token_label` already
  expects the linked token's object label to equal the process token's).
- `SetTokenInformation(TokenIntegrityLevel)`: lowering a token (used by
  sandboxed child processes) is measured but not implemented.
- `TokenGroups` label entry: measured, not implemented.

## Notes for a Wine bug report

Programs that read `TokenIntegrityLevel` see High in every process,
while Windows reports Medium for a non-elevated process (limited
token). The .NET Framework COM shim then skips per-user (HKCU) COM
registrations of .NET classes, and `CreateObject` fails with 0x1AD /
REGDB_E_CLASSNOTREG. Steps: register a .NET class in HKCU only
(`per-user-dotnet-class.vbs`), run it with cscript in a prefix with .NET
4.8. Windows: works from a normal console, fails from an elevated one.
Wine: fails. Related: bug 17019 (HKCR merging).

## Result log

- 2026-11-24: candidate v1. MEASURE program on winum-11.22-2: all tokens
  S-1-16-12288, attributes 0x60, no label in `TokenGroups`,
  `SetTokenInformation` accepted without effect. Standalone L1 test on
  winum-11.22-2: 66 checks, 10 failures per architecture (expected).
