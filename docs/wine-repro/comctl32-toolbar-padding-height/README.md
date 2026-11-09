# Toolbar button height and TB_SETPADDING: Windows MEASURE

Original, AI-assisted (Claude), LGPL-2.1-or-later. Patch:
`patches/comctl32-toolbar-padding-height.patch` (button height padding taken
from `TB_SETPADDING`, like the width, in `TOOLBAR_MeasureButton` for
non-list toolbars with a bitmap). Read before writing: the patch, Wine 11.20
`dlls/comctl32/toolbar.c` (`TOOLBAR_MeasureButton`, `TOOLBAR_CalcToolbar`,
`TOOLBAR_SetPadding`, which clamps the padding to `SM_CXEDGE`/`SM_CYEDGE`,
`TOOLBAR_SetButtonSize`) and `test_sizes` in `dlls/comctl32/tests/toolbar.c`.

What is not known and is measured: the button height Windows computes for a
given padding, whether `TB_SETPADDING` clamps (Wine: at most 2), and which
messages recalculate the size afterwards.

Matrix, comctl32 v5 and v6 (`COMCTL32 ... dll=` and `ccm_version` show which
one a case used):

- styles: plain, `TBSTYLE_FLAT`, `TBSTYLE_LIST`, flat list, and Inventor's
  symbol palette style `TBSTYLE_FLAT | TBSTYLE_WRAPABLE | CCS_VERT`;
- content: 16x16 image list without text (`img`, the Inventor case), image
  list with text, text with `I_IMAGENONE` and no image list, neither;
- padding: not set, and `TB_SETPADDING` (0,0) (1,1) (2,2) (3,3) (4,4) (7,6)
  (8,8) (10,10) (0,10) (10,0) (16,16) (0xffff,0xffff);
- order: padding set before the buttons are added, or after (then
  `TB_AUTOSIZE`).

Each case prints a `CASE id=...` line and `SIZE id=... step=...` lines after:
creation, `TB_SETPADDING`, `TB_ADDBUTTONS`, `TB_AUTOSIZE`, `TB_SETBITMAPSIZE`
(16,16), `TB_SETBUTTONSIZE` (1,1) (the minimum, which Wine computes from the
padding as well) and one more `TB_ADDBUTTONS`. Each `SIZE` line: `TB_GETPADDING`,
`TB_GETBUTTONSIZE`, `TB_GETITEMRECT` of buttons 0 and 1, window size and
`TB_GETMETRICS` (pad, bar pad, spacing). `SYSTEM` gives `SM_CXEDGE`, DPI and
the GUI font height, so expected values can be written relative to them.

Local check (Wine 9.0, not an oracle): complete, about 1 MB of output per
architecture; with padding 0,0 and the 16x16 image list the height stays 22
(bitmap + 6) through `TB_SETBITMAPSIZE` and drops to 16 only with
`TB_SETBUTTONSIZE` (1,1).

## Windows observations (2026-11-09)

Offline Windows 10 VM, `windows-2026-11-09/` (both architectures complete,
`api_errors=0`, identical apart from `bits=`). comctl32 5.82 and 6.16 give
the same sizes in all 520 cases; `SM_CXEDGE` 2, 96 DPI.

- `TB_SETPADDING` is not clamped: (16,16) reads back as 16,16, (0,10) as
  0,10. (0xffff,0xffff) reads back as the default 7,6.
- The size is recalculated only where strings are involved:
  `TB_ADDSTRING` / `TB_ADDBUTTONS` with strings. Then width = the wider of
  bitmap and text + padding.cx, height = 16 + padding.cy + 1 + text
  (default 23x38, 0x0 16x32, (0,10) 16x42, (10,0) 26x32) for the non-list
  styles, and height = 16 + padding.cy for `TBSTYLE_LIST` (43x22 default,
  36x16 for 0x0).
- Without strings (image list only, or nothing) the size stays 23x22 for
  every padding, also after `TB_SETIMAGELIST`, `TB_AUTOSIZE` and
  `TB_SETBITMAPSIZE(16,16)`; the list styles stay 27x22.
- Padding set after the buttons changes nothing until the next
  recalculation; `TB_AUTOSIZE` and `TB_SETBITMAPSIZE` with an unchanged
  size do not recalculate.
- `TB_SETBUTTONSIZE(1,1)` gives 16 + padding in both directions in every
  style and content (16x16 for 0x0, 32x32 for 16x16), and a following
  `TB_ADDBUTTONS` keeps it.
- `TB_GETMETRICS` (v6 only) reports the padding as `cxPad`/`cyPad`; v5
  returns zeros.

## Follow-up after cross review (`measure-followup.c`)

Original, AI-assisted (Claude), LGPL-2.1-or-later. For the v3 candidate:
`TB_SETPADDING` with -1 x -1 after paddings 3x4, 0x0 and 16x16, with -1 in
one direction only and with -2 x -2 (`PAD` lines: old value returned, value
read back), and `TB_SETBITMAPSIZE` with the same and four different sizes
(20x20, 16x24, 24x16, 16x15) on toolbars with and without strings,
padding default, 0x0, 0x10 and 10x0 set before or after the buttons
(`BMP` lines: button size before and after). v5 and v6. In
`docs/wine-repro/batch1-measure/followup2.list` and the main list;
completes under a local Wine 11.20 build.

### Windows observations of the follow-up (2026-11-09)

`windows-2026-11-09-followup/`, both architectures identical, exit 0,
`api_errors=0`; v5 and v6 identical.

- `PAD`: -1 in a direction leaves that direction unchanged: after 3x4,
  (-1,-1) keeps 3x4, (-1,5) gives 3x5, (5,-1) gives 5x4; the same after
  0x0 and 16x16. -2 (0xfffe) is stored as given. The returned old value is
  the padding before the call. (v3 reset both directions to 7x6: wrong.)
- `BMP`, without strings: the same bitmap size keeps the button (23x22);
  another size recalculates to bitmap + padding in both directions, with
  the padding current at that moment (padding 0: 20x20 bitmap gives
  20x20; 0x10 gives 20x30; default gives 27x26), whether the padding was
  set before or after the buttons.
- `BMP`, with strings: the height is bitmap + padding height + 1 + text
  (relative to the size before: the new bitmap height and the current
  padding height); the width is bitmap + padding, but with the padding
  set after the buttons a narrower result keeps the earlier width
  (23 stays 23 for a 20x20 bitmap with padding 0).

So the padding height applies to buttons without text as well when a
changed bitmap size recalculates them (v3 had limited it to text), and
only the same-size call recalculates nothing.
