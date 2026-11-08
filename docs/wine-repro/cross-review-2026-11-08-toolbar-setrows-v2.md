# Cross review: toolbar TB_SETROWS v2 (2026-11-08)

Reviewer: Codex; candidate author: Claude. Reviewed at winum `1c30588`.
No proprietary binaries, shaders or implementation details were inspected.

## Scope

Read in full: candidate README and patch, production v1 patch, test plan,
AGENTS.md, check-series.sh and l2-series-2026-11-08.md. In Wine 11.20
(`7298a818b79446bae153b8d5aa27235ab3703bf9`), read the complete
TOOLBAR_SetRows, TOOLBAR_WrapToolbar, TOOLBAR_CalcToolbar and
TOOLBAR_LayoutToolbar functions, and test_setrows/tbrows_results in
tests/toolbar.c. The rest of these large source files was not read in full.

Applied padding-height followed by each alternative patch in separate copies
of upstream toolbar.c using `--fuzz=0`, then compared the resulting files.
The only implementation change from v1 is the two-line bLarger conditional;
the associated explanatory comment changes too. Hunk offsets account for
padding-height. WrapColumns, hidden-button filtering, separator stretching,
saved column count, resize handling and all other v1 code are unchanged.
This is a replacement for v1, not an additional patch to apply after v1.

## Findings

- **P1: none found in the bounded bLarger correction.** For visible groups
  of sizes g[i], WrapColumns(c) returns sum(ceil(g[i]/c)). This is monotone
  non-increasing. The loop finds the first column count with row count at
  most the request. If it undershoots, the previous column count necessarily
  exceeds the request; stepping back therefore chooses the closest larger
  achievable button-row count. Plateaus do not invalidate the argument.
  For nine buttons, request 8 gives 5 rows at two columns when FALSE and
  9 rows at one column when TRUE, matching the upstream table. Exact matches
  and the other table entries remain unchanged. Counts of zero/one visible
  button cannot decrement below one column. Hidden buttons and hidden
  separators are excluded consistently from both counting and wrapping.
- **P2: separator/hidden-button Windows coverage remains incomplete.**
  WrapColumns counts button rows, whereas LayoutToolbar's nRows also counts
  separator-related rows. Consequently the bLarger proof is about the
  helper's group model, not necessarily TB_GETROWS for grouped toolbars.
  This distinction predates v2. Leading, consecutive and trailing separators,
  groups split by hidden separators, and trailing hidden buttons require
  native measurements of TB_GETROWS, item rectangles and wrap bits. In
  particular LayoutToolbar decides whether a wrap is last using the last
  physical array index, not the last visible button. Do not label these
  inherited cases Windows-compatible solely from the nine-button test.
  The early-out comparing nRows to the requested count is also unchanged.
- **P2: required patch provenance header is absent.** The candidate starts
  directly with `--- a/...`; it needs the From/Subject, original AI-assisted
  LGPL attribution and dated v2 paragraph required by AGENTS.md before
  production replacement. This is also an inherited packaging deficiency,
  not a new bLarger logic defect.
- **P3: tests cover a narrow boundary.** test_setrows uses nine visible
  non-separator buttons and six sequential requests. It establishes the
  reported regression and its correction, not changes after hiding buttons,
  toolbar resizing, varying group sizes or separator geometry. The new
  conditional calls WrapColumns again, but final WrapColumns reapplies the
  selected layout, so its temporary mutation leaves no stale wrap bits.

## Verdict

Approve the bounded bLarger correction as a repair of the reported L2
regression. The recorded fourth paired run reports that toolbar passes in
both architectures with no new comctl32 failure; this review did not rerun
that CI job. Approval is not Windows L1 for the original grouping behaviour
and does not satisfy the release test gate. Before promotion, supply the
required header, native separator/hidden-button measurements and their
Windows-tested regression test, plus the Inventor palette smoke test.
Do not change series as part of this review. Mouse-move failures remain a
separate investigation as described in the L2 report.
