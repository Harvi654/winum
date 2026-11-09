# Cross review: server input attachment components v2 (2026-11-09)

Reviewer: Claude; candidate author: Codex. Reviewed at winum `a1ab7b7`.
No vendor or Microsoft binaries were inspected.

## Scope

Read in full: the candidate header, its `server/queue.c` and
`server/window.c` hunks, compared line by line with the production v1
patch; the candidate README; in Wine 11.20 `server/window.c`
`attach_parent_thread`, `set_parent_window` and the create path, and
`is_toplevel`. Read the test's todo sites, its timing and foreground
calls, and its structure. Skimmed: the 592 literal state checkpoints
of the test body (spot checks against the Windows output only) and the
MEASURE program.

## Server change

- `queue.c`: identical to v1 apart from hunk offsets (the base now
  includes the earlier server patches of the series). The component walk
  was reviewed with v1; nothing new to judge.
- `window.c`: `set_parent_window` no longer calls
  `attach_parent_thread(win, false)` when the new parent is the desktop.
  This matches the measurement (SetParent(child, NULL) keeps the shared
  input of the old host) and is as narrow as it can be: message-only and
  child-to-child reparenting still detach, and the attach call after the
  change is a no-op for a top-level window (`is_toplevel`), so the window
  gains no new association. Approved.

## Findings

- **P2: header is out of date.** Its v2 paragraph says native L1 and
  paired L2 are pending; L1 passed (10195 checks per architecture, 0
  failures, `99e45cd`). Update it when the L2 result is in.
- **P3: an unmeasured follow-on.** After SetParent(child, NULL) the
  retained association has no window that owns it: a later SetParent of
  that window to a child of a third thread attaches the new host without
  detaching the old one (the early return in `attach_parent_thread` for a
  top-level window). Plausibly what Windows does, given that it keeps
  the association even after the child is destroyed, but not measured.
  Record it as an open point; no change requested.
- **P3: foreground dependence.** The test calls `SetForegroundWindow`
  (one site). It passed natively; under Xvfb in CI foreground changes can
  be refused. If the paired L2 shows intermittent `user32:attachment`
  failures, this is the first place to look.

The two `todo_wine_if` uses (direct destruction does not split in Wine;
self-detach returns access denied instead of invalid parameter) are
limited to the assertions Wine does not reach and are described in the
header. No hidden regression found.

## Verdict

Approved, subject to the paired L2 of `test-run/batch4-1`
(42695491325), which was still running at review time: the new
`user32:attachment` test must pass on the series side and the
neighbouring user32 and server suites must show no new failures. Then
update the header (P2) and the page marker after the patch replaces v1.
