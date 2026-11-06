# kernelbase-account-domain-sid.patch

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `kernelbase`
- **Origin:** Autodesk-Inventor-for-Linux, commit `543d641`, first shipped in `wine-11.19-1`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Content Center libraries stayed empty and a refresh crashed Inventor (.NET `Win32Exception: Invalid SID`).

## Cause and fix

`GetWindowsAccountDomainSid` fails with `ERROR_NON_ACCOUNT_SID` for well-known SIDs, as .NET expects, instead of `ERROR_INVALID_SID`.

## Wine test

`dlls/advapi32/tests/security.c`

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.19-1` (2026-10-19, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06) |
| L1 | Wine test exists, **never run on Windows 10** |
| L2 | not recorded per patch |
| L3 | not recorded |
| L4 | Content Center libraries load. (Inventor, maintainer) |
| L5 | in Inventor's runtime since `wine-11.19-1` (2026-10-19); a week of use not formally recorded - maintainer to confirm |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

Its Wine test was never run on Windows.

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).

