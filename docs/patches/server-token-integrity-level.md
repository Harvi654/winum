# server-token-integrity-level.patch

<!-- winum-test: l1 -->

- **DLL:** `server` (wineserver, `server/token.c`, protocol: `get_token_info` reply), test with the ntdll patch
- **Status:** In the winum-11.22-3 series. [Candidate history](../../patches/candidates/server-limited-token-medium-integrity/README.md)
- **Target:** winum-11.22-3

## Symptom

Inventor Nastran 2027, installed into the Inventor 2027.2 prefix, showed "Could not start the user interface components ... 0x80040154" at Generate Mesh and at Run. Not on Windows 10. Nastran registers its .NET classes (NEWPFUI) only per user, in `HKCU\Software\Classes`, as on Windows.

## Cause and fix

The native .NET Framework 4.8 COM shim (mscoreei.dll) reads the process token's `TokenIntegrityLevel` before it uses an InprocServer32 registration; at High it ignores per-user registrations and looks in HKLM only. Windows 10 does exactly that in an elevated console (`CreateObject` fails with 0x1AD) and creates the object from a normal one (Medium). Wine reported High for every token: wineserver had no integrity level per token and ntdll returned a constant.

The patch gives every server token an integrity level: Medium for a limited token (what processes started from Linux get), High for a full or default admin token. Duplicates (primary, impersonation, `ImpersonateSelf`, restricted tokens, child processes) keep the level of their source; the linked token is made with the opposite elevation type and so gets the other level - all as measured on Windows 10. `get_token_info` returns it in a new reply field that fills existing padding (protocol 980), which the ntdll patch reports.

Not changed (measured, separate follow-ups): the token object's own mandatory label (Windows: the creator's level), the label entry in `TokenGroups`, `SetTokenInformation(TokenIntegrityLevel)` (Windows lowers, refuses raising with 1314) and `TokenMandatoryPolicy`. Wine does not enforce mandatory labels in access checks, so the lower level changes no access.

In plain words: Wine told every program it runs "as administrator, elevated"; now a normally started program hears "normal user", as on Windows, and .NET then uses its per-user COM registrations.

## How it was found

`+relay` of the native mscoreei.dll (Microsoft .NET 4.8 with a native mscoree override, observed from outside only) around `CreateObject("NEWPFUI.NEProgressImpl")`: `OpenThreadToken` (STATUS_NO_TOKEN), `GetTokenInformation(TokenIntegrityLevel)`, `GetSidSubAuthority`, then only HKLM. The same three-line VBScript on Windows 10 in a normal and an elevated console. MEASURE program `docs/wine-repro/server-token-integrity/` (normal and elevated console, x86_64 and i686), reproducer without Autodesk files `docs/wine-repro/server-token-integrity-vbs/`.

## Wine test

In `ntdll-token-integrity-level.patch` (`dlls/advapi32/tests/security.c`, `test_token_integrity_level`): the server change is observable only through ntdll.

## Validation

| Level | Result |
| --- | --- |
| L0 | Applies with `--fuzz=0` after the full series on Wine 11.22; built x86_64 and i386 in the combined next-release build [45485575952](https://github.com/Harvi654/winum/actions/runs/45485575952) (2026-11-24) and by "Build Wine" on the release branch |
| L1 | The advapi32 test passed on offline Windows 10 (10.0.19045), x86_64 and i686, from a normal and an elevated console: 66 checks, 0 failures each (maintainer, 2026-11-24); MEASURE confirmed the level of every derived token |
| L2 | Paired series vs series + candidate: advapi32, ntdll, kernelbase, kernel32, combase, ole32 locally and g1 in CI [45457615532](https://github.com/Harvi654/winum/actions/runs/45457615532): no new failures, 10 checks per architecture fixed. [Run 45485567767](https://github.com/Harvi654/winum/actions/runs/45485567767): the released 11.22-2 series against it plus the seven 11.22-3 patches, 452 units in five groups, both architectures; no new failure (details in `docs/wine-repro/next-release-1/README.md`) |
| L3 | `per-user-dotnet-class.bat` (own class registered in HKCU only): Windows 10 normal `err=0x0`, elevated `err=0x1AD`; Wine with the patch `err=0x0`, without `err=0x1AD` (maintainer, 2026-11-24); NEWPFUI classes `err=0x0` |
| L4 | With the combined next-release build (maintainer, 2026-11-24/25): Inventor 2027.2 + Nastran prefix copy - part, assembly, drawing, open/save/save-as and reopen, Stress Analysis, Frame Analysis, Frame Generator, Nastran Generate Mesh / Run / results without the 0x80040154 dialog; AutoCAD 2027.1.1 fresh install from its Create Installer image (33 steps, exit 0), sign-in, open, SAVE, SAVEAS, reopen. SOLIDWORKS deferred under AGENTS.md |
| L5 | Pending |
| Cross review | Written by Claude; reviewed and approved by Codex (2026-11-24) within the measured scope, see the candidate README |

## Open

- explorer.exe gets a default admin token from Wine, so it and what it starts stay High (Windows: Medium) - e.g. Inventor in a Wine virtual desktop (`INV_VDESKTOP`). Separate candidate.
- Token object label, `TokenGroups` label, `SetTokenInformation`, `TokenMandatoryPolicy`: measured, not implemented.

## Notes for a Wine bug report

`TokenIntegrityLevel` is High in every Wine process; Windows reports Medium for a non-elevated one. The .NET Framework COM shim then skips per-user registrations of .NET classes and `CreateObject` fails with 0x1AD. Public synthetic test: `docs/wine-repro/server-token-integrity-vbs/` and the advapi32 test.

## History

- v1 2026-11-24: candidate, Nastran L3/L4.
- v2 2026-11-25: no code change; promoted to the series.
