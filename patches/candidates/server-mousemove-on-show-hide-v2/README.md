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
