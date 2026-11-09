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
