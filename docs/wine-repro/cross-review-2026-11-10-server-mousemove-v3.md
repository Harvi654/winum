# Cross review: server mouse move after show/hide v3 (2026-11-10)

Reviewer: Codex; candidate author: Claude. Test revision 2f22ffe, revised
v2 default 6923ee6. Scope: the new user32:mousemove test, its measured
oracles and CI timing, plus confirmation that server behavior is unchanged
from the current 32-ms v2. This is not a fresh audit of the whole server.

Read in full: v3 patch/README, current v2 patch, the two revision diffs,
MEASURE source, both complete Windows output files, investigation README,
patch page and prior v2 review. Revisited pinned Wine 11.20
7298a818b79446bae153b8d5aa27235ab3703bf9 mouse-message merging and the
existing user32 event-flush pattern. Checked actual all-2 patch identity,
series applicability, group and native-bundle routing. Unrelated user32
suites and the inherited timer implementation beyond the changed/default
comparison were skimmed or retained from the previous review. Only open
source and our own synthetic measurements were used; no vendor internals.

## Native contract and narrow todo

All 18 selected scenario counts and recipients match the x86_64/i686
MEASURE. The original same-position SetCursorPos control is explicitly
omitted, not assigned an unmeasured Wine expectation. Idle, repaint and
the two outside-window cases expect zero messages **to the fixture**;
this does not assert that other windows receive no mouse messages.

The hidden-move todo is supported by the actual operation history:
move_toplevel_away runs after b_away was hidden, with no intervening show.
Native Windows nevertheless records one move to A. The patch changes
visibility transitions and preserves Wine's visible-window move path;
it does not implement this hidden-window move. Only that count assertion
is todo_wine_if(TRUE); recipient checks, if a move arrives, remain ordinary.
No show/hide/create/destroy failure is hidden by it.

For create_child_under_visible the native log calls both the hit-tested
window and recorded receiver "other". The source resolves that label:
only our registered class records messages, the five persistent fixture
HWNDs all have explicit tags, and the newly created temporary child is
the only untagged live fixture window. Its measured position covers the
cursor. Thus the test's tmp recipient is supported by the measurement
and fixture source rather than an assumption about an arbitrary window.

## Findings

- **P1: no new memory-safety defect found in the bounded test delta.**
  The receiver array is capped at 16; saturation still fails expectations
  of zero/one. No worker-thread lifetime or asynchronous stack data is
  introduced. The server hunks match the revised v2, apart from separating
  patch whitespace; the v3 addition does not alter server ownership.
- **P2: the pump can expire before dispatching an already-ready message.**
  pump_messages computes end, then tests wall-clock expiry before every
  PeekMessage drain. If the client is descheduled across that deadline
  while the separate server queues the correct move, it returns without
  retrieving that move. At check_moves this falsely reports zero; at
  start_scenario it clears the counter with the old event still queued,
  potentially charging that event to the next scenario. Increasing the
  nominal wait to 800 ms reduces the probability but does not fix the
  control-flow hole. A host-only model compiled the exact unmodified pump
  against a simulated client stall: configured_wait=800, elapsed=1000,
  dispatches=0, queued=1. This demonstrates the source path, not an
  observed failure of the actual CI/Wine run.

  Before promoting the test, ensure ready messages are drained at least
  once and after an overdue wakeup, including a bounded final drain before
  counts are assessed/reset. Also bound the inner PeekMessage drain: it
  currently has no deadline check if the queue remains busy. Preserve the
  measured counts/recipients and finite overall guard; do not replace them
  with a 32-ms latency assertion or convert failures into skips. The exact
  revised body still needs its own native L1, which is already pending.
- **P3: initial environmental skips must not masquerade as coverage.**
  The initial GetCursorPos/WindowFromPoint guard legitimately skips when
  the cursor cannot be placed over A, before testing any layout action.
  A finished unit with only its two setup checks and this skip is not a
  successful regression run. Inspect the actual coverage on both
  architectures: require zero scenario/environment skips, not just a green
  compare badge. The documented complete patched local result is 33
  checks/one todo; a complete native pass additionally checks the hidden
  move's recipient. Do not infer successful new native L1 from the older
  MEASURE or the author's three local patched runs.
- **P3: the older investigation summary mislabels the move as visible.**
  Its table says "move a visible top-level window"; the source and v3
  correctly identify it as hidden. Correct that historical wording when
  updating the investigation. This is not a reason to remove the todo.

## Default delay and CI limits

No assertion compares elapsed time with 15, 16, 24 or 32 ms. That is the
right scope: the native logs record receipt at tick granularity, not an
exact internal timer value. The 32-ms default is supported by the
maintainer's Inventor L4 comparison (16/24 fail, 32/100 work), not proved
to be Windows' timer setting. Full all-2-build L4 is still open.

The test covers one GUI thread/desktop and a fast burst. It does not close
the prior v2 findings about server-global cross-desktop cancellation,
allocation-failure reference cleanup or timer expiry as a client-message
ordering barrier. It also does not prove that all neighboring strict
message-sequence tests are immune to delayed events. Those limits are
inherited, not new defects introduced by this unit; retain their record.

## Checks, run identity and verdict

V3 is byte-identical to the production-position copy in test-run/all-2 at
04ae7080574a734b63f3870764f5f80d89afe6dd. All 64 actual branch entries
apply to pinned Wine with --fuzz=0. Both new PE object targets compile
under Wine's headers with -Werror; this is not a full DLL/EXE L0 build or
Wine/native test execution. The timing model executes only host-side
synthetic functions, not Wine.

[Run 42815100059](https://github.com/Harvi654/winum/actions/runs/42815100059)
was in progress, 0/10 test jobs completed, when the public page was
checked on 2026-11-10. No completed L2 result or native v3 unit output was
available. The actual branch packs user32:mousemove in its L1 bundle and
runs user32 with neighboring suites. Its presence in the branch does not
establish execution without inspecting the eventual unit summary/skips.

Verdict: **native oracles and hidden-move todo are correct; request the
P2 pump robustness adjustment before approving the new test for promotion**.
Keep revised native L1, completed neighboring paired L2 and all-2-build
Inventor L4 open. No patch implementation, series, marker, test body or
other author's test-run branch was changed by this review.
