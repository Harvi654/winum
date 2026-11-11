# Cross review: toolbar TB_SETROWS v5 (2026-11-11)

Reviewer: Codex; candidate author: Claude. Candidate `dbd7fcd`, reviewed
at `7dfc712`. This is an independent source and stored-evidence review;
no Wine, Windows or CAD program was executed here. No vendor internals
were inspected.

## Scope

Read in full: v5 patch and README, v4-to-v5 patch diff, setrows MEASURE
README, padding-height MEASURE README and `measure-sequence.c`, the prior
setrows v2 and MEASURE reviews, and the all-5 validation README/exitcodes.
Read the v5 implementation and all three tests in full, including their
helpers and both v5/v6 registrations. Earlier v2/v3/v4 patches were
compared for implementation changes; their documentation was read only
in relevant sections. The setrows measurement source was read in relevant
construction, output and completion sections, not re-audited in full.

In original Wine 11.20 (`7298a818b79446bae153b8d5aa27235ab3703bf9`),
read complete `TOOLBAR_WrapToolbar`, `TOOLBAR_CalcToolbar`,
`TOOLBAR_LayoutToolbar`, `TOOLBAR_SetRows`, `TOOLBAR_Size`,
`TOOLBAR_SetBitmapSize`, `TOOLBAR_SetButtonSize` and
`test_setrows`/`tbrows_results`. Other portions of these large files
were not read in full. Downloaded these two open-source files from the
pinned Wine commit and applied padding v5 then setrows v5 with
`patch -p1 --fuzz=0`: all hunks applied without fuzz or offsets.

Parsed the complete relevant CASE/PALETTE/child outcomes in the complete
setrows native records and STEP/child outcomes in sequence2, comparing
architectures. Read the all-5 native failure and summary lines, rather
than every unrelated toolbar trace line. L2 and L4 are the repository's
reported results, not independently rerun CI or application validation.

## Findings

- **P1: none found in the reviewed v5 delta.** The only v4-to-v5 DLL
  change replaces `TOOLBAR_CalcToolbar` with top-margin initialization
  and `TOOLBAR_LayoutToolbar` in `TOOLBAR_SetRowsVertical`. The former
  overwrites button dimensions from current padding; the latter keeps
  them and still updates wrapping, item rectangles and row counts.
  This matches the measured ordering, including explicitly set 40x24
  buttons. The implementation is limited to `CCS_VERT | TBSTYLE_WRAPABLE`;
  the original horizontal/non-wrapable SetRows branch remains intact.
- **P2: inherited native coverage remains incomplete.** Hidden buttons,
  hidden/leading/trailing/consecutive separators, variable-width buttons,
  style transitions and nonzero indent are not covered by this matrix.
  Separator stretching changes `rect.right` after tooltip rectangles have
  already been updated; the available layout strings do not establish
  the correct Windows separator extent or tooltip geometry. This is an
  inherited limitation, not a demonstrated v5 size regression. Retain
  the earlier measurement follow-up; do not claim general conformance.
- **P3: four completed vertical cases remain deliberately wrong.**
  `g3-4`, request 4, both bLarger values and comctl32 versions return
  `3 S 4` on Windows. The group-search model does not reproduce that
  exception; tests explicitly omit it. Vanilla was also wrong there.
  The 28 timeout cases provide no layout oracle and are correctly left
  out of the test. The early return comparing requested rows with
  `nRows` (which includes separator-related rows) is retained and lacks
  repeated-request coverage with separator groups.
- **P3: test checks are narrower than the observed geometry.**
  `test_setrows_groups` checks row count and layout, palette checks the
  operation sequence, and button-size checks dimensions and layouts
  before/after a 200-pixel resize. Returned bounding rectangles, wrap
  bits, separator widths and API failures in several setup/resize calls
  are not all asserted. The fixed 64-item/128-character helper buffers
  suffice for the current visible matrices; they are not general helpers
  for arbitrary hidden/large shapes.

## Oracle and preservation checks

A local parser matched all 88 `test_setrows_groups` entries to completed
native CASE records for both comctl32 versions (176 matches). Both native
architectures agree on 1,436 CASE/PALETTE/timeout/child-end lines: 676
returned cases, 24 palette steps, 28 timeouts and 708 child ends. The only
nonzero child outcomes are the 28 recorded timeouts. The sequence2 native
architectures agree on 307 STEP/child-end/failure-summary lines, with no
failed child. Thus the historical parent completion defect does not
invalidate these individually completed observations.

The palette expectations follow the stored zero-padding sequence,
including two buttons per row at width 60 with unchanged 23x22 buttons.
The three size tests follow native sequences: exact Inventor 40x24,
no-size-change 23x22, padding-first 36x16. The Inventor test omits the
MEASURE's zero-indent operation; it does not claim coverage of unknown
nonzero application indent. The expected resize layouts fit respectively
5, 8 and 5 buttons at width 200 and match the native STEP records.

The stored v4 comparison reports 144/148 completed vertical cases right,
including all four that vanilla already got right; horizontal results
are unchanged from vanilla (18 flatwrap, 18 wrap and 62 nowrap matches).
These are read results, not a fresh Wine run. Source inspection confirms
v5 changes neither that selection algorithm nor the horizontal path.
The lost vanilla-compatible 23x22 height in the zero-padding sequence is
restored by avoiding recalculation, rather than changing padding rules.
The v5 README reports 24/24 palette steps and matching sequence sizes;
the stored sequence2 v5 output supplies size evidence, but no full v5
setrows matrix output is committed in the setrows result directory.

The requested `comctl32-toolbar-padding-sequence/` path does not exist:
the actual program and native records are under
`comctl32-toolbar-padding-height/`, notably `windows-2026-11-10-sequence2/`.
The screenshot-derived 36x16 hypothesis is explicitly superseded by
measured 40x24 and the recovered TB_SETBUTTONSIZE call sequence.

## Validation and verdict

`7dfc712` is the validation-record commit, not an Actions run ID.
The all-5 record identifies bundle commit `f5b9397`, run `42889340446`.
Native summaries show x86_64 11,108 checks/one failure and i386
11,107/zero; the sole recorded failure is the old WM_PAINT sequence,
not one of these setrows tests. The README reports all three setrows
tests passed in both architectures. L2 records one ClipCursor difference
also reproduced on vanilla in 3/6 local runs; this review did not rerun
that investigation. L4 is maintainer-reported Inventor 2027 comparison,
build `42889351892`, matching columns/groups; differing screenshot scales
do not independently measure pixel dimensions or establish other CAD L4.

The old MEASURE review's two P2 points were addressed by documentation in
`7dee0ba`, not instrument fixes: the current parent still aggregates only
timeouts and lacks pre-call stage markers. The README now limits hang
attribution and verifies successful child records individually. Keep the
instrument robustness and P3 coverage/time-budget follow-ups open.

Verdict: **approve the bounded v5 size-preservation correction and the
measured visible-group/palette behavior, with the inherited coverage and
four known matrix deviations disclosed above.** This is not blanket
Windows compatibility or an independent L0/L2/L4 pass. No production
series, candidate source or release marker is changed by this review.
