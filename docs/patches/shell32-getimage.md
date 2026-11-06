# shell32-getimage.patch

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `shell32`
- **Origin:** Autodesk-Inventor-for-Linux, commit `fe8d38b`, first shipped in `wine-11.19-1`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

New/Open: big icons and the preview pane stayed empty.

## Cause and fix

`IShellItemImageFactory::GetImage` uses the registered thumbnail handler, with the file's icon as fallback.

## Wine test

`dlls/shell32/tests/shlfolder.c`

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.19-1` (2026-10-19, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06) |
| L1 | Wine test exists, **never run on Windows 10** |
| L2 | not recorded per patch |
| L3 | not recorded |
| L4 | Icons and previews in the dialogs. (Inventor, maintainer) |
| L5 | in Inventor's runtime since `wine-11.19-1` (2026-10-19); a week of use not formally recorded - maintainer to confirm |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

Its Wine test was never run on Windows.

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).

