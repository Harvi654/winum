# Cross review: server mouse move after show/hide v2 (2026-11-09)

Reviewer: Codex; candidate author: Claude. Candidate regenerated in `114e883`.

Read in full: production and candidate patches, candidate README, MEASURE
source and both Windows result files, investigation README and the full
l2-series-2026-11-08.md report. Checked Wine 11.20
`7298a818b79446bae153b8d5aa27235ab3703bf9`: cursor synthesis and
queue_hardware_message, set_window_pos request handling, timeout allocation,
cancellation and callback dispatch, and test_radiobutton_focus. Read the
affected test_mouse_input sequences, not the whole user32 win.c suite.
The rest of the server files was not audited. No vendor internals were used.

## Change and measurements

V2 changes the default delay from 100 to 16 ms and updates the comment/header.
Visibility-change detection, the callback, environment override, object
references, global timer state and cancellation/coalescing are unchanged.
The candidate is a replacement for v1, not a patch to apply after it.

The two native result files record one move for the tested layout changes,
one for a fast burst of five child show/hide pairs, none for repaint or the
idle baseline. Observed delays are 15-32 ms. A 16-ms relative timeout is a
reasonable experiment based on those observations; GetTickCount quantization
and the message pump mean they do not establish an exact native timer value
or a guaranteed delivery deadline. The outside-window scenarios establish
zero messages to the fixture, not zero mouse messages system-wide.

The callback uses current coordinates and routes through the normal hardware
queue; recipient selection, button/modifier state and RIDEV_NOLEGACY filtering
remain in that path. Holding a desktop reference protects its lifetime during
the pending timeout. Timer cancellation and callback execution are serialized
by the server event loop, not concurrent callbacks on worker threads.

## Findings

- **P1: no new memory-safety defect identified in the 100 -> 16 change.**
  This does not establish absence of behavioural regressions.
- **P2: elapsed time is not a client-message ordering barrier.** The timer
  starts when the server processes the visibility update. It does not wait
  for the requesting client to finish subsequent window-position/show/paint
  processing. Another GUI thread can retrieve the synthesized move while the
  changing thread is delayed or reenters a message pump. A 16-ms delay can
  therefore still precede completion of messages caused by the change. The
  native MEASURE logs mouse moves, not the surrounding window-message order;
  it does not establish the comment's stronger ordering claim. Measure that
  order under slow/reentrant and cross-thread changes before asserting it.
- **P2: faster debounce alone does not close the intermittent L2 finding.**
  The server still cancels/restarts the timeout on every visibility change.
  A flush can finish before the last timer expires; another sequence can
  then observe its move. A scheduler stall can also delay callback delivery.
  The comctl32 radio-button sequence expects WM_PAINT after WM_LBUTTONDOWN;
  user32's click sequence checks that no extra message remains. Both can
  still encounter a delayed move. Require repeated paired results and a
  same-build control with WINE_SHOWHIDE_MOUSEMOVE_MS=0 before declaring the
  regression removed. Do not relax those Windows-derived sequences.
- **P2 (inherited): debounce state is server-global, not per desktop.**
  A visibility change on desktop B cancels desktop A's pending update and
  releases A's reference, so A can lose its move. Repeated changes spaced
  less than 16 ms also postpone delivery indefinitely. The single-desktop,
  fast-burst MEASURE does not establish either behaviour as native. V2 does
  not introduce these limitations, but they bound any compatibility claim.
- **P3 (inherited): timeout-allocation failure is not unwound.**
  add_timeout_user can return NULL. After grab_object, that leaves a held
  desktop reference with no callback; a later call overwrites it because
  cursor_pos_timeout is NULL. Fix the ownership path separately before
  treating this as a fully reviewed timer implementation. The environment
  override's unvalidated atoi conversion is also unchanged.

## Applicability and Actions run 42552802374

Applied the full 62-patch series to a separate Wine 11.20 tree, replacing
toolbar setrows, msxml3 filepos and mouse move with the current candidates.
All applied with `--fuzz=0`. The production series was not edited.

Directly inspected [run 42552802374](https://github.com/Harvi654/winum/actions/runs/42552802374)
on 2026-11-09: **In progress, 0/10 test jobs completed**, comparison pending.
The run points to `b8d22e647cee2513ab5d744054c1fb121442b6e4` on
`test-run/candidates-2`. Fetched that branch and verified that its three
replacement patch files are byte-for-byte identical to the current
candidates. Thus it is the relevant input, but it has no completed L2 result
at this review's observation time. No pass, failure count, or disappearance
of button.c:2631 / user32 win.c:4866-4882 is inferred from a running job.

## Verdict

The bounded default-delay change is supported as an experiment by the native
timing samples. Approval for production promotion is withheld: the recorded
intermittent failures are not yet resolved by completed repeated L2 results,
the ordering claim is stronger than the measurement, and a Windows-tested
regression test plus Inventor dimension-update L4 remain open. Add the final
run report when available and inspect the two affected units before closing
the L2 finding. This review did not build or execute Wine locally.
