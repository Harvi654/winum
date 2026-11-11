# winex11-xinput-bad-device.patch

<!-- winum-test: l3-only: legacy XInput device-query errors and Wine's X11 fallback have no Windows counterpart; reproducer in docs/wine-repro/winex11-xinput-bad-device -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `winex11.drv`
- **Origin:** Autodesk-Inventor-for-Linux, commit `9b73c7e`, first shipped in `wine-11.19-1`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Hole command: crash `XI_BadDevice ... X_GetDeviceButtonMapping` under Xwayland.

## Cause and fix

The X error is caught and the identity button mapping is used for such a device.

## Wine test

No Windows test for XInput device-query failure. The own raw-input window
and scoped Linux fault helper are in
[`winex11-xinput-bad-device`](../wine-repro/winex11-xinput-bad-device/README.md).

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.19-1` (2026-10-19, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06) |
| L1 | L3 only: no legacy XInput/Xlib error-handler counterpart on Windows |
| L2 | not recorded per patch |
| L3 | reproducer/fault helper added 2026-11-09; 2026-11-11, [run 42999903587](../wine-repro/l3-ci-2026-11-11/README.md), KDE Wayland: **uncovered** - Wine never called XOpenDevice/XGetDeviceButtonMapping during the run (no call, no fault), clicks fine in all modes; fixture follow-up needed |
| L4 | Hole command works. (Inventor, maintainer) |
| L5 | in Inventor's runtime since `wine-11.19-1` (2026-10-19); a week of use not formally recorded - maintainer to confirm |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

Post-release fixture coverage follow-up (maintainer decision, 2026-11-11):
make Wine actually call XOpenDevice/XGetDeviceButtonMapping, for example
by testing a source-device switch. That trigger is not yet validated.
Require real XI_CALL coverage in observe mode and same-PID XI_HOOK/FAULT
coverage in injected modes before comparing fallback/survival and clicks.
The helper resolving both symbols alone does not exercise the patch.

The maintainer explicitly decided that this uncovered branch does **not**
block the winum-11.20-1 tag. The l3-only reason is the missing Windows
counterpart, and the maintainer reports this patch fixed Inventor's Hole
command crash (L4). Keep the marker; do not describe run 42999903587 as
a successful fault-injection L3. See the
[reviewed record](../wine-repro/l3-ci-2026-11-11/README.md).

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).

