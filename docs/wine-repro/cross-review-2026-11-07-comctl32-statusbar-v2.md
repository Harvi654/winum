# Cross review: comctl32 status bar background v2 (2026-11-07)

Reviewer: Claude (wrote v1, did not write v2). Author of v2: Codex,
commit `2921b29`, `patches/candidates/comctl32-statusbar-v2/`.

Read in full: `0001-status-background-tests.patch`,
`0002-status-background-v2.patch`, the candidate `README.md` and
`series`, `scripts/build-l1-comctl32-statusbar-v2.sh`; checked against
Wine 11.20 `dlls/comctl32/status.c` and `dlls/comctl32/tests/v6util.h`
and the Windows MEASURE results in
`docs/wine-repro/comctl32-statusbar-erase/windows-2026-11-07/`.
Not done: building or running anything (no MinGW in the review
environment); the runtime claims below are from reading the code.

## Behaviour against the Windows measurements

| Case | Windows (measured) | v2 |
| --- | --- | --- |
| v5, WM_PAINT | background always | `STATUSBAR_HasTheme` is FALSE for v5, so `STATUSBAR_Refresh(..., TRUE)`: always |
| v5, SB_SETTEXT | no erase | `InvalidateRect(..., __WINE_COMCTL32_VERSION == 6)` = FALSE |
| v5, other invalidations, WM_ERASEBKGND | (vanilla Wine already matched Windows) | all back to FALSE / `DefWindowProcW`, i.e. vanilla |
| v6 unthemed, WM_PAINT | background always (custom dark erase painted over) | `!HasTheme` = TRUE: always |
| v6 unthemed, SB_SETTEXT | erase | TRUE |
| v6 themed, WM_PAINT | no background; erase fill survives; non-erasing repaint keeps old pixels | FALSE: never in the `BeginPaint` path |
| v6 themed, invalidations | SB_SETTEXT erases (others not measured) | TRUE, as v1 |

v2 matches every measured case. For v5 the result is vanilla Wine's
status bar again, which the Wine MEASURE run already showed to match
Windows. For Inventor (themed v6, its own `WM_ERASEBKGND`) the behaviour
is the same as v1 in every path Inventor's traces showed (erasing
invalidations, erase handled by Inventor and returning 1).

## Findings

- **P2 - the unthemed v6 path is not exercised under Wine.** The test gets
  an unthemed v6 control only through `SetWindowTheme(bar, L"", L"")`,
  which fails in Wine (separate uxtheme bug, marked `todo_wine`), and then
  skips. So the main correction of v2 for v6 (background in WM_PAINT for
  an unthemed v6 control) has no Wine-side coverage; only v5 and themed v6
  are checked under Wine. Acceptable for L1 (Windows exercises all three),
  but the README should say it, and L2 cannot show that path either. A
  second way to get an unthemed v6 control would have to be measured on
  Windows first; not required for promotion.
- **P3 - themed v6 with an erase left undone.** v1 painted the background
  when `ps.fErase` was still TRUE (an application whose `WM_ERASEBKGND`
  returns 0 without painting, or no erase handler reached). v2 never paints
  it in themed v6. The Windows data only covers erases that were handled
  (default handler or our fill returning 1), so this case is unmeasured.
  Inventor is not affected (it returns 1). Worth one more MEASURE row
  ("subclass returns 0 without painting") before the next revision; not a
  blocker.
- **P3 - indentation.** The `if (!STATUSBAR_HasTheme(...))` line in the
  `WM_ERASEBKGND` case uses spaces where the surrounding block uses tabs.
- **P3 - promotion form.** The candidate applies after v1. When it is
  promoted, fold it into `patches/comctl32-statusbar-erase-background.patch`
  as its v2 (one fix per file, the test included, a dated v2 paragraph),
  instead of adding a second patch for the same fix.

Test code: expectations are derived from the measured marker pixel and
erase counts only (no glyph or font assumptions); the bar geometry
(12,24,600,44 in a 660x180 host) is the MEASURE geometry. `todo_wine`
marks are limited to the vanilla differences and are removed exactly
where 0002 fixes them. `load_v6_module`/`unload_v6_module` match
`v6util.h`. The L1 builder extracts the test block unchanged, downloads
nothing, and builds with `-Werror`.

## Verdict

Approved for L1 on Windows 10 and, after L1, for a paired L2 of the
comctl32 suite. P2 is to be documented, not fixed, for this round.
Not a fix for Inventor's overlapping status text (that is the separate
maximised-window/GPU investigation); it removes v1's divergence for
unthemed status bars.
