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

## Which build has the candidate

Only builds of `test-run/token-integrity-level-1` carry the candidate
(it is in that branch's `patches/series`): "Build Wine"
[run 45317855190](https://github.com/Harvi654/winum/actions/runs/45317855190),
artifact `winum-11.22-2-binary` (the name is the series' version, the
content is series + candidate). Builds of `topic/token-integrity-level`
(e.g. run 45307586910) contain only `patches/series` - the candidate
lives in `patches/candidates/` there - and report High as before
(maintainer's first L3 attempt, 2026-11-24). Check a runtime with the
MEASURE program: `process.integrity=S-1-16-8192` for a program started
from Linux. For the L4 runtime the four Nastran overlays (wbemdisp,
msado15, combase, kernelbase) were copied over this artifact, both
architectures.

## Validation

| Level | Status |
| --- | --- |
| L0 | 2026-11-24: "Build Wine" on `test-run/token-integrity-level-1` (Wine 11.22 + `patches/series` + this series, both architectures, `--fuzz=0`) passed, [run 45317855190](https://github.com/Harvi654/winum/actions/runs/45317855190). Also a local reduced build (no X11). |
| L1 | 2026-11-24: passed on the maintainer's offline Windows 10 (10.0.19045), x86_64 and i686, from a normal and an elevated console: 66 checks, 0 failures, todos or skips each; MEASURE values for every token the test checks agree (`docs/wine-repro/server-token-integrity/windows-2026-11-24/`). |
| L2 | Local paired run 2026-11-24 (series vs series + candidate, both with the tests-first test): advapi32, ntdll, kernelbase, kernel32, combase, ole32 - no new failures; the 10 integrity checks per architecture fixed. CI run [45307710447](https://github.com/Harvi654/winum/actions/runs/45307710447) (compares *vanilla* Wine with series + candidate, so it also shows the whole series): the same 10 fixed per architecture, one unit `ntdll.i386.virtual` timed out (exit 124) on the series side; locally it passes in 4-5 s, 0 failures, three runs with the candidate. Re-run of group g1 (advapi32, ntdll, kernelbase, kernel32) on 2026-11-24, [run 45457615532](https://github.com/Harvi654/winum/actions/runs/45457615532): **0 regressions**, `ntdll.i386.virtual` finishes (2180 -> 2390 checks, a count that varies between runs), the 10 integrity checks per architecture fixed, `ntdll.i386.time` one check fixed (timing). L2 clean. |
| L3 | Windows 10 (2026-11-24): `per-user-dotnet-class.bat` created from a normal console, `err=0x1AD` from an elevated one - the same split as Nastran's NEWPFUI. Wine (2026-11-24, maintainer's Inventor 2027.2 + Nastran prefix, native .NET 4.8, candidate runtime below): `NEWPFUI.NEProgressImpl` and `NEWPFUI.NEWPFUIImpl` `err=0x0` (without the candidate `0x1AD`); MEASURE: limited process token 8192, linked 12288. `per-user-dotnet-class.bat` under Wine not run yet (see the repro README). |
| L4 | Nastran 2027, 2026-11-24, passed: part, Nastran environment, Generate Mesh (parabolic, 0.167 mm, 1,254,416 nodes, 806,387 elements), Run (linear static), von Mises contour, Deformed + Animate - no 0x80040154 dialog. Still pending: full Inventor smoke test (assembly, drawing, save/reopen, restart), ODIS installer, AutoCAD (wineserver and ntdll are shared). |
| Cross review | Codex, 2026-11-24. Approved for the combined test-run within the measured scope. CI timeout and remaining L4 checks are open. |

On winum-11.22-2 the L1 test fails 10 checks per architecture (every
Medium expectation; the High linked token passes): see the Result log.

## Open points

Measured on Windows 10 and left for follow-ups (none is needed by
Nastran; each changes what other programs see, so each would be its own
patch with its own test):
- Token object label: Windows labels a token object with the level of
  the process that created it (a normal process's linked full token has
  an S-1-16-8192 label); Wine labels every token 12288.
- Process object label (mask 0x3): Windows has one, Wine none.
- `TokenGroups`: Windows lists the label SID (attributes 0x60); Wine
  does not.
- `SetTokenInformation(TokenIntegrityLevel)`: Windows lowers, refuses
  raising with 1314; Wine claims success and changes nothing.
- `TokenMandatoryPolicy`: Windows 3; Wine fails with error 1.
- explorer.exe gets a default admin token from Wine (`ProcessWineGrantAdminToken`), so it and **everything it starts stay High**; on Windows explorer runs at Medium. Programs started from Linux are not affected, but a program started through explorer is - e.g. Inventor with a Wine virtual desktop (`INV_VDESKTOP`: the launcher runs `wine explorer /desktop=... Inventor.exe`), where Nastran's dialog stays. A fix in Wine would give the desktop process a limited token and elevate only what asks for it; that is a separate candidate with its own measurement.

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
- 2026-11-24: CI re-run of g1: no regressions; the earlier
  `ntdll.i386.virtual` timeout did not recur (L2 clean).
- 2026-11-24: L3 and L4 by the maintainer (Validation table): NEWPFUI
  created, Nastran mesh and solve without the dialog. Seen in the same
  Inventor run, not yet attributed: `DllGetClassObject` 0x80070002 on
  `C:\windows\system32\MSCOREE.DLL` for four CLSIDs registered nowhere
  (probably registration-free `clrClass` entries of a manifest), no
  visible effect; to be compared with a runtime without the candidate.
- 2026-11-24: L1 passed in the offline VM (normal and elevated console,
  both architectures); MEASURE confirms the test's values for every
  token. The first synthetic VBScript failed on Windows in both consoles
  and was replaced by a compiled per-user class; that one shows the
  Windows split (normal `err=0x0`, elevated `err=0x1AD`).
- 2026-11-24: local L2 paired and CI L0/L2 (Validation table). MEASURE on
  the candidate build (`wine-candidate-v1-*.txt`): limited process token,
  its pseudo handles, duplicates, restricted token, ImpersonateSelf token
  and the child are S-1-16-8192; the linked (full) token S-1-16-12288, its
  linked token S-1-16-8192. Every other line equals winum-11.22-2 (object
  labels, groups, `SetTokenInformation`). Standalone L1 test under the
  candidate: 66 checks, 0 failures per architecture.

## Cross review (Codex, 2026-11-24)

Read in full: both candidate patches, the advapi32 test patch, the
MEASURE program and this README. Checked the relevant Wine 11.22 token
creation, duplication and process-grant paths and all four native
measurement reports.

- The limited process token is Medium (8192) and the full token High
  (12288). In both consoles and bitnesses, primary, impersonation,
  identification and restricted duplicates, ImpersonateSelf, pseudo
  handles and the child retain the originating level. The linked token
  uses the opposite level and its linked token returns to the original
  level. Every sampled label has attributes 0x60. Initializing in
  create_token and copying in token_duplicate covers those paths.
- The explorer desktop path requests ProcessWineGrantAdminToken. The
  server replaces the token with a default admin token, which remains
  High. Full admin and explicitly supplied process tokens keep their
  existing path and duplicates retain the source level.
- Both patches apply without fuzz. The regenerated protocol header and
  request-handler assertions agree. The regenerated get_token_info
  trace printer agrees too. The new RID is at offset 44 and the reply
  remains 56 bytes. The protocol bump to 980 makes wineserver and ntdll
  a matched pair. The server handler checks TOKEN_QUERY and the ntdll
  SID result retains its layout and label flags.
- The native measurements establish the split-token behavior. Token
  object labels, TokenGroups, mandatory policy and integrity-level
  setters remain separate compatibility work. The measured Nastran
  result is a pass with the combined overlays. Recheck the recorded
  ntdll.i386.virtual timeout before declaring CI L2 clean. Full
  Inventor, ODIS and AutoCAD L4 remain open.
