# WM_MOUSEMOVE after window changes on Windows (MEASURE)

For `server-mousemove-on-show-hide`, whose generated mouse moves make
Wine's own comctl32 button and user32 mouse tests fail in about half of
the L2 runs. `measure.c` puts the cursor over window A, does one action per
scenario, pumps messages for 400 ms and prints every `WM_MOUSEMOVE` with the
receiving window and the delay.

## Windows 10 (VM on a Mac, 2026-11-08), x86_64 and i686 the same

| Scenario | Moves | Delay |
| --- | --- | --- |
| nothing | 0 | - |
| `SetCursorPos` to the same position | 1 to the window under the cursor | 16-31 ms |
| show / hide a top-level window away from the cursor | 1 to A | 15-32 ms |
| show / hide a top-level window under the cursor | 1 to the window then under the cursor | 16-31 ms |
| show / hide a child away from / under the cursor | 1 (A, or the child when under it) | 15-32 ms |
| 5 show/hide pairs in a row | **1** | 31 ms |
| move a hidden top-level window away from the cursor (B was hidden in the scenario before; corrected 2026-11-10, cross review P3) | 1 to A | 16 ms |
| create / destroy a visible child (away or under) | 1 | 16-32 ms |
| `InvalidateRect` + `UpdateWindow` on A | 0 | - |
| cursor outside our windows, show / hide | 0 to our windows | - |

Windows sends **one** `WM_MOUSEMOVE` to the window under the cursor after
any change of the window layout (show, hide, create, destroy, move) and
after `SetCursorPos`, coalescing a burst, and delivers it **within one or
two timer ticks (15-32 ms)**. A repaint alone sends none.

The patch matches the "one move per burst after show/hide" part but waits
**100 ms**. Wine's tests pass on Windows, where the move arrives within
~16-32 ms and is consumed by the tests' own message flushing; a move 100 ms
later lands inside the next expected message sequence, which is exactly the
failure seen ("expected WM_PAINT, got WM_MOUSEMOVE"; "message 0200
available"). Next: v2 with the delay of one tick (16 ms) instead of 100 ms,
then L2 (the intermittent failures must disappear) and L4 (Inventor's
sketch dimension still updates without moving the mouse). A Wine test from
these scenarios (show/hide away from the cursor gives one move within
100 ms) for batch 4.
