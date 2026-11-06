# mspatcha-pa19-32bit-and-large-files.patch

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `mspatcha`
- **Origin:** Autodesk-Inventor-for-Linux, commit `4a1a803`, first shipped in `wine-11.19-1`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Inventor 2026.4 update: PatchFiles failed with 1603 on .NET assemblies and on old files larger than 8 MB.

## Cause and fix

Wine MR 3870 (Aidan Khoury, 2023, never merged) ported to 11.19: 32-bit PE patching; plus old files larger than the LZXD window. Not original code of this project.

## Wine test

None in the patch.

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.19-1` (2026-10-19, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06) |
| L1 | **no Wine test** - needs one, measured on Windows first |
| L2 | not recorded per patch |
| L3 | not recorded |
| L4 | 2026.4 update installs. (Inventor, maintainer) |
| L5 | in Inventor's runtime since `wine-11.19-1` (2026-10-19); a week of use not formally recorded - maintainer to confirm |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

Third-party origin (Wine MR 3870); Wine test missing; licence LGPL like Wine.

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).

