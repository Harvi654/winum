# server-mousemove-on-show-hide v4 (candidate)

v3 with the test's message pump fixed after Codex's cross review
(`docs/wine-repro/cross-review-2026-11-10-server-mousemove-v3.md`, P2):
the pump drains the queue after every wait, also a wait that ends past
the deadline, so a move that is already queued is counted in its own
scenario and not cleared into the next one; the inner drain is bounded
(1000 messages). Server change, scenarios and expectations are v3's
(see `../server-mousemove-on-show-hide-v3/README.md` for the table).
Written by Claude, 2026-11-10.

## Local run (2026-11-10)

Wine 11.20 x86_64, Xvfb: vanilla server 20 checks, 13 failures, 1 todo;
with the patch 33 checks, 0 failures, 1 todo, three runs.

## v3 test on Windows 10 (L1 bundle of test-run/all-2, 2026-11-10)

- i686: 34 checks, 0 failures, exit 0 (the hidden-window move arrived,
  so its recipient check ran too).
- x86_64: 39 checks, **1 failure**: "show child under: got 6
  WM_MOUSEMOVE, expected 1"; all six went to the child, every other
  scenario as measured. The MEASURE (x86_64 too) had one move there. Not
  explained by the pump (that would lose moves, not add them); the most
  likely source is mouse input from the VM host while the new child
  appeared under the cursor. Not relaxed: v4 goes to the next VM visit,
  both architectures run twice; if the extra moves come back, the
  scenario is measured again before the test changes.

## Open

- L1 of v4 (next VM visit, with the setrows MEASURE).
- L2 with v4 (next test run); all-2 ran v3.
- Codex: re-read of the pump change.
- L4 of the 32 ms default with the all-2 build.

## L4, 2026-11-10 (Inventor 2027, build of test-run/all-2)

The 32 ms default works: after a dimension edit the sketch shows the new
value without moving the mouse (maintainer).

## L1 (all-3, 2026-11-10) - superseded by v5

`docs/wine-repro/l1-all-3-2026-11-10/`: i386 34 checks, 0 failures, two
runs; x86_64 run 2 0 failures, run 1 "five show/hide pairs: got 2,
expected 1". **Superseded by `../server-mousemove-on-show-hide-v5/`**
(counts bounded).
