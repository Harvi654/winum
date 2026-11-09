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
