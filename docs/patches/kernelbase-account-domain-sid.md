# kernelbase-account-domain-sid.patch

<!-- winum-test: l1 -->

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
| L1 | Windows 10 VM (Mac host), L1 bundle of run 42483162074 (2026-11-08): advapi32 security x86_64 3653 checks 0 failures; i386 3666 checks, 4 failures in the unrelated upstream `test_system_security_access` (registry SACL, lines 6804-6836), the patch's checks (after line 6892) pass |
| L2 | not recorded per patch |
| L3 | not recorded |
| L4 | Content Center libraries load. (Inventor, maintainer) |
| L5 | in Inventor's runtime since `wine-11.19-1` (2026-10-19); a week of use not formally recorded - maintainer to confirm |
| Cross review | not recorded (written before the cross-review rule) |

### winum-11.22-1 (Wine 11.22)

| Level | Result | Date |
| --- | --- | --- |
| L0 | the whole series on the official Wine 11.22 archive, both architectures, `--fuzz=0` ([run 43703211594](https://github.com/Harvi654/winum/actions/runs/43703211594)) | 2026-11-15 |
| L1 | unchanged from winum-11.20-2: the patch and its test are the same, and Windows did not change | - |
| L2 | [run 43703213332](https://github.com/Harvi654/winum/actions/runs/43703213332), group g3 re-run [43739605851](https://github.com/Harvi654/winum/actions/runs/43739605851), paired against vanilla Wine 11.22: only the known flaky failures (ntdll threadpool, ntdll time, urlmon `url.c:3535` not repeated in the re-run); the workflow shows "failure" because of these | 2026-11-15 |
| L4 | Inventor 2027 on a copy of the maintainer's prefix: import, parts, assemblies (maintainer) - a smoke test of the build, not a retest of this patch's own feature. SOLIDWORKS and AutoCAD: deferred (not being worked on since 2026-11-09, AGENTS.md) | 2026-11-15 |

## Open points

Its Wine test was never run on Windows.

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).

