# server-mousemove-on-show-hide v2 (candidate)

Replaces `patches/server-mousemove-on-show-hide.patch` after review and
validation. Written by Claude, 2026-11-08 (v1 from Autodesk-Inventor-for-Linux
`e236679`).

Why: the L2 runs of 2026-11-08 failed intermittently in comctl32 button and
user32 mouse tests with a stray `WM_MOUSEMOVE`. The MEASURE in
`docs/wine-repro/server-mousemove-show-hide/` shows Windows sends the same
move but within 15-32 ms, not 100 ms. v2 uses 16 ms and a header per
AGENTS.md; nothing else changes.

Open: cross review (Codex); L2 - the intermittent failures must be gone in
several runs; L4 - Inventor's sketch dimension still updates without moving
the mouse; a Wine test (batch 4) from the measured scenarios.

## L2: test-run/all-1 (2026-11-10)

[docs/wine-repro/l2-all-1-2026-11-10](../../../docs/wine-repro/l2-all-1-2026-11-10/README.md), run 42788810738, with every current candidate: no new failure in the touched DLLs, both architectures.

## L4, 2026-11-10 (Inventor 2027, build of test-run/all-1)

With the 16 ms default the sketch did not redraw after a dimension edit
until the mouse moved, so the 16 ms version did not fix the user's
problem. Same build with `WINE_SHOWHIDE_MOUSEMOVE_MS` (read by wineserver
at start, `wineserver -k` before each try): 16 and 24 ms fail, 32 and
100 ms work. The default is now 32 ms, the upper end of what Windows
showed (15-32 ms). L4 of the 32 ms default itself is still to be done
with the next build; the Wine test must accept a move within that delay.
