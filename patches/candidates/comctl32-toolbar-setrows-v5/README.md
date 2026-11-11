# comctl32-toolbar-setrows-wrapable v5 (candidate)

v4 with `TB_SETROWS` no longer recalculating the button size. Applies
after `comctl32-toolbar-padding-height-v5`. Written by Claude, 2026-11-10.

## Why

Measured with Inventor's complete palette sequence
(`docs/wine-repro/comctl32-toolbar-padding-height/`, `windows-2026-11-10-sequence2/`):
Windows sets the button size with `TB_SETBITMAPSIZE` (bitmap + padding
current then) or `TB_SETBUTTONSIZE` and keeps it through a later
`TB_SETPADDING` and `TB_SETROWS` - 40x24 for the palette. v4's
`TOOLBAR_SetRowsVertical` called `TOOLBAR_CalcToolbar`, which
recalculated the size from the current padding (36x16 for the palette).
v5 sets the top margin and calls `TOOLBAR_LayoutToolbar` only.

## Test

`test_setrows_button_size`: Inventor's sequence (expect 40x24), the
sequence without a bitmap size change (23x22) and padding first (36x16),
each after `TB_SETROWS(12, TRUE)` and after a resize to 200 pixels, with
the measured layouts. The 60x600 step of `test_setrows_palette` is now an
ordinary check (it was `todo_wine` in v4 because of the 16-pixel buttons).

Local run (Wine 11.20 x86_64, Xvfb, `comctl32:toolbar`, 11107 checks):

| comctl32 | failures |
| --- | --- |
| padding v5 only | 348 |
| + setrows v4 | 18 (button size, 60x600 step) |
| + setrows v5 | 0 |

MEASUREs under the same Wine: all 24 palette steps and 144 of 148
vertical `TB_SETROWS` cases as Windows (rectangle included); in the
padding sequence MEASURE every button size equals Windows'.

## Open

- L1 (bundle of `test-run/all-5`), L2 (`all-5`), cross review (Codex).
- L4 in Inventor: the palette's buttons are now 40x24 as measured on
  Windows (v4 gave 36x16 under Wine); compare with Inventor on Windows.

## L1, L2, L4 (2026-11-11)

`docs/wine-repro/l1-all-5-2026-11-11/`: every setrows check passed on
Windows 10 in both architectures (one unrelated failure of the older
`test_WM_PAINT` on x86_64, from messages outside the test). L2 of
all-5: the only regression is a `test_ClipCursor` check that fails in
vanilla Wine too (3 of 6 local runs). L4 in Inventor 2027: the palette
matches Inventor on Windows (5 columns, same groups).
