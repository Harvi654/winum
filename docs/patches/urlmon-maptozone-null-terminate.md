# urlmon-maptozone-null-terminate.patch

<!-- winum-test: missing -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `urlmon`
- **Origin:** Autodesk-Inventor-for-Linux, commit `c1b4e90`, first shipped in `wine-11.20-5`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

AutoCAD 2027 (experimental branch): ribbon icons failed at random (`SecurityException`).

## Cause and fix

`map_url_to_zone` copies the URL with its terminating null before `CreateUri`.

## Wine test

None in the patch.

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.20-5` (2026-10-21, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06) |
| L1 | **no Wine test** - needs one, measured on Windows first |
| L2 | not recorded per patch |
| L3 | not recorded |
| L4 | AutoCAD 2027 starts (experimental); no Inventor feature depends on it. (Inventor, maintainer) |
| L5 | in Inventor's runtime since `wine-11.20-5` (2026-10-21); a week of use not formally recorded - maintainer to confirm |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

Wine test missing; added for AutoCAD, not Inventor.

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).

