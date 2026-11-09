# Cross review: comctl32 owner-draw capture v2 (2026-11-09)

Reviewer: Codex; candidate author: Claude. Snapshot: `3e7279d`.

Read in full: candidate patch/README, production v1 and patch page,
MEASURE source and topic README, shared comctl-support.h, new Wine test,
batch L1 README and both button L1 summaries. Parsed the complete 24-case
native result matrix per architecture and extracted the down-event order
for all eight owner-draw cases; the files match except for bitness. Read
Wine 11.20 `7298a818b79446bae153b8d5aa27235ab3703bf9`'s button down/up,
double-click, keyboard, capture-change and focus-related paths in
comctl32_v6, user32's corresponding down path, win32u's complete
`set_capture_window`, and the test insertion/v6 activation context. The
remaining button painting code and existing test suite were skimmed.
Only open source and our own public-API results were used.

## Findings

- **P1: none identified.** v2 removes v1's suppression of a repeated
  SetCapture. Windows sends WM_CAPTURECHANGED even when the same window
  already owns capture; both self-capture cases in the measurement prove
  this. The replacement instead moves the owner-draw SetCapture before
  SetFocus, keeping exactly one capture call on that path.
- **P2: none identified in the measured owner-draw scope.** The event
  expectations FS, CFS, CFS and OFS match v5/v6 on both architectures.
  Capture/focus after down, one BN_CLICKED and capture release after up
  are consistent with the native output. Ordinary checks cover the target;
  no new todo excuses a remaining discrepancy. Other button types retain
  their original focus/capture order, and the split-button early return
  is unchanged. The keyboard path is unchanged. Owner-draw double-clicks
  take the notification branch rather than falling through the new down
  sequence.
- **P3: clean up the registered class on the foreground-skip path.** The
  two windows are destroyed there, but UnregisterClassA is reached only
  on the normal path. This does not invalidate the returned L1 results
  (zero skips), but a single cleanup path would keep the fixture tidy.

## Scope and evidence

The test records only the measured events, avoiding unrelated hot-state
differences. It bounds the event buffer, resets setup events before the
down, and invokes the actual button procedure through a subclass, not a
replacement implementation. It requires a foreground parent and explicitly
skips when that prerequisite cannot be met. It is inserted after the suite
has activated comctl32 v6. User32's already-compatible order is unchanged.
The plain repeated-capture measurement likewise supports preserving
win32u's existing notification rather than changing it globally.

The implementation differs materially from v1, as required by the native
results. Its claimed explanation for SOLIDWORKS' superclass remains a
hypothesis; these own-window tests cannot demonstrate that consumer fix.
SOLIDWORKS L4 is deferred under AGENTS.md, and Inventor's owner-draw smoke
result still needs recording for the new implementation.

In a separate full-series pinned Wine 11.20 tree, reversed v1 and applied
v2 with `--fuzz=0`; the later themed-checkbox changes remain present. Test
insertion uses line offsets but no fuzzy context. Production series and
patches are unchanged. No new build or Wine runtime result is claimed.

Both native comctl32:button L1 outputs from run 42642179431's bundle report
14224 tests, zero failures/todos/skips, exit 0. The author's local Wine
comparison reports three event-order failures on vanilla and none with v2;
that is recorded evidence, not a rerun by this reviewer. The public Actions
page at review time still showed batch1-1 (head `56bb9ee`) in progress,
9/10 matrix jobs complete and compare waiting. API retrieval is blocked
by environment policy; no independent paired L2 report was obtained.

Verdict: **no blocking source/test finding** for the bounded replacement
of v1. Record paired L2 and the remaining applicable consumer validation
before treating the candidate as ready for a release. Native correctness
does not prove the old v1 SOLIDWORKS symptom stays fixed.
