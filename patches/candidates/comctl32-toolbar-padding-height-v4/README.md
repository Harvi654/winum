# comctl32-toolbar-padding-height v4: after the follow-up measurement

Candidate only: `patches/series`, the production patch and its page marker
are unchanged. Substitute `comctl32-toolbar-padding-height.patch` from this
folder for the production file at its place in the series (entry 10).
Supersedes `../comctl32-toolbar-padding-height-v3/` (its README has the
full comparison with the first measurement). Reviews:
`docs/wine-repro/cross-review-2026-11-09-comctl32-toolbar-padding-height-v2.md`
and `-v3.md`.

## What the follow-up measurement showed (review of v3, P2)

`docs/wine-repro/comctl32-toolbar-padding-height/windows-2026-11-09-followup/`:
- -1 in a direction of `TB_SETPADDING` leaves that direction unchanged;
  v3 reset both to the default - wrong after a non-default padding.
- A `TB_SETBITMAPSIZE` with another size recalculates buttons without
  text with the padding height too (padding 0, 20x20: 20x20); v3 had
  limited the padding height to buttons with text.

## The change (v4)

`dlls/comctl32/toolbar.c`:
- `TOOLBAR_MeasureButton` (non-list): height = bitmap + padding height
  (+ 1 + text with strings), as v1/v2;
- `TOOLBAR_SetBitmapSize`: the same size returns at once (from v3; this is
  what removes the regression the v2 review found);
- `TOOLBAR_SetPadding`: stored as given, 0xffff leaves that direction.

## Check against both measurements (local Wine 11.20, x86_64)

| | vanilla | v3 | v4 |
| --- | --- | --- | --- |
| first run: height / width matches (all paddings, relative, of 7200) | 5136 / 4912 | 7000 / 7200 | 7000 / 7200, 0 regressions |
| follow-up: padding read-back (of 24) | 0 | 6 | **24** |
| follow-up: height after another bitmap size (of 160) | 88 | - | **160** |
| follow-up: width after another bitmap size (of 160) | 106 | - | 148, 0 regressions |

Open, unchanged: `TBSTYLE_LIST` height padding (200 rows of the first
run); 12 follow-up rows where, with strings and the padding set after the
buttons, Windows keeps a wider button when the bitmap shrinks (vanilla
and v4 shrink it).

## Test

`test_padding_height` as v3, with the measured -1 sequences (-1 x -1 on
a new toolbar and after 3x4, -1 in one direction, 0xfffe), and
`test_padding_bitmap_size`: plain toolbar, image list, with and without
strings, padding default/0x0/0x10/10x0 set before or after the buttons,
bitmap 20x20, 16x24, 24x16, 16x15 - without strings the size, with
strings the height change. No `todo_wine`. Local run: vanilla 888
failures, v4 5309 tests, 0 failures (v5 and v6).

## Validation of v4

- L0: 62/62 at `--fuzz=0` with the batch 1 candidates, also with
  `comctl32-toolbar-setrows-v2` (2026-11-09).
- L1 (both architectures; v3's i386 run failed for an environment
  reason), L2 (`test-run/batch1-5`), L4 (Inventor palette), cross review
  (Codex): pending.
