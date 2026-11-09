# comctl32-ownerdraw-existing-capture v2: Windows order instead of a skip

Candidate only: `patches/series`, the production patch and its page marker
are unchanged. Substitute `comctl32-ownerdraw-existing-capture.patch` from
this folder for the production file at its place in the series (entry 45).

## Why v1 is replaced

v1 skipped `SetCapture` in the v6 button's `WM_LBUTTONDOWN` when an
owner-drawn button already held the capture, so its superclass got no
`WM_CAPTURECHANGED`. The Windows measurement
(`docs/wine-repro/comctl32-ownerdraw-existing-capture/`, results
`windows-2026-11-09/`) shows that Windows does send it - in v5 and v6, for
every button type. What differs is the order: Windows takes the capture
before the focus, so the capture change arrives before the focus is drawn
(`WM_DRAWITEM`, `ODA_FOCUS`); Wine's v6 button set the focus first.
Wine's own user32 button already takes the capture first.

## The change

`dlls/comctl32_v6/button.c`, `WM_LBUTTONDOWN`: an owner-drawn button
calls `SetCapture` before `SetFocus`. Other button types keep Wine's order:
theirs was not measured (no parent drawing to mark the focus), and a patch
for a paused program must not change what vanilla already does like
Windows (AGENTS.md).

## Test

`test_ownerdraw_capture` in `dlls/comctl32/tests/button.c` (v6): an
owner-drawn button subclassed like a superclass, the events in order -
`C` the button gets `WM_CAPTURECHANGED` naming itself, `O` another window
loses the capture to it, `F` the parent's `WM_DRAWITEM` with `ODA_FOCUS`,
`S` `BM_SETSTATE(TRUE)` - for four starting states:

| Case | Windows | vanilla Wine | v1 | v2 |
| --- | --- | --- | --- | --- |
| no capture | `FS` | `FS` | `FS` | `FS` |
| superclass captures in its `WM_LBUTTONDOWN` | `CFS` | `FCS` | `FS` | `CFS` |
| the button held the capture before | `CFS` | `FCS` | `FS` | `CFS` |
| another window held the capture | `OFS` | `FOS` | `FOS` | `OFS` |

(vanilla and v2 columns confirmed by the local run below; v1 from the
source.) Also: after the down the button has capture and focus, after the
up one `BN_CLICKED` and no capture. The test skips when the parent cannot
be made the foreground window.

## Open points

- SOLIDWORKS' L4 is deferred (AGENTS.md, 2026-11-09). When it is done:
  if the PropertyManager buttons do not react with v2, the next step is a
  filtered `+msg` trace of one click and a MEASURE of what the superclass
  does differently - not a return to v1, which contradicts Windows.
  Hypothesis: the superclass arms the press when the focus is drawn and
  disarms it on the capture change, so only the order matters.
- The order for push buttons and check boxes is not measured; they are
  unchanged.
- Inventor uses the same v6 buttons; its smoke test covers owner-drawn
  buttons there.

## Validation of v2

- L0: the full series applies at `--fuzz=0` on Wine 11.20 with this file
  substituted (with the toolbar and header v2 candidates), 62/62
  (2026-11-09); the later themed-checkbox patch still applies.
- L1 of the new test, L2, L4 (SOLIDWORKS, Inventor), cross review (Codex):
  pending.

## Local run (2026-11-09)

Wine 11.20 built locally for x86_64 (Ubuntu, X11 under Xvfb), test units run
with the new tests applied, first on vanilla DLLs, then with all six batch 1
v2 candidates. Quick feedback only; L2 is the CI run.

| | vanilla | v2 |
| --- | --- | --- |
| comctl32:button | 3 failures, all in `test_ownerdraw_capture` (`FCS`, `FCS`, `FOS`) | 12023 tests, 0 failures |

Round 1 of the fix: passed. (A first local version changed the order for
all non-split buttons; it was limited to owner-drawn buttons before
commit, the test result is the same.)
