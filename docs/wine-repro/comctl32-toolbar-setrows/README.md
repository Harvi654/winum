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

## Windows 10, complete run (2026-11-10), `windows-2026-11-10/`

Child-process version; x86_64 and i686 exit 0 and identical in every
`CASE`, `PALETTE` and `CHILD_TIMEOUT` line: 676 cases, 24 palette steps,
`api_errors=0`, comctl32 5.82 and 6.16. **28 cases did not return**
(`CHILD_TIMEOUT`), all with `CCS_VERT | TBSTYLE_WRAPABLE` and separators,
in both versions, whenever fewer rows were asked than a vertical layout
of the groups needs: `g3-4` rows 1, `g1-6-2` rows 1-2, the palette shape
rows 1-4. Windows spins there (the CPU load seen in the VM); a Wine test
must not call those. Which call spins is not known: the program prints
nothing between creating the toolbar, `TB_ADDBUTTONSW` and `TB_SETROWS`
(cross review of the measurement, P2). Only those cases are affected;
the conclusions below use completed cases only.

Completion accounting (cross review P2: the parent counts only
timeouts): checked in the returned files - every other child ended with
exit 0 (`CHILD_END ... exit=00000000`), on both architectures, and no
`ERROR` line was printed.

### Inventor's palette (`CCS_VERT | TBSTYLE_WRAPABLE`, 12+10+10+8+7)

| rows | bLarger FALSE | bLarger TRUE |
| --- | --- | --- |
| 5 | 12 per row, `TB_GETROWS` 13 | same |
| 6 | 10 per row, 14 | same |
| 8 | 8 per row, 16 | same |
| 10 | 6 per row, 18 | same |
| 12 | **5 per row, 19** (layout `5 5 2 S 5 5 S 5 5 S 5 3 S 5 2`) | 4 per row, 21 |
| 15 | 4 per row, 21 | 3 per row, 26 |
| 20 | 3 per row, 26 | 2 per row, 32 |

Each group starts a new row, every separator has a row of its own, and
the number of buttons per row is the one whose button rows (separators
not counted) are the most that do not exceed the request (bLarger FALSE)
or the fewest that reach it (TRUE). comctl32 5 and 6 agree. This is what
candidate v2 computes: 5 columns for `TB_SETROWS(12)`.

After the palette sequence: resizing to the returned rectangle and
replacing a button keep the layout; a resize to 500x300 re-wraps it to
one row per group (`12 S 10 S 10 S 8 S 7`), `TB_AUTOSIZE` keeps that, and
a resize to 60x600 re-wraps to 2 per row. **Windows does not keep the
columns of `TB_SETROWS` across a resize.** The rectangle is 115x274 for
`TB_SETROWS(12)` even with `TB_SETPADDING(0, 0)` (buttons 23x22: without
strings the padding does not change the size, as in the padding MEASURE).

### Wine compared (local Wine 11.20 x86_64, `wine-2026-11-10/`)

`compare.py` compares `TB_GETROWS` and the layout of every case that
returned on Windows with vanilla Wine and with candidate v2 (on top of
padding v5); pixel sizes are ignored. Summary of `compare.txt`:

| style | cases | vanilla right | v2 right | v2 wrong where vanilla was right |
| --- | --- | --- | --- | --- |
| `CCS_VERT` wrapable (palette style) | 148 | 4 | 144 | 0 |
| `TBSTYLE_WRAPABLE` | 176 | 18 | 44 | 14 |
| `TBSTYLE_FLAT \| TBSTYLE_WRAPABLE` | 176 | 18 | 44 | 14 |
| `TBSTYLE_FLAT`, not wrapable | 176 | 62 | 44 | 18 |
| palette steps | 24 | 0 | 12 | 0 |

- Vertical wrapable toolbars: v2 matches Windows in 144 of 148 cases.
  The 4 others are `g3-4` rows 4 (both bLarger): Windows 4 per row
  (`3 S 4`), v2 2 per row; Windows' choice there does not follow the rule
  above (rows 3 gives 3 per row, rows 4 gives 4, rows 5 gives 2).
- Horizontal toolbars with separators: Windows keeps a separator on the
  row of the last buttons of its group, starts the next group on a new
  row and counts a separator as a row in `TB_GETROWS` (palette shape,
  rows 12: `8 4s 8 2s 8 2s 8s 7`, `TB_GETROWS` 12). v2 lays them out like
  vertical toolbars (`5 5 2 S ...`), wrong in most cases and in 46 where
  vanilla was right - also for toolbars without `TBSTYLE_WRAPABLE`.
- Resizes after `TB_SETROWS`: v2 keeps the columns, Windows re-wraps
  (12 palette steps wrong in both vanilla and v2).

### Consequence for the candidate

v2's rule is right for the case Inventor uses and wrong elsewhere, and
it changes horizontal toolbars that vanilla handled like Windows. A v3
limits the group rule to `CCS_VERT` wrapable toolbars, leaves other
toolbars to vanilla's code, and gets a Wine test from the vertical rows
above (both comctl32 versions; the hanging cases left out). Whether v3
can drop v2's "keep the columns on resize" depends on why Inventor needed
it - Windows does not do it; to be checked in Inventor (L4) with v3.

## Candidate v3 (2026-11-10)

`patches/candidates/comctl32-toolbar-setrows-v3/`: the group rule only
for vertical wrapable toolbars, rows dropped on a width change. Under
local Wine (`wine-2026-11-10/result-setrows-v3-x86_64.txt`,
`compare-v3.txt`): 144 of 148 vertical cases as Windows, no horizontal
case changed from vanilla. Still different from Windows, outside v3:

- After a width change Wine wraps a vertical toolbar by width the old
  way (palette 500x300: Windows `12 S 10 S 10 S 8 S 7`, Wine `22s 25s`).
- With `TB_SETPADDING(0, 0)` set before the image list and no strings,
  Windows' buttons stay 23x22 (rectangle 115x274 for 5 per row); with
  padding v5 Wine's are 16 wide (80x208). For the padding candidate:
  Inventor's palette looked right with 16-pixel rows on Windows, so it
  probably sets sizes in another order; to be looked at with padding.

## Candidate v4 (2026-11-10)

v3 failed L4 (Inventor changes the palette's width after `TB_SETROWS`).
v4 follows the palette sequence above after a width change: buttons per
row = width / button width, groups kept. Under local Wine
(`wine-2026-11-10/result-setrows-v4-x86_64.txt`, `compare-v4.txt`): 20 of
24 palette steps as Windows (the 60x600 step differs by the button width,
16 against 23 - see the padding v5 README), vertical and horizontal
cases as v3.
