# riched20-textfont-applytmp.patch

<!-- winum-test: missing -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `riched20`
- **Origin:** Autodesk-Inventor-for-Linux, commit `d029ebd`, first shipped in `wine-11.19-1`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Drawings: text turned black after the spell checker ran.

## Cause and fix

`ITextFont::Reset(tomApplyTmp)` drops temporary formatting and the following `tomApplyNow` no longer writes stale cached properties back.

## Wine test

None in the patch.

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.19-1` (2026-10-19, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06) |
| L1 | **no Wine test** - needs one, measured on Windows first |
| L2 | not recorded per patch |
| L3 | not recorded |
| L4 | No more black text. (Inventor, maintainer) |
| L5 | in Inventor's runtime since `wine-11.19-1` (2026-10-19); a week of use not formally recorded - maintainer to confirm |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

Wine test missing; temporary formatting (spell-check underline) is dropped, not implemented.

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).

