# comctl32-toolbar-padding-height v5: test fix

> **Promoted 2026-11-11** into `patches/series` for winum-11.20-1. This folder stays as the record of the candidate.

Candidate only: `patches/series`, the production patch and its page marker
are unchanged. Substitute `comctl32-toolbar-padding-height.patch` from this
folder for the production file at its place in the series (entry 10).
Supersedes `../comctl32-toolbar-padding-height-v4/` (its README has the
change and the measurements; Codex approved v4's code:
`docs/wine-repro/cross-review-2026-11-10-comctl32-toolbar-padding-height-v4.md`).

## What changed from v4

Only the test: `test_padding_height` declared `HWND toolbar;` without a
value and passed it to `rebuild_toolbar()`, which first destroys the
window it is given. On i386 the stack held the test's parent window, so
every later toolbar of the unit failed (L1 of batch1-4 and batch1-5 on
i386, and Wine's own i386 run in the batch1-5 L2: the same failures,
starting in this test at `toolbar.c:595`). v5 initialises it to NULL.
The DLL change is byte-identical to v4.

## Validation of v5

- L0: 62/62 at `--fuzz=0` with the batch 1 candidates (2026-11-10).
- L1: x86_64 passed with v4's test body (5309 tests, run 42718217301);
  both architectures pending for v5 (`test-run/batch1-6`).
- L2: batch1-5's x86_64 toolbar result has only the known setrows v1
  failure; i386 pending.
- L4 (Inventor palette): pending. Cross review of the one-line test fix:
  pending (Codex).

## L1 and L2 of test-run/batch1-6 (2026-11-10)

Run 42772681118 (`docs/wine-repro/batch1-measure/l1-2026-11-10-batch1-6/`).
L1: `comctl32:toolbar` 5309 tests, 0 failures, x86_64 and i386. L2 (paired, both architectures): 6 regressions, all known and
none from batch 1 (production setrows v1, mousemove v1, urlmon v1 and its
tests-first test; ntdll threadpool timing).

## L2: test-run/all-1 (2026-11-10)

[docs/wine-repro/l2-all-1-2026-11-10](../../../docs/wine-repro/l2-all-1-2026-11-10/README.md), run 42788810738, with every current candidate: no new failure in the touched DLLs, both architectures.

## Button size after TB_SETROWS (2026-11-10, found by the setrows MEASURE)

In the setrows measurement Windows kept 23x22 buttons after
`TB_SETPADDING(0, 0)`, an image list, buttons without strings and
`TB_SETROWS`; Wine with padding v5 had 16x16. Not a contradiction with
this candidate's L1, but a real Wine difference that the L1 test does
not reach:

- The padding MEASURE (`windows-2026-11-09`) shows Windows does not
  recalculate a toolbar without strings from the padding at any step it
  measured (buttons, `TB_AUTOSIZE`, same-size `TB_SETBITMAPSIZE`): 23x22
  with padding 0, 2, 10 or 16; only `TB_SETBUTTONSIZE` (and strings)
  bring the padding in (16 + padding). The setrows measurement adds
  `TB_SETROWS` to the steps that keep 23x22.
- Wine recalculates the size in `TOOLBAR_CalcToolbar` (bitmap + padding)
  whenever that function runs. `TB_ADDBUTTONS`, `TB_AUTOSIZE` and v5's
  same-size `TB_SETBITMAPSIZE` do not run it, so the L1 steps keep 23x22
  in Wine too; `TB_SETROWS` runs it (probe: 23x22 after the buttons,
  16x16 after `TB_SETROWS`, with and without the setrows patch).
- The width part (16) is vanilla Wine already (`szPadding.cx` in the
  width); v5 adds the height part (vanilla kept 22 = bitmap + 6).

So in this sequence v5 changes a height vanilla had like Windows. For
Inventor the 16-pixel rows looked right, which on Windows would need
`TB_SETBUTTONSIZE` or strings; a `+toolbar` trace of the palette (which
size messages Inventor sends) decides whether v5 is right for Inventor
for the right reason. Until then: open point before promotion.

### Inventor's sequence (trace, 2026-11-10)

The `+toolbar` trace of the palette shows no `TB_SETBUTTONSIZE`:
`TB_SETBITMAPSIZE` 16x15 -> 32x16, `TB_SETPADDING(4, 0)`,
`TB_SETROWS(12, TRUE)`. On the Windows VM the palette's columns are about
34-36 pixels apart and its rows about 16 (maintainer's screenshot), i.e.
buttons of bitmap + padding = 36x16: Windows applied the padding here.
The difference to the measured sequences is that Inventor *changes* the
bitmap size (the padding MEASURE only set the same size, and Windows
then recalculated nothing). Most likely Windows recalculates the buttons
after a real bitmap size change, with the padding current at the next
layout. For this sequence padding v5 gives what Windows gives (36x16;
vanilla 36x22), so v5 is right for Inventor for the right reason.

What remains is a narrower Wine difference, older than v5 for the width:
without a bitmap size change (the setrows MEASURE's sequence) Windows
keeps 23x22, Wine recalculates in `TOOLBAR_CalcToolbar`. Not needed by
Inventor; recorded as an open point, not a blocker. A MEASURE of
"bitmap size change, then padding, then TB_SETROWS" would confirm the
hypothesis above; proposed for a later VM visit.

## Resolved (2026-11-10): no v6

The measurement of Inventor's exact palette sequence
(`docs/wine-repro/comctl32-toolbar-padding-height/`, last section) shows
the button-size difference came from `TB_SETROWS` recalculating the size
(setrows candidate), not from this patch: v5's bitmap + padding rule
matches Windows at every measured `TB_SETBITMAPSIZE`. Fixed in
`../comctl32-toolbar-setrows-v5/`; with it, v5 no longer changes the
setrows sequence's 23x22 (Windows 23x22, vanilla 16x22, v5 + setrows v4
16x16, v5 + setrows v5 23x22).
