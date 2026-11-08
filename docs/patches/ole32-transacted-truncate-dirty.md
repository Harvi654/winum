# ole32-transacted-truncate-dirty.patch

<!-- winum-test: l1 -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `ole32`
- **Origin:** Autodesk-Inventor-for-Linux, commit `accb378`, first shipped in `wine-11.20-6`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Found with the storage lock: a stream truncated to 0 in a transacted storage lost its data on commit.

## Cause and fix

`TransactedSnapshotImpl_StreamSetSize` marks the entry dirty when it drops the link to the parent's data.

## Wine test

`dlls/ole32/tests/storage32.c`

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.20-6` (2026-10-22, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06) |
| L1 | Windows 10 VM (Mac host), L1 bundle of run 42483162074 (2026-11-08): ole32 storage32 19102 checks, 0 failures, both architectures |
| L2 | not recorded per patch |
| L3 | Reproducer `docs/wine-repro/ole32-storage/trunc.c` (Inventor repository). |
| L4 | Frame Analysis Simulate (wine-11.20-6). (Inventor, maintainer) |
| L5 | in Inventor's runtime since `wine-11.20-6` (2026-10-22); a week of use not formally recorded - maintainer to confirm |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

Its Wine test was never run on Windows.

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).

