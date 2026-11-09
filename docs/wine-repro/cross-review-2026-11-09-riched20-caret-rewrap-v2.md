# Cross review: riched20 caret rewrap v2 (2026-11-09)

Reviewer: Codex; candidate author: Claude. Snapshot: `3e7279d`.

Read in full: candidate patch/README, v1 patch and patch page, MEASURE
source, topic README, batch L1 README and both richole L1 outputs. Parsed
all 36 native child sequences on each architecture, checking the changed
and triggered observations for both TOM operations and all five tested
triggers; the two native files match apart from bitness. Read Wine 11.20
`7298a818b79446bae153b8d5aa27235ab3703bf9`'s complete cursor_coords,
create_caret/update_caret, offset extraction and rewrap routine,
ITextRange::SetText, relevant cursor lookup/selection/focus callers and
ME_Repaint/ME_UpdateRepaint. Inspected richole test initialization and
insertion context. Other editor, wrapping and test code was skimmed, not
audited in full. No vendor internals were inspected.

## Findings

- **P1: none identified in the bounded fallback.** v1 cached run/paragraph
  before a rewrap which may split runs. v2 saves the logical character
  offset, wraps, reconstructs the supplied cursor and only then reads
  run/paragraph and computes row/coordinates. This avoids using v1's
  stale pre-wrap layout. When the paragraph is already wrapped, the
  coordinate calculation and DC release path are unchanged.
- **P2: none identified in the asserted native values or todo placement.**
  S_OK/23 return values, collapsed selection 23,23, a non-first wrapped
  line and caret equality with EM_POSFROMCHAR agree with the observations.
  The new todo concerns only SetText's separately incorrect selection
  preservation, not wrapping or caret placement. Native output keeps
  0,0; Wine's existing SetText saves/restores a cursor structure across
  insertion. The caret equality assertion remains ordinary. The focus
  case's conditional check avoids treating a nonempty stale selection as
  a visible collapsed caret. ScrollIntoView intentionally has no caret
  equality assertion: the native observation would not support it.
- **P3: cover the shared msftedit path with the same ordinary assertions
  when practical.** Measurements exercise RichEdit20W and RICHEDIT50W;
  this new unit exercises RichEdit20A with ASCII text. Native L1 confirms
  that actual variant, but is not an assertion run of RICHEDIT50W for
  this test. The shared backend makes this a useful additional control,
  not evidence that the present expectations are wrong.

## Scope and checks

The source change applies only when the paragraph is marked for rewrap,
where vanilla asserted rather than returning a Windows-compatible caret.
It does not rewrite the already-wrapped coordinate calculation. Wrapping
still uses the existing DC and existing routine with invalidation FALSE,
as in v1. Saving the offset does not correct SetText's independent cursor
preservation bug, and the test does not claim it does. Native immediate
line-count behavior, more complex scripts/tables and arbitrary host/DC
failures are not established as fully compatible by this small matrix.

Replaced v1 by v2 in the separate full-series pinned Wine 11.20 tree:
`--fuzz=0` application passed, alongside the three reviewed comctl32
candidates. Production series and patches were not edited. No new build
or Wine runtime execution is claimed. The README's reported vanilla abort,
four v1 caret failures and zero v2 failures are author-provided local
evidence, not an independent paired run by this reviewer.

Both native riched20:richole L1 outputs from run 42642179431's bundle
report 183232 tests, zero failures/todos/skips, exit 0. At review time the
public Actions page for batch1-1 (`56bb9ee`) still showed 9/10 matrix jobs
complete with compare waiting. The environment blocks API retrieval; no
independent paired L2 report was obtained.

Verdict: **no blocking source/test finding** for replacing the abort and
v1's stale coordinate calculation. Record paired riched20/msftedit L2 and
the applicable Inventor dimension-edit smoke result for this revision
before promotion. The selection-preservation todo remains an explicitly
separate compatibility gap.
