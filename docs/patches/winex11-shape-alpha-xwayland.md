# winex11-shape-alpha-xwayland.patch

<!-- winum-test: l3-only: Xwayland compositor alpha and X11 shapes have no Windows counterpart; reproducer in docs/wine-repro/winex11-shape-alpha-xwayland -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `winex11.drv`
- **Origin:** Autodesk-Inventor-for-Linux, commit `6697e7a`, first shipped in `wine-11.19-1`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Sketch input boxes had black rectangles around them under Xwayland.

## Cause and fix

Under Xwayland a window clipped with `SetWindowRgn` gets an ARGB visual and the pixels outside the region are transparent (compositors ignore the X shape).

## Wine test

No Windows test for Xwayland visual/compositor behavior. The own-dialog
fixture with region reset and excluded-window controls is in
[`winex11-shape-alpha-xwayland`](../wine-repro/winex11-shape-alpha-xwayland/README.md).

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.19-1` (2026-10-19, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06) |
| L1 | L3 only: the changed X11/compositor path is absent on Windows |
| L2 | not recorded per patch |
| L3 | reproducer added 2026-11-09; 2026-11-11, [run 42999903587](../wine-repro/l3-ci-2026-11-11/README.md), KDE Wayland: ARGB visual chosen only for the eligible targets; all five cases transparent with the patch; the opaque rectangle did not appear without it (unreproduced on this compositor) |
| L4 | No black rectangles. (Inventor, maintainer) |
| L5 | in Inventor's runtime since `wine-11.19-1` (2026-10-19); a week of use not formally recorded - maintainer to confirm |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

No Wine test (Xwayland compositor behaviour).

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).

