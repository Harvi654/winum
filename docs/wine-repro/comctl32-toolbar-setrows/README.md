# TB_SETROWS with wrapable toolbars and separator groups (MEASURE)

For `comctl32-toolbar-setrows-wrapable`, which Inventor needs for the
symbol palette of its drawing text editor: a toolbar with
`CCS_VERT | TBSTYLE_WRAPABLE`, 47 buttons in 5 groups between separators,
sized with `TB_SETROWS(12)` (Autodesk-Inventor-for-Linux,
`docs/arch-hyprland.md`). The patch's handling of `bLarger` matches
Wine's own `test_setrows` table, but how Windows places the groups was
only compared visually. This program measures it, so that setrows v3 gets
a Wine test from Windows' values (or v2 is confirmed).

`measure.c` creates, for comctl32 5 and 6 (version 6 through an own
activation context, `../batch1-measure/comctl-support.h`), a fresh
300x400 toolbar per case with a 16x16 image list and calls
`TB_SETROWS(rows, larger)`:

- styles: `TBSTYLE_WRAPABLE`, `TBSTYLE_FLAT | TBSTYLE_WRAPABLE`,
  `TBSTYLE_FLAT | TBSTYLE_WRAPABLE | CCS_VERT` (the palette), and
  `TBSTYLE_FLAT` without wrapping as a control;
- button groups: 9 without separators (Wine's `test_setrows`), 3+4,
  1+6+2, and the palette shape 12+10+10+8+7;
- rows 1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 20, `bLarger` FALSE and TRUE.

Each `CASE` line gives `TB_GETROWS`, the rectangle `TB_SETROWS` returned,
the client size, the buttons with `TBSTATE_WRAP` and the layout from
`TB_GETITEMRECT`: buttons per row in order, `S` for a separator on a row
of its own, `s` appended when a separator shares a row with buttons.

`PALETTE` lines follow Inventor's known sequence on the palette shape:
zero padding, `TB_SETROWS(12)`, the toolbar resized to the returned
rectangle, one button deleted and inserted again (Inventor replaces its
buttons one by one), a resize to 500x300, `TB_AUTOSIZE`, a resize to
60x600.

Build: in the bundle (`measure.list` here, also in
`../measure-bundle.list`). Runs about 5 s, no input needed; text output
only.

## Trial run on Wine 11.20 (local, vanilla comctl32), 2026-11-10

Completes, `api_errors=0`, 734 lines. With `TBSTYLE_WRAPABLE` every
`TB_SETROWS` result is undone by the wrap to the 300-pixel width (one row,
`TB_GETROWS` 1); without it the table of Wine's `test_setrows` holds
(8 rows: 5 with `bLarger` FALSE, 9 with TRUE). Not a measurement of
Windows; only shows that the program separates the cases.

## Windows results

Pending (VM visit with the L1 bundle of `test-run/all-2`).
