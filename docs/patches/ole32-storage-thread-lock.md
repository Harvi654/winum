# ole32-storage-thread-lock.patch

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `ole32`
- **Origin:** Autodesk-Inventor-for-Linux, commit `accb378`, first shipped in `wine-11.20-6`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Frame Analysis: the solver `rkernel.exe` crashed while writing its results.

## Cause and fix

Structured storage objects of a file are serialised with one recursive lock (four threads wrote into one compound file at once).

## Wine test

`dlls/ole32/tests/storage32.c`

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.20-6` (2026-10-22, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06) |
| L1 | Wine test exists, **never run on Windows 10** |
| L2 | not recorded per patch |
| L3 | Multithreaded reproducer `docs/wine-repro/ole32-storage/mt.c` (Inventor repository). |
| L4 | Frame Analysis Simulate (wine-11.20-6). (Inventor, maintainer) |
| L5 | in Inventor's runtime since `wine-11.20-6` (2026-10-22); a week of use not formally recorded - maintainer to confirm |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

Its Wine test was never run on Windows.

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).

