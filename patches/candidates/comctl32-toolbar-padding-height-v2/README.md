# comctl32-toolbar-padding-height v2: Windows-derived test

Candidate only: `patches/series`, the production patch and its page marker
are unchanged. Substitute `comctl32-toolbar-padding-height.patch` from this
folder for the production file at its place in the series (entry 10).

## What changed from v1

The code change is the same as v1 (in `TOOLBAR_MeasureButton`, non-list
layout with a bitmap: `nBitmapHeight + szPadding.cy` instead of
`nBitmapHeight + DEFPAD_CY`). v2 adds a Wine header and the test
`test_padding_height` in `dlls/comctl32/tests/toolbar.c`, run with v5 and
again in the v6 part of `START_TEST`.

## Windows basis

MEASURE `docs/wine-repro/comctl32-toolbar-padding-height/`, results in
`windows-2026-11-09/` (x86_64 and i686 identical apart from `bits=`;
comctl32 5.82 and 6.16 identical in all 520 cases each). What the test
uses, for styles plain, `TBSTYLE_FLAT` and Inventor's
`TBSTYLE_FLAT | TBSTYLE_WRAPABLE | CCS_VERT`, padding set before the image
list, strings and buttons:

| Content | default (7x6) | 0x0 | 1x1 | 2x2 | after `TB_SETBUTTONSIZE(1,1)` |
| --- | --- | --- | --- | --- | --- |
| 16x16 image list + strings | 23x38 | 16x32 | 17x33 | 18x34 | 16 + padding |
| strings, `I_IMAGENONE`, no image list | 23x38 | 16x32 | 17x33 | 18x34 | not tested (see below) |
| image list, no strings | 23x22 | 23x22 | 23x22 | 23x22 | 16 + padding |
| neither | 23x22 | 23x22 | 23x22 | 23x22 | not tested |

The test expresses the string rows relative to the default case of the same
style and content (`width - 7 + padding.cx`, `height - 6 + padding.cy`), so
font metrics do not enter. `TB_GETPADDING` returns the set value.

## Expected results

- vanilla Wine 11.20: the string rows fail for padding 0x0, 1x1 and 2x2
  (height stays default + 0, Wine adds 6), 2 styles x ... in v5 and v6;
  the image-list-only row is `todo_wine` (Wine recalculates when the image
  list is set and takes the width from the padding: 16x22 for 0x0);
- with v2: the string rows pass; the image-list-only row stays
  `todo_wine` (now 16x16 for 0x0, still not 23x22).

Not tested, because Wine differs from Windows outside this patch:
`TB_SETPADDING` above 2 (Wine clamps to `SM_CXEDGE`/`SM_CYEDGE`; Windows
kept 16x16 and turned 0xffff x 0xffff into 7x6), `TB_SETBUTTONSIZE` without
an image list (Windows' minimum height is 16 + padding, Wine's default
bitmap height is 15), `TBSTYLE_LIST` (Windows: height 16 + padding.cy,
Wine a fixed 6 with an image list), and padding set after the buttons
(Windows recalculates nothing then; Wine does on `TB_SETBITMAPSIZE`).

## Open points

- The image-list-only case: Windows does not recalculate the button size
  when an image list is set (23x22 for every padding); Wine does, and with
  this patch the height follows the padding as well (16x16 for 0x0
  instead of 16x22 before). Inventor's palette looked right with v1 (L4),
  which is consistent with Windows' TB_SETBUTTONSIZE result (16x16 for
  padding 0); whether Inventor reaches that size through TB_SETBUTTONSIZE
  or through Wine's extra recalculation is not known. A fix that stops
  the extra recalculation would be a separate patch, measured first.
- The three differences listed under "Not tested".

## Validation of v2

- L0: the full series applies at `--fuzz=0` on Wine 11.20 with this file
  substituted (62/62), also together with
  `candidates/comctl32-toolbar-setrows-v2` (2026-11-09).
- Local run: see the dated note below, when done.
- L1 of the new test: pending (series L1 bundle of a test run).
- L2: pending (`test-run/<name>` with this candidate).
- Cross review: pending (Codex).
