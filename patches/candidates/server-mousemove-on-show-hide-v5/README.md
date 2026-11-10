# server-mousemove-on-show-hide v5 (candidate)

v4 with the test's counts bounded after two Windows runs. Server change
unchanged (32 ms default, L4 in Inventor passed). Written by Claude,
2026-11-10.

## Why

Windows 10 x86_64 sent more than one `WM_MOUSEMOVE` in two of three runs
of the test (all-2 v3: 6 after showing a child under the cursor; all-3
v4 run 1: 2 after five show/hide pairs), always to the measured window;
i386 sent one in all three runs, as did the MEASURE. A test that fails on
Windows is wrong: v5 expects at least one move to the measured window
after a single change and one to nine after the burst of ten changes
(fewer than one per change); the scenarios with no move stay exactly 0;
the hidden-window move stays `todo_wine`.

## Local run (2026-11-10)

Wine 11.20 x86_64, Xvfb: vanilla 20 checks, 13 failures; with the patch
33 checks, 0 failures, two runs.

## Open

- L1 of v5 (bundle of `test-run/all-4`), L2 (`all-4`).
- Codex: re-read of the bounds.
