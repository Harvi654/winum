# Cross review: comctl32 toolbar padding height v2 (2026-11-09)

Reviewer: Codex; candidate author: Claude. Snapshot: `3e7279d`.

Read in full: candidate patch and README, production v1 patch and patch
page, MEASURE source, topic README, batch 1 L1 README and both toolbar L1
results. Parsed all 1040 cases per architecture in the Windows measurement
files; inspected the relevant rows rather than reading two megabytes of
repeated observations line by line. Read Wine 11.20
`7298a818b79446bae153b8d5aa27235ab3703bf9`'s complete
`TOOLBAR_MeasureButton`, `TOOLBAR_CalcToolbar`, `TOOLBAR_SetBitmapSize`,
`TOOLBAR_SetButtonSize`, `TOOLBAR_SetPadding`, the sizing call sites and
test insertion/activation-context setup. The rest of toolbar.c and its
existing tests were skimmed, not audited in full. Only public source and
our own public-API measurements were used.

## Findings

- **P1: the changed calculation also regresses a measured height that
  vanilla gets right.** Windows case 3 (v5, plain, image list, no strings,
  padding 0,0 before adding buttons) retains height 22 after
  `TB_SETBITMAPSIZE(16,16)`. The same observation holds for v6 and both
  architectures. Vanilla's `TOOLBAR_SetBitmapSize` always calls
  `TOOLBAR_CalcToolbar`; that passes `bHasBitmap=TRUE` to
  `TOOLBAR_MeasureButton`, whose original `16 + DEFPAD_CY` produces 22.
  The candidate changes it to `16 + 0`, producing 16. This is a source-
  derived new height mismatch, independent of the pre-existing width
  mismatch in that sequence. The candidate README already acknowledges
  Wine's extra recalculation, but treating it as an excluded test case
  does not protect behavior that matches Windows. Restrict the fix or
  correct the recalculation decision, with a measured regression test,
  before promotion. This finding applies to the unchanged v1 code too.
- **P2: the new no-string controls stop before the problematic operation.**
  They check creation/addition/autosize and then `TB_SETBUTTONSIZE`, but
  omit the measured intervening `TB_SETBITMAPSIZE` operation. Thus the
  Windows L1 pass and reported local v2 pass do not cover the P1 path.
  Add the no-string recalculation sequence as ordinary assertions; do not
  place this new regression under `todo_wine`.
- **P3: the header overstates the bitmap restriction.**
  `TOOLBAR_CalcToolbar` passes TRUE even with no valid image list. Say
  which calculation is actually changed, rather than implying this branch
  is reached only for controls with a bitmap. Keep the documented
  unimplemented list/padding-clamp differences separate.

## Expectations and scope

For the sequences actually asserted, the default-relative sizes, paddings
0/1/2, string/no-string controls, three non-list styles and image-list
minimum sizes agree with the Windows rows. Computing text sizes relative
to the default avoids hard-coding the VM font height. The test runs in v5
and v6. It has no new `todo_wine`; none is needed for its target cases.
The implementation hunk is exactly v1's; all other changes add the test
and header. List-style behavior and the padding clamp are unchanged.
The implementation is nevertheless wider than the passing subset, as P1
demonstrates.

## Checks and verdict

All 62 series entries applied with `--fuzz=0` in a separate pinned Wine
11.20 tree, substituting this candidate at its production position.
Production files and `patches/series` were not edited. This was an apply
check, not a new build or Wine runtime run.

The committed Windows L1 outputs from run 42642179431's bundle each report
2655 tests, zero failures/todos/skips, exit 0. These are native results, not
paired Wine L2. At review time the public Actions page showed the run still in progress,
9/10 matrix jobs complete and compare waiting (head `56bb9ee`). API access
was denied by the environment's network policy; no paired report or test
logs were independently retrieved. The candidate README's local comparison
(36 target failures without the fix, none with it) is recorded evidence,
not a new observation by this reviewer.

Verdict: **request changes before promotion**, for the measured no-string
height regression and its missing regression assertion. Keep the candidate
outside the production series until corrected and reviewed, then verify
the exact revision with native L1 and paired comctl32 L2. The existing
Inventor palette result does not rule out the neighboring regression.
