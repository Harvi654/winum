# comctl32-toolbar-padding-height v3: after cross review

Candidate only: `patches/series`, the production patch and its page marker
are unchanged. Substitute `comctl32-toolbar-padding-height.patch` from this
folder for the production file at its place in the series (entry 10).
Supersedes `../comctl32-toolbar-padding-height-v2/`; review:
`docs/wine-repro/cross-review-2026-11-09-comctl32-toolbar-padding-height-v2.md`.

## What changed from v2

The review (P1) found that v1/v2 used the padding height also for buttons
without text, so after `TB_SETBITMAPSIZE(16,16)` a toolbar without strings
and padding 0 got 16 instead of 22, which Windows and vanilla Wine agree
on. Checking every measured row relative to its default-padding row then
showed two more sources of difference that the patch made worse:
`TB_SETPADDING` clamping to `SM_CXEDGE` (so a padding above 2 came out as
2) and `TB_SETBITMAPSIZE` recalculating with an unchanged size. v3, three
functions of `toolbar.c`:

- `TOOLBAR_MeasureButton` (non-list): the padding height only when the
  buttons have text; without text Wine's `DEFPAD_CY`, as before (the
  function is reached for every toolbar, `TOOLBAR_CalcToolbar` passes
  `bHasBitmap = TRUE`; review P3);
- `TOOLBAR_SetBitmapSize`: the same size returns at once;
- `TOOLBAR_SetPadding`: stored as given; -1 x -1 gives 7 x 6.

## Check against the measurement (local Wine 11.20, x86_64)

The whole MEASURE (`docs/wine-repro/comctl32-toolbar-padding-height`,
1040 cases x 7 steps, v5 and v6), each size taken relative to the same
case with the default padding, so that Wine's and Windows' font metrics do
not enter:

| | vanilla | v2 | v3 |
| --- | --- | --- | --- |
| height matches (paddings 0-2, of 1800) | 1524 | 1668 (36 regressions) | 1740, 0 regressions |
| height matches (all paddings, of 7200) | 5136 | 5112 (264 regressions) | 7000, 0 regressions |
| width matches (all, of 7200) | 4912 | 4912 | 7200 |
| `TB_GETPADDING` read-back (of 7800) | 3840 | - | 7800 |

The 200 remaining height differences are all `TBSTYLE_LIST` with an image
list (Windows uses the padding height there too, Wine a fixed 6; not
changed).

## Test

`test_padding_height` (v5 and v6): three non-list styles x padding before
or after the buttons x four contents x eight paddings (default, 0, 1, 2,
8, 0x10, 10x0, 16): `TB_GETPADDING`, the size after the buttons (strings:
default-relative; otherwise the default size), after `TB_SETBITMAPSIZE(16,16)`
(unchanged - the review's P1/P2 sequence, ordinary assertions), after
`TB_SETBUTTONSIZE(1,1)` (16 + padding in every content); and -1 x -1 on a
new toolbar. No `todo_wine`. Local run: vanilla 806 failures, v3 5043
tests, 0 failures.

## Open points

- Inventor's palette (L4 of v1 showed rows as on Windows). v3 keeps the
  padding height only for buttons with text; if the palette's buttons have
  none, its rows depend on `TB_SETBUTTONSIZE` and on Wine no longer
  recalculating on an unchanged `TB_SETBITMAPSIZE`. **Needs L4 again.** If
  the rows are taller, a filtered `+toolbar` trace of opening the text
  editor (`grep -E "SetPadding|SetButtonSize|SetBitmapSize|cx %ld"`) shows
  the sequence.
- Not measured, measured by the follow-up program
  `docs/wine-repro/comctl32-toolbar-padding-height/measure-followup.c` (in
  `docs/wine-repro/batch1-measure/followup2.list`):
  -1 x -1 after another padding and -1 in one direction, and
  `TB_SETBITMAPSIZE` with a different size, with and without strings.
- `TBSTYLE_LIST` padding height (above).

## Validation of v3

- L0: the full series applies at `--fuzz=0` with the batch 1 candidates
  substituted (toolbar v3, header v2, ownerdraw v2, caret v2, SCF_WORD v4,
  textfont v3), 62/62, also with `comctl32-toolbar-setrows-v2`
  (2026-11-09).
- Local run (2026-11-09): toolbar, header, button units pass, 0 failures.
- L1 of the revised test, L2 (`test-run/batch1-3`), L4 (Inventor palette),
  cross review (Codex): pending.

## Cross review of v3 (2026-11-09)

`docs/wine-repro/cross-review-2026-11-09-comctl32-toolbar-padding-height-v3.md`:
the v2 findings are resolved; promotion is conditional on the sentinel
follow-up (P2: `-1 x -1` after a non-default padding has no Windows
result yet - `TOOLBAR_SetPadding`'s reset branch is not established for
that state until `measure-followup.c` returns), exact-body L1, paired L2
and the Inventor palette. P3 (this README named the wrong list) is
corrected. The code is unchanged; the follow-up MEASURE is the bundle of
run 42662108608 (`followup2.list`).
