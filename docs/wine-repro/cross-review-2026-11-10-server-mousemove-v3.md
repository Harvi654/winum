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

## V4 follow-up (2026-11-10, 13c9d85)

Read in full: v4 patch/README and v3-to-v4 diff, the corrected investigation
wording, and the two returned native v3 summaries plus all-2 L1 README.
The server hunks, all actions/expected recipients and the single hidden-
window count todo are unchanged; only the message pump and its explanation
change in the test. No unrelated server/user32 source was re-audited.

- **P2 addressed for the demonstrated stalled-client path.** The pump
  drains before checking elapsed time and after every wait, including an
  overdue wakeup. It no longer exits solely because the deadline passed
  before its first drain. The inner loop dispatches at most 1000 messages
  per batch. The existing 400/800-ms budgets and absence of a latency
  assertion are retained. Host-only models using the exact new function
  dispatch the ready move both after preemption before the first drain
  and after an overdue wait; unsigned elapsed arithmetic also handles a
  32-bit tick wrap. Both PE object builds pass -Werror with Wine headers.
  These are compilation/model checks, not new Wine or Windows execution.
- **The historical visible/hidden P3 wording is corrected.** The MEASURE
  source always moved the already-hidden top-level window; the README now
  says so. The one todo remains justified and no assertion is weakened.
- **P3, bounded-drain limit:** "never left for the next scenario" is too
  absolute if a final batch saturates. With 1001 queued messages and an
  expired budget the new function dispatches 1000 and returns with one
  still queued (confirmed in the host model). This does not reproduce a
  problem in the small quiet fixture, but qualify the guarantee by the
  cap; if that cap is hit in practice, report busy/incomplete collection
  instead of treating it as established clean separation. A finite cap
  is reasonable; no unbounded draining is requested.

Native **v3**, not v4, now has actual L1 output from all-2. i386 passes
34 checks, zero failures/skips and exit 0. x86_64 executes 39 checks with
one failure: show child under receives six WM_MOUSEMOVE rather than one.
There is no recipient assertion failure; the five extra receiver checks
explain the higher check count. The remaining scenarios, including the
native hidden-window move, match the measured assertions. Consequently
v3 does not have a two-architecture L1 pass, and none transfers to v4.

Host mouse input in the VM is plausible, **not confirmed**. The summaries
contain neither per-event input provenance nor coordinates/timestamps, so
they cannot identify the source of the five extra moves. The v4 README's
categorical statement that the old pump cannot explain extras because it
only loses messages is stronger than the evidence: delayed collection
can misattribute events to a later scenario, as the original P2 noted.
That does not establish it caused these particular six messages either.
Keep the cause open, keep count=1 ordinary, and do not convert this native
failure into a todo, accepted flake or relaxed count.

The planned v4 native run twice on both architectures, without host mouse
interaction, is the appropriate next check. If excess moves recur, obtain
new public-API scenario measurements before changing the oracle. Require
zero skips and complete scenarios, not just exit status. All-2's source
contains v3 and cannot establish v4 runtime validation; paired L2 with v4
and the final 32-ms Inventor L4 remain open.

Verdict: **the requested P2 pump fix is acceptable in source; no new P1/P2
blocker identified in this delta.** The cap wording is a remaining P3,
and the unexplained native count failure is an open validation gate, not
a proven implementation defect or proven host-input incident. No candidate,
implementation, marker or production series was changed by this follow-up.
