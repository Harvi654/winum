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

Every case runs in a child process of the same program with a 3 s limit
(palette sequences 12 s); a case that does not return is printed as
`CHILD_TIMEOUT <case>` and the measurement goes on. `TIMEOUTS n` at the
end counts them. Build: in the bundle (`measure.list` here, also in
`../measure-bundle.list`). No input needed; text output only. Expect a
few minutes and a busy CPU while a hung case runs out its 3 s.

## Trial run on Wine 11.20 (local, vanilla comctl32), 2026-11-10

Completes, `api_errors=0`, 734 lines. With `TBSTYLE_WRAPABLE` every
`TB_SETROWS` result is undone by the wrap to the 300-pixel width (one row,
`TB_GETROWS` 1); without it the table of Wine's `test_setrows` holds
(8 rows: 5 with `bLarger` FALSE, 9 with TRUE). Not a measurement of
Windows; only shows that the program separates the cases.

## Windows 10, first run (2026-11-10), `windows-2026-11-10-first-run/`

The first version ran all cases in one process. x86_64 and i686 stopped
at the same place, after 198 cases (`v=5 style=vertwrap shape=g9`, all
rows): the next case, `v=5 vertwrap g3-4 rows=1`, did not return, the CPU
ran at full load and the 120 s watchdog ended the program (exit 2). So
`CCS_VERT | TBSTYLE_WRAPABLE` with a separator hangs in Windows'
comctl32 5 in at least that case. Both architectures agree on every
line that was written. The program now runs each case in a child
process with a time limit (above).

What the 198 cases already show (comctl32 5, `TBSTYLE_WRAPABLE` with and
without `TBSTYLE_FLAT`, all shapes; `CCS_VERT` for the 9-button shape):

- 9 buttons without separators: as in Wine's `test_setrows`.
- Separators count as rows: palette shape 12+10+10+8+7 with
  `TB_SETROWS(12)` gives `TB_GETROWS` 12 and the layout
  `8 4s 8 2s 8 2s 8s 7` - 8 buttons per row, a group continues on the
  next row, each new group starts a row, the separator on the row of the
  last buttons of its group. That is 8 columns, not the 5 the candidate
  v2 computes (and Inventor showed on Windows - but Inventor's palette
  uses comctl32 6, `CCS_VERT` and zero padding, still to be measured).
- `bLarger` FALSE can give fewer rows than asked (palette, 2 rows: one
  row of 47).

Complete results (v6, `CCS_VERT` with separators, the palette sequence):
next VM visit with the child-process version.
