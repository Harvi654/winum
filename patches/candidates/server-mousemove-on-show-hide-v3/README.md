# server-mousemove-on-show-hide v3 (candidate)

Replaces `patches/server-mousemove-on-show-hide.patch` after review and
validation. v3 = the v2 server change unchanged (32 ms default since the
Inventor L4 of 2026-11-10) plus a Wine test. Written by Claude,
2026-11-10.

## Test

`dlls/user32/tests/mousemove.c` (new unit `user32:mousemove`), from the
MEASURE in `docs/wine-repro/server-mousemove-show-hide/` (Windows 10,
x86_64 and i686, 2026-11-08), with the same window layout: window A with
the cursor at its centre, a top-level popup and a child away from the
cursor, and one of each under it. Each scenario waits 400 ms, does one
action, pumps messages for 800 ms and counts `WM_MOUSEMOVE`:

| Scenario | Expected (Windows) |
| --- | --- |
| nothing | 0 |
| show / hide top-level away | 1, to A |
| show / hide top-level under | 1, to the popup / to A |
| show / hide child away | 1, to A |
| show / hide child under | 1, to the child / to A |
| five show/hide pairs of a child | 1, to A |
| move a hidden top-level | 1, to A (`todo_wine`: Wine sends a move only for visible windows; outside this change) |
| create / destroy a visible child away | 1, to A |
| create / destroy a visible child under | 1, to the new child / to A |
| repaint (`InvalidateRect` + `UpdateWindow`) | 0 |
| cursor not over the test windows, show / hide | 0 |

The delay is not checked: Windows needed 15-32 ms, the patch waits
32 ms, and a loaded CI machine may be later; 800 ms leaves room. If the
cursor cannot be placed over A, the test skips (an environment limit,
not a Wine failure). `SetCursorPos` to the same position (one move on
Windows) is not part of the test.

On Windows the mouse must not be touched while it runs (about 30 s).

## Local run (2026-11-10)

Wine 11.20 x86_64, built locally, Xvfb 1280x1024:

| | `user32:mousemove` |
| --- | --- |
| vanilla server | 20 checks, 13 failures (every show/hide/create/destroy scenario: no move), 1 todo; same in two runs |
| with the patch | 33 checks, 0 failures, 1 todo; three runs |

## Open

- L1 (`user32:mousemove` in the L1 bundle of `test-run/all-2`).
- L2: `test-run/all-2`, with the neighbouring suites (user32, comctl32
  button, which failed intermittently with v1).
- Cross review (Codex) of the test.
- L4 of the 32 ms default with the all-2 build.

**Superseded by `../server-mousemove-on-show-hide-v4/` (2026-11-10)**:
message pump fixed after cross review; v3's Windows results are recorded
there.
