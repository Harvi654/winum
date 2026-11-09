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

`dlls/comctl32_v6/button.c`, `WM_LBUTTONDOWN`: `SetCapture` before
`SetFocus` for every non-split button; split buttons keep their order
(focus, dropdown check, capture), which was not measured.

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

(Wine columns: expected from the source; to be confirmed by the local run
and L2.) Also: after the down the button has capture and focus, after the
up one `BN_CLICKED` and no capture. The test skips when the parent cannot
be made the foreground window.

## Open points

- **SOLIDWORKS L4 is required again**: v2 changes behaviour that v1 fixed
  in a different way. If the PropertyManager buttons do not react with v2,
  the next step is a `+msg` trace of one click (filtered) and a MEASURE of
  whatever the superclass does differently - not a return to v1, which
  contradicts Windows.
- The order for push buttons and check boxes is not measured (no parent
  drawing); v2 changes them too, consistent with user32. A follow-up
  MEASURE that logs `WM_SETFOCUS` would close this.
- Inventor uses the same v6 buttons; its smoke test covers this too.

## Validation of v2

- L0: the full series applies at `--fuzz=0` on Wine 11.20 with this file
  substituted (with the toolbar and header v2 candidates), 62/62
  (2026-11-09); the later themed-checkbox patch still applies.
- L1 of the new test, L2, L4 (SOLIDWORKS, Inventor), cross review (Codex):
  pending.
