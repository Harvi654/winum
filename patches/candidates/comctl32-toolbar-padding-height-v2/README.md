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
  (Wine adds 6 to the height whatever the padding);
- with v2: all pass; toolbars without strings keep 23x22 in both, as on
  Windows.

Not tested, because Wine differs from Windows outside this patch:
`TB_SETPADDING` above 2 (Wine clamps to `SM_CXEDGE`/`SM_CYEDGE`; Windows
kept 16x16 and turned 0xffff x 0xffff into 7x6), `TB_SETBUTTONSIZE` without
an image list (Windows' minimum height is 16 + padding, Wine's default
bitmap height is 15), `TBSTYLE_LIST` (Windows: height 16 + padding.cy,
Wine a fixed 6 with an image list), and padding set after the buttons
(Windows recalculates nothing then; Wine does on `TB_SETBITMAPSIZE`).

## Open points

- The differences listed under "Not tested". (An earlier reading of the
  source suggested that Wine recalculates when the image list is set; the
  local run shows 23x22 there, as on Windows.)

## Validation of v2

- L0: the full series applies at `--fuzz=0` on Wine 11.20 with this file
  substituted (62/62), also together with
  `candidates/comctl32-toolbar-setrows-v2` (2026-11-09).
- Local run (2026-11-09): below.
- L1 of the new test: pending (series L1 bundle of a test run).
- L2: pending (`test-run/<name>` with this candidate).
- Cross review: pending (Codex).

## Local run (2026-11-09)

Wine 11.20 built locally for x86_64 (Ubuntu, X11 under Xvfb), test units run
with the new tests applied, first on vanilla DLLs, then with all six batch 1
v2 candidates. Quick feedback only; L2 is the CI run.

| | vanilla | v2 |
| --- | --- | --- |
| comctl32:toolbar | 36 failures, all in `test_padding_height` (string rows, height 36 instead of 30-32) | 2655 tests, 0 failures |
