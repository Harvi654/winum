# ntdll-locate-xstate-feature-enabled.patch

<!-- winum-test: l1 -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `ntdll`
- **Origin:** Autodesk-Inventor-for-Linux, commit `a2a6a54`, first shipped in `wine-11.20-4`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Frame Analysis: Simulate crashed the solver on CPUs with compacted XSAVE (Zen 3, newer Intel).

## Cause and fix

`RtlLocateExtendedFeature2` returns NULL for a feature that is not enabled before touching the XSAVE area.

## Wine test

`dlls/ntdll/tests/exception.c`

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.20-4` (2026-10-21, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06) |
| L1 | Windows 10 VM (Mac host), 2026-11-08: the patch's check run standalone (`docs/wine-repro/ntdll-xstate-locate/`, because the whole `ntdll:exception` unit restarts Windows VMs): x86_64 and i686 2 checks, 0 failures, CET_U not enabled, AVX enabled |
| L2 | not recorded per patch |
| L3 | not recorded |
| L4 | Frame Analysis Simulate (wine-11.20-6). (Inventor, maintainer) |
| L5 | in Inventor's runtime since `wine-11.20-4` (2026-10-21); a week of use not formally recorded - maintainer to confirm |
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

