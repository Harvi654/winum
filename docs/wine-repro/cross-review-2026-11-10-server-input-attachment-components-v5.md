# Cross review: server input attachment components v5 (2026-11-10)

Reviewer: Claude; candidate author: Codex. Candidate as on main `f64f3b9`.
Follows the v2 review (`cross-review-2026-11-09-server-input-attachment-components-v2.md`).

Read in full: the v4 -> v5 diff of the patch (header paragraph and the
test's `snapshot()` changes), the whole v5 `snapshot()` function, and the
README. Checked that the `server/` hunks are byte-identical in v2, v3, v4
and v5, so the server change approved in the v2 review is unchanged. The
v2 -> v4 test changes (rounds 1 and 2, about 400 diff lines) and the 592
literal rows were skimmed, not re-read line by line; the native and L2
numbers are taken from the README and its linked records.

## Findings

- **P1: none.** v5 changes the test only, at one place: for the two
  `children-late1-*` / `one-child` checkpoints the exact active/focus
  window of the reference thread becomes `todo_wine_if`. Wine keeps host
  A's window there, Windows the creator C's; that is decided by the
  create/attach path, which this patch does not touch (correct, checked:
  no `server/` change since v2).
- **P2: none. The todo does not hide a membership failure.** At those two
  checkpoints the observed window is used as the component's witness
  only after ordinary `ok()`s prove it is the top-level window of a live
  worker whose measured row is in the same component as the reference.
  NULL, a child window or a window of B/D fails there. The per-observer
  relations (A and C share it, B and D do not) stay ordinary and now
  compare against that witness, and capture stays unconditional. So a
  missing or extra attachment still fails. Elsewhere the exact native
  handles are required as in v4. On Windows the witness is C's window,
  so the same predicates pass there (native L1: 16719 checks, 0
  failures, both architectures).
- **P3: `missing_absolute_choice` uses the active row for both the active
  and the focus todo** in the optional absolute view. In these two rows A
  and C have the same expected active and focus, so it is equivalent;
  if more rows were added, a focus-specific condition would be cleaner.
  The absolute view does not run in CI anyway (prerequisite probe fails
  under Xvfb, reported by `skip`, as in v4).
- **P3: when Wine's window choice is fixed upstream,** the four todos turn
  into "test succeeded inside todo block" failures - the intended signal.

## Checks and verdict

All series entries apply at `--fuzz=0` with this candidate substituted,
also with every current candidate together (`test-run/all-1`, 64
entries). Native assertion L1 of the v5 body passed; the focused paired
L2 (run 42772336963) is green with 16 todos and full component coverage
(README). Neighbouring-suite L2 (user32 group g1 and the rest) comes from
`test-run/all-1` (run 42788810738). No Wine runtime run by this reviewer.

Verdict: **approve.** Promotion after `test-run/all-1` shows no new
failure in user32 and the other groups; Inventor L4 as the README says,
SOLIDWORKS L4 deferred.
