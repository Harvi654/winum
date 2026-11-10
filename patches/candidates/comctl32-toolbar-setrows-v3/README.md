# comctl32-toolbar-setrows-wrapable v3 (candidate)

Replaces `patches/comctl32-toolbar-setrows-wrapable.patch` after review
and validation. Applies after `comctl32-toolbar-padding-height-v5` (the
series order; with the production padding patch it does not apply).
Written by Claude, 2026-11-10.

## Why v3

The MEASURE `docs/wine-repro/comctl32-toolbar-setrows/` (Windows 10,
comctl32 5 and 6, both architectures) showed that v2's rule is Windows'
rule for vertical wrapable toolbars - the kind Inventor's symbol palette
is - and wrong for horizontal ones, where v2 had changed 46 cases that
vanilla Wine already did like Windows (also toolbars without
`TBSTYLE_WRAPABLE`). Windows also wraps by width again after a resize,
which v2 prevented for good.

## The change (against v2)

- `TOOLBAR_SetRowsVertical`: v2's rule (groups start a new row, a
  separator has its own row, the number of buttons per row from the
  requested rows and bLarger), used only for `CCS_VERT | TBSTYLE_WRAPABLE`.
  Every other toolbar goes through Wine's unchanged `TB_SETROWS` code.
- The rows are kept while buttons change and while `TB_SETROWS` sizes the
  toolbar to its rectangle (`bSettingRows`); `TOOLBAR_Size` drops them
  when the width changes, as Windows does, and the toolbar wraps by width.
  v2 kept them for good. This is the part the maintainer checks in
  Inventor (L4); if the palette falls apart, it comes back as a recorded
  known deviation.
- Unchanged from v2: `TOOLBAR_WrapColumns`, the full-width separator
  lines of a vertical toolbar while the rows are kept.

## Test

`test_setrows_groups` in `dlls/comctl32/tests/toolbar.c`, run with
comctl32 5 and 6, expected values from the Windows results (generated
from `windows-2026-11-10/result-toolbar-setrows-x86_64.txt`):

- 72 vertical cases (group shapes 9, 3+4, 1+6+2, 12+10+10+8+7; rows 1-20;
  bLarger both): `TB_GETROWS` and the rows of buttons. Left out: the 28
  cases Windows does not return from (fewer rows than the groups need)
  and the 3+4 groups with 4 rows (Windows' choice there does not follow
  the rule; v3 gives 2 per row, Windows 4).
- 16 horizontal cases (`TBSTYLE_FLAT | TBSTYLE_WRAPABLE` and `TBSTYLE_FLAT`)
  with separators that vanilla Wine already does like Windows, so a
  change to horizontal toolbars fails the test.

Local run (Wine 11.20 x86_64, Xvfb, 2026-11-10), `comctl32:toolbar`:

| comctl32 | checks | failures |
| --- | --- | --- |
| padding v5 only (no setrows patch) | 9069 | 280 (all vertical) |
| padding v5 + setrows v2 | 9069 | 64 (all horizontal) |
| padding v5 + setrows v3 | 9069 | 0 |

MEASURE under the same Wine (`wine-2026-11-10/compare-v3.txt`): v3
matches Windows in 144 of 148 vertical cases (as v2) and differs from
vanilla in no horizontal case.

## Open

- L1: the test on Windows 10 (L1 bundle of `test-run/all-3`).
- L2: `test-run/all-3`.
- Cross review (Codex).
- L4 in Inventor: the palette with 5 columns and its groups, also after
  the dialog is resized (rows are no longer kept across a width change).
- Not in this patch: after a width change Wine wraps a vertical toolbar
  differently from Windows (Windows: each group on new rows, separators
  on their own rows; Wine: by width as before). And with
  `TB_SETPADDING(0, 0)` before the image list and no strings, Windows'
  buttons stay 23x22 in this sequence, Wine's (padding v5) become 16
  wide - for the padding candidate, see the MEASURE README.
