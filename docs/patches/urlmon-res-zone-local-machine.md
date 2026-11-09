# urlmon-res-zone-local-machine.patch

<!-- winum-test: missing -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `urlmon`
- **Origin:** Autodesk-Inventor-for-Linux, commit `f34967c`, first shipped in `wine-11.20-5`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

AutoCAD 2027 (experimental branch): status bar XAML failed and AutoCAD shut down.

## Cause and fix

`res:` URLs without a host map to the local machine zone.

## Wine test

None in the patch.

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.20-5` (2026-10-21, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06) |
| L1 | **no Wine test** - needs one, measured on Windows first |
| L2 | not recorded per patch |
| L3 | not recorded |
| L4 | AutoCAD 2027 starts (experimental). (Inventor, maintainer) |
| L5 | in Inventor's runtime since `wine-11.20-5` (2026-10-21); a week of use not formally recorded - maintainer to confirm |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

Measured 2026-11-09 (`docs/wine-repro/urlmon-zones/`): Windows puts a `res:///` URL without a module in zone 3 (Internet), not 0, and fails the literal `%prodname%` URL with E_INVALIDARG. The patch is to be corrected to the Windows value and checked again in AutoCAD; test first in `patches/tests-first/urlmon-tests-res-zones.patch`.

Wine test missing; Windows zone for res: without a module not measured by our own program.

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).

