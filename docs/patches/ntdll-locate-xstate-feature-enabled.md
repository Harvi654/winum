# ntdll-locate-xstate-feature-enabled.patch

<!-- winum-test: test-not-on-windows -->

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
| L1 | Wine test exists, **never run on Windows 10** |
| L2 | not recorded per patch |
| L3 | not recorded |
| L4 | Frame Analysis Simulate (wine-11.20-6). (Inventor, maintainer) |
| L5 | in Inventor's runtime since `wine-11.20-4` (2026-10-21); a week of use not formally recorded - maintainer to confirm |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

Its Wine test was never run on Windows.

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).

