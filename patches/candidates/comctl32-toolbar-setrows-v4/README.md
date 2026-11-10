# comctl32-toolbar-setrows-wrapable v4 (candidate)

Replaces `patches/comctl32-toolbar-setrows-wrapable.patch` after review
and validation; applies after `comctl32-toolbar-padding-height-v5`.
Written by Claude, 2026-11-10.

## Why v4

v3 failed L4: in Inventor 2027 the symbol palette fell apart. Inventor
changes the toolbar's width after `TB_SETROWS`; v3 then dropped the rows
and Wine's width wrap put the separators inside the rows. The Windows
measurement (`docs/wine-repro/comctl32-toolbar-setrows/`, palette
sequence) shows what Windows does after a width change: the groups and
separator rows stay, with as many buttons per row as fit (500 pixels:
`12 S 10 S 10 S 8 S 7`; 60 pixels with 23-pixel buttons: 2 per row).

## The change against v3

`TOOLBAR_Size`: after `TB_SETROWS` in a vertical wrapable toolbar, a
change of width (not `TB_SETROWS`' own resize) sets the buttons per row
to `width / button width` (at least 1) instead of dropping the rows.
Everything else as v3: the group rule only for `CCS_VERT |
TBSTYLE_WRAPABLE`, other toolbars untouched.

## Test

`test_setrows_groups` as in v3 (72 vertical, 16 horizontal cases) and new
`test_setrows_palette`: the measured palette sequence (`TB_SETPADDING(0,0)`,
`TB_SETROWS(12)` with bLarger FALSE and TRUE, resize to the rectangle,
replace a button, 500x300, `TB_AUTOSIZE`, 60x600), comctl32 5 and 6. The
60x600 step is `todo_wine`: Wine's buttons are 16 pixels wide there,
Windows' 23 (see "Button width" below), so Wine fits 3 per row.

Local run (Wine 11.20 x86_64, Xvfb, `comctl32:toolbar`, 10353 checks):

| comctl32 | failures | where |
| --- | --- | --- |
| padding v5 only | 320 | vertical cases and every palette step |
| + setrows v2 | 80 | horizontal cases, palette after the width change |
| + setrows v3 | 16 | palette after the width change (500x300, autosize) |
| + setrows v4 | 0 | (8 todo: the 60x600 step) |

MEASURE under the same Wine: 144 of 148 vertical cases and 20 of 24
palette steps as Windows (the 4 others: 60x600, button width); no
horizontal case changed from vanilla.

## Button width (not this patch)

With `TB_SETPADDING(0, 0)` and no strings, Windows keeps 23x22 buttons
through `TB_SETROWS`; Wine recalculates them in `TOOLBAR_CalcToolbar` to
16 wide (vanilla) and, with padding v5, 16 high. See the padding v5
README. It changes the 60x600 step only; whether it matters for Inventor
depends on how Inventor sizes the palette (trace requested).

## Open

- L1 of the new test (L1 bundle of `test-run/all-4`), L2 (`all-4`).
- L4 in Inventor: the palette as on Windows, also after resizing the
  dialog. If it still falls apart: a `+toolbar` trace of the palette's
  messages (`TB_SETROWS`, `TB_SETPADDING`, `TB_SETBUTTONSIZE`, sizes).
- Cross review (Codex).

## L4, 2026-11-10 (Inventor 2027, build 42865261514 of test-run/all-4)

Passed. The drawing-text symbol palette looks as in Inventor on the
Windows VM (screenshots compared by the maintainer, not committed):
5 columns, the same groups, separator lines across the palette, also
after the dialog appears. The `+toolbar` trace of the palette
(`docs/wine-repro/comctl32-toolbar-setrows/inventor-2026-11-10/`):
`TB_SETBITMAPSIZE` 16x15, then 32x16, `TB_SETPADDING(4, 0)`,
`TB_SETROWS(12, TRUE)`, then three resizes. With bLarger TRUE the rule
gives 4 per row for 44 buttons in 4 groups; the 5 columns on both
systems come from the width Inventor gives the palette afterwards
(width / button width), which is what v4 added.
