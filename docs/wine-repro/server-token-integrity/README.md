# Token integrity level (MEASURE)

Autodesk Inventor Nastran 2027 registers its .NET classes per user only
(HKCU Software\Classes\CLSID, InprocServer32 = mscoree.dll). Under Wine,
`CreateObject` on them failed with 0x1AD and Nastran showed 0x80040154 at
Generate Mesh and Run. The same three-line VBScript fails the same way on
Windows 10 in an elevated console (High integrity) and works in a normal
one (Medium). A `+relay` trace of the native mscoreei.dll (Microsoft .NET
4.8, native mscoree override; observed from outside, nothing disassembled)
shows it reading the process token's `TokenIntegrityLevel` and, at High,
skipping the HKCU registration and looking only in HKLM.

Wine 11.22 reports High for every token: `NtQueryInformationToken`
(`dlls/ntdll/unix/security.c`) returns a constant S-1-16-12288, and
`token_create_admin` (`server/token.c`) gives every admin token, also the
limited (`TokenElevationTypeLimited`) token processes start with, a High
object label. This program measures what Windows reports instead.

The program uses only the process's own tokens. It writes no registry
keys or files and needs no network. It prints mandatory label SIDs
(S-1-16-*) only; group, user and logon SIDs are never printed (for groups
only their count and the label entries).

## What it prints (`key=value`, one per line)

For each token: `type`, `impersonation_level`, `elevation_type`
(1 Default, 2 Full, 3 Limited), `elevated`, `mandatory_policy`,
`integrity_probe` (GetTokenInformation with no buffer: return, error,
required size), `integrity` (SID) and `integrity_attributes`,
`integrity_size`, the label entries of `TokenGroups` (SID, index,
attributes), and the token object's own label from
`GetKernelObjectSecurity(LABEL_SECURITY_INFORMATION)` (control, ACE type,
flags, mask, SID).

| Prefix | Token |
| --- | --- |
| `process` | `OpenProcessToken` |
| `pseudo_process`, `pseudo_effective` | `GetCurrentProcessToken()`, `GetCurrentThreadEffectiveToken()` (integrity only) |
| `process_object` | label of the process object itself |
| `thread_before` | `OpenThreadToken` without impersonation (mscoreei calls it first) |
| `linked`, `linked_linked` | `TokenLinkedToken`, and its linked token |
| `dup_primary`, `dup_impersonation`, `dup_identification` | `DuplicateTokenEx` primary and impersonation, `DuplicateToken` |
| `restricted` | `CreateRestrictedToken(DISABLE_MAX_PRIVILEGE)` |
| `impersonate_self`, `impersonate_self_effective` | `ImpersonateSelf(SecurityImpersonation)` thread token |
| `set_untrusted` ... `set_system` | `SetTokenInformation(TokenIntegrityLevel)` on a fresh primary duplicate, then read back |
| `child` | a `CreateProcess` child (the same program with `--child`) |

## Offline Windows 10

1. Download the **token-integrity-measure** artifact of the workflow
   "Build the token integrity measurement" (branch
   `topic/token-integrity-level`) and check `SHA256SUMS` before copying it
   into the offline VM.
2. Open a **normal** console in the bundle directory and run
   `run-measure.bat`.
3. Open an **elevated** console ("Run as administrator") in the same
   directory and run `run-measure.bat` again.

The batch file names the results after the console's integrity
(`whoami /groups`). Return only these six files:
`exitcodes-medium.txt`, `exitcodes-high.txt`,
`result-token-integrity-{x86_64,i686}-{medium,high}.txt`.

## Local build and Wine observation

~~~bash
bash docs/wine-repro/server-token-integrity/build-bundle.sh out
~~~

Needs `x86_64-w64-mingw32-gcc` and `i686-w64-mingw32-gcc` (or
`CC_KIND=clang` for llvm-mingw). Under Wine:

~~~bash
export WINEPREFIX=/tmp/token-integrity-prefix
export WINEDEBUG=-all
wine out/token-integrity-x86_64.exe
~~~

`wine-winum-11.22-2-{x86_64,i686}.txt` are the results on winum-11.22-2
(2026-11-24, fresh prefix, started from a Unix shell): every token,
including the limited process token, reports S-1-16-12288 with
attributes 0x60; `TokenGroups` has no label entry;
`SetTokenInformation(TokenIntegrityLevel)` succeeds and changes nothing;
`TokenMandatoryPolicy` fails with error 1.

`wine-candidate-v1-{x86_64,i686}.txt`: the same with
`patches/candidates/server-limited-token-medium-integrity` (local build,
2026-11-24): the limited token and every token made from it S-1-16-8192,
the linked full token S-1-16-12288; nothing else changes.

## Windows 10 results

Pending. What the maintainer already saw on Windows 10 with
`whoami /groups` and the VBScript repro (`../server-token-integrity-vbs/`):
normal console Medium (S-1-16-8192), elevated console High
(S-1-16-12288).
