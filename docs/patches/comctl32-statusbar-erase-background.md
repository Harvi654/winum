# comctl32-statusbar-erase-background.patch

<!-- winum-test: l1 -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `comctl32`
- **Origin:** Autodesk-Inventor-for-Linux, commit `cf9e456`, first shipped in `wine-11.19-1`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Inventor's status bar was white instead of dark.

## Cause and fix

The status bar paints its background in `WM_ERASEBKGND` (which Inventor handles), `WM_PAINT` paints it only if nobody erased, invalidations ask for an erase, owner-drawn parts get no theme pane.

## Wine test

Since v2 (in the series from winum-11.20-1): the status background test in
`dlls/comctl32/tests/status.c` (marker pixels outside the text, themed and
unthemed v6, v5; measured on Windows 10, `windows-2026-11-07`).

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.19-1` (2026-10-19, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06); winum-11.20-1 series (promoted 2026-11-11): every entry applies with `--fuzz=0` on the official 11.20 archive; the tree is identical to that of `test-run/all-6` (run 42997560598) |
| L1 | 2026-11-07, v5 180/0, v6 360/0, both arch (promoted file: `comctl32-statusbar-v2`, its two incremental stages merged with v1 into one file (same tree at `--fuzz=0`)) |
| L2 | not recorded per patch; winum-11.20-1: ok (all-1..5, `comctl32:status` 1025 -> 1401) |
| L3 | not recorded |
| L4 | Dark status bar in Inventor 2026/2027. (Inventor, maintainer); winum-11.20-1: Inventor: status bar (all-1) |
| L5 | in Inventor's runtime since `wine-11.19-1` (2026-10-19); a week of use not formally recorded - maintainer to confirm |
| Cross review | Claude, approve (2026-11-07) |

## Open points

The P2 of the cross review is documented, not fixed (see the v2 candidate README).

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).

## History

- **2026-11-11, promoted** into `patches/series` for winum-11.20-1 from `comctl32-statusbar-v2`, its two incremental stages merged with v1 into one file (same tree at `--fuzz=0`) (maintainer approval of `docs/release/winum-11.20-1.md` at `ef64451`). Open: merge into one patch file; P2 of the review documented, not fixed.
