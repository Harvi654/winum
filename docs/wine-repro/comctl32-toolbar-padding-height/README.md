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

## Sequence MEASURE (2026-11-10): `measure-sequence.c`

Why: in the setrows measurement Windows kept 23x22 buttons without
strings after `TB_SETPADDING(0, 0)` and `TB_SETROWS`, while Wine with v5
had 16x16 - v5 changes there a height vanilla Wine has like Windows.
Inventor's palette (trace, `../comctl32-toolbar-setrows/inventor-2026-11-10/`)
does `TB_SETBITMAPSIZE` 16x15, then 32x16, `TB_SETPADDING(4, 0)`,
`TB_SETROWS(12, TRUE)`, and on Windows its buttons look like bitmap +
padding (36x16, from a screenshot only). This program measures it.

Per comctl32 5 and 6, a `CCS_VERT | TBSTYLE_WRAPABLE` toolbar with 47
buttons without strings in groups 12+10+10+8+7, one child process per
sequence (10 s limit), a `STEP` line after every step with
`TB_GETBUTTONSIZE`, `TB_GETPADDING`, the first item's rectangle,
`TB_GETROWS`, the client size and the layout. Steps: `1`
`TB_SETBITMAPSIZE` 16x15 (the default), `2` 32x16, `i`/`I` image list
16x16/32x16, `a` the buttons, `p` `TB_SETPADDING(4, 0)`, `r`
`TB_SETROWS(12, TRUE)`, `w` width 200, `z` `TB_AUTOSIZE`.

| Sequence | Steps | Question |
| --- | --- | --- |
| inventor | `12Iaprwz` | Inventor's order |
| inventor-buttons-late | `12Iparwz` | buttons after the padding |
| padding-first | `p12Iarwz` | padding before the size change |
| padding-between | `1p2Iarwz` | padding between the two sizes |
| no-size-change | `1iaprwz` | same as the setrows MEASURE, padding 4,0 |
| imagelist-only-32 | `Iaprwz` | size changed by the image list only |
| imagelist-only-16 | `iaprwz` | no size change at all |
| changed-and-back | `121iaprwz` | a change undone before the buttons |
| inventor-no-setrows | `12Iapwz` | Inventor's order without `TB_SETROWS` |

Trial under Wine 11.20 with padding v5 and setrows v4
(`wine-2026-11-10/`): completes, no error. Inventor's order gives 36x16
after `TB_SETROWS`; "no-size-change" gives 20x16 (Wine recalculates on
`TB_SETROWS` whatever happened before).

Run in the VM: own list `sequence.list`; about 20 s, no input.

## Sequence MEASURE on Windows 10 (2026-11-10), `windows-2026-11-10-sequence/`

x86_64 and i686 identical, exit 0, `api_errors=0`, no child failed;
comctl32 5.82 and 6.16 identical. **The hypothesis from the screenshot
is wrong.** What Windows does (button size `TB_GETBUTTONSIZE`):

| Sequence | after bitmap/image list | after `TB_SETPADDING(4,0)` | after `TB_SETROWS` | after resize, autosize |
| --- | --- | --- | --- | --- |
| inventor (`12Iaprwz`) | 39x22 | 39x22 | **39x22** | 39x22 |
| inventor-buttons-late | 39x22 | 39x22 | 39x22 | 39x22 |
| padding-first (`p12Iarwz`) | 36x16 | (before) | 36x16 | 36x16 |
| padding-between (`1p2Iarwz`) | 36x16 | (between) | 36x16 | 36x16 |
| no-size-change (`1iaprwz`) | 23x22 | 23x22 | **23x22** | 23x22 |
| imagelist-only-32 | 39x22 | 39x22 | 39x22 | 39x22 |
| imagelist-only-16 | 23x22 | 23x22 | 23x22 | 23x22 |
| changed-and-back | 23x22 | 23x22 | 23x22 | 23x22 |
| inventor-no-setrows | 39x22 | 39x22 | - | 39x22 |

Windows computes the button size when the bitmap size is set
(`TB_SETBITMAPSIZE`, also 16x15 -> 23x21, and `TB_SETIMAGELIST` with
another size), as bitmap + the padding current at that moment. A later
`TB_SETPADDING` changes nothing, and neither do the buttons,
`TB_SETROWS`, a resize or `TB_AUTOSIZE`. So in Inventor's order the
padding (4,0) comes too late and Windows' buttons stay 39x22.

Wine 11.20 with padding v5 (`wine-2026-11-10/`): identical up to
`TB_SETROWS`; there `TOOLBAR_CalcToolbar` recalculates with the current
padding (Inventor's order 36x16, no-size-change 20x16), which Windows
does not do. Vanilla Wine recalculates there as well (width from the
padding, height + 6).

Consequences:

- The 16-pixel rows of Inventor's palette on Windows do not come from
  this sequence (it gives 39x22 there). The trace grep (`SETBUTTONSIZE`
  in capitals) could not match Wine's `TOOLBAR_SetButtonSize` trace
  lines, so a `TB_SETBUTTONSIZE` or other size call by Inventor may
  simply be missing from it.
- The difference to fix is not "padding in the height", but that Wine
  recalculates the button size from the padding where Windows does not
  (at least `TB_SETROWS`). Padding v6 should be written once Inventor's
  complete size calls are known, so that the L4 palette stays right.

## Inventor's complete palette sequence (second and third trace, 2026-11-10)

Filtered `+toolbar` traces of the palette in Inventor 2027 under Wine
(summary only; the trace lines are not committed): `TB_SETBITMAPSIZE`
16x15, `TB_SETBUTTONSIZE` 23x22, `TB_SETBITMAPSIZE` 32x16,
**`TB_SETBUTTONSIZE` 40x24**, then the 47 buttons one by one (each
`TB_ADDBITMAP` with its own bitmap handle and `TB_ADDBUTTONS` with one
button; Wine's buttons 40x24), `TB_SETPADDING(4, 0)`, `TB_SETINDENT`,
`TB_SETROWS(12, TRUE)` (Wine with padding v5 and setrows v4: 5 per row,
180x224, i.e. 36x16 buttons), then three resizes. The first trace's grep
had missed the `TB_SETBUTTONSIZE` calls.

`measure-sequence.c` now also runs this sequence ("inventor-exact",
steps `1b2Bmpnrwz`) and variants: without the padding, without
`TB_SETROWS`, the padding before `TB_SETBUTTONSIZE(40,24)`,
`TB_SETBUTTONSIZE(1,1)` instead of 40x24, and without any
`TB_SETBUTTONSIZE`. The question for Windows: after
`TB_SETBUTTONSIZE(40, 24)` and a later `TB_SETPADDING(4, 0)`, does the
palette end with 40x24 or 36x16 buttons? Trial under Wine with padding
v5 (`wine-2026-11-10/result-sequence2-...`): 40x24 until `TB_SETROWS`,
36x16 from there.

## Inventor's exact sequence on Windows 10 (2026-11-10), `windows-2026-11-10-sequence2/`

x86_64 and i686 identical, exit 0, no child failed, comctl32 5.82 and
6.16 identical. Button size (`TB_GETBUTTONSIZE`):

| Sequence | after 40x24 / bitmap | after `TB_SETPADDING(4,0)` | after `TB_SETROWS` | after resize |
| --- | --- | --- | --- | --- |
| inventor-exact (`1b2Bmpnrwz`) | 40x24 | 40x24 | **40x24** | 40x24 |
| exact-no-padding | 40x24 | - | 40x24 | 40x24 |
| exact-no-setrows | 40x24 | 40x24 | - | 40x24 |
| exact-padding-before-buttonsize | 40x24 | (before) | 40x24 | 40x24 |
| exact-buttonsize-minimum (1x1) | 39x22 | 39x22 | 39x22 | 39x22 |
| exact-no-buttonsize | 39x22 | 39x22 | 39x22 | 39x22 |

So Windows' palette buttons are 40x24 (5 per row after Inventor's
resize to 200 pixels in the measurement); neither `TB_SETPADDING` nor
`TB_SETROWS` changes the size; `TB_SETBUTTONSIZE(1, 1)` does not shrink
below the bitmap size + padding (39x22).

**Conclusion.** The difference is not in padding v5: its rule
(bitmap + padding when the bitmap size is set) matches every measured
`TB_SETBITMAPSIZE` step, e.g. 20x15 and 36x16 with the padding set
first, where vanilla's +6 height would give 21/22. The difference was
`TB_SETROWS` recalculating the size in `TOOLBAR_CalcToolbar` (in the
setrows candidate). Setrows v5 only lays the buttons out there; with it,
under local Wine (`wine-2026-11-10/result-sequence2-padding-v5-setrows-v5-x86_64.txt`)
every button size of every step of every sequence equals Windows'. What
still differs are layouts *before* `TB_SETROWS` (Wine's width wrap of a
vertical toolbar, an old Wine difference outside both candidates).

No padding v6 is needed. The open point "v5 changes a case vanilla has
like Windows" came from the setrows sequence with padding 0, where v5
plus setrows v4 gave 16x16; with setrows v5 it is 23x22 as on Windows.
The 16-pixel rows read from the Windows screenshot are not what Windows
measures (40x24); to be compared again in the L4 with setrows v5.
