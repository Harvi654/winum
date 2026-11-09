# Owner-drawn button and an existing mouse capture: Windows MEASURE

Original, AI-assisted (Claude), LGPL-2.1-or-later. Patch:
`patches/comctl32-ownerdraw-existing-capture.patch` (in `comctl32_v6`'s
`BUTTON_WindowProc`, `WM_LBUTTONDOWN` of a `BS_OWNERDRAW` button does not call
`SetCapture` again when the button already holds the capture). Read before
writing: the patch, Wine 11.20 `dlls/comctl32_v6/button.c`
(`WM_LBUTTONDOWN`, `WM_LBUTTONUP`, `WM_CAPTURECHANGED`) and
`set_capture_window` in `dlls/win32u/input.c`, which sends
`WM_CAPTURECHANGED` to the previous capture window even when it is the
same window. SOLIDWORKS' PropertyManager buttons (checkmark, red X) capture
the mouse in their own `WM_LBUTTONDOWN` before calling the button procedure;
the extra `WM_CAPTURECHANGED` cancelled the press, so no `BN_CLICKED`.

Measured, user32 buttons (v5) and comctl32 v6 buttons (`CreateWindow` of
`Button` with the v6 activation context active), types `BS_OWNERDRAW`
(changed by the patch), `BS_PUSHBUTTON` and `BS_AUTOCHECKBOX` (neighbours):

- `pre=none`: nobody holds the capture;
- `pre=self_down`: the subclass calls `SetCapture(button)` in its
  `WM_LBUTTONDOWN` before forwarding (the SOLIDWORKS case);
- `pre=self_before`: the button holds the capture before the message;
- `pre=other`: another window of ours holds it.

`WM_LBUTTONDOWN` and `WM_LBUTTONUP` are sent with `SendMessage` (no real
input). The subclass logs `WM_CAPTURECHANGED` (and the new capture window),
`BM_SETSTATE`, and `GetCapture`/`BM_GETSTATE` after each message; the
parent logs `WM_COMMAND` notifications and `WM_DRAWITEM` (`ODS_SELECTED`).
`AFTER_UP` gives `clicked=` (the number of `BN_CLICKED`). `PLAIN` lines
measure the neighbouring user32 behaviour: `SetCapture` twice on a plain
window, then on another one (does the repeat send `WM_CAPTURECHANGED`?).

What the Wine test needs from this: whether the subclass (standing in for
the owner-draw superclass) gets a `WM_CAPTURECHANGED` during
`WM_LBUTTONDOWN` when the button already holds the capture, and that the
click still completes. The button's own `BN_CLICKED` alone does not show
the bug: Wine's button ignores a `WM_CAPTURECHANGED` naming itself, the
superclass does not.

Local check (Wine 9.0, not an oracle): with `pre=self_down` and
`pre=self_before` the subclass gets `WM_CAPTURECHANGED new=button` inside
`WM_LBUTTONDOWN` (v5 and v6), and the repeated `SetCapture` on a plain
window sends `WM_CAPTURECHANGED` to that same window; `BN_CLICKED` arrives
in every case.

## Windows observations (2026-11-09)

Offline Windows 10 VM, `windows-2026-11-09/` (both architectures complete,
`api_errors=0`, identical apart from `bits=`). comctl32 5.82 (user32
buttons) and 6.16 (v6 buttons):

- `PLAIN`: a repeated `SetCapture` on the window that holds the capture
  sends it `WM_CAPTURECHANGED` naming itself - as in Wine.
- `pre=self_down` and `pre=self_before`, every button type, v5 and v6: the
  button gets `WM_CAPTURECHANGED new=button` inside `WM_LBUTTONDOWN` - as
  in Wine. So patch v1, which skips that `SetCapture` for owner-drawn v6
  buttons, makes Wine differ from Windows.
- The order differs from Wine's v6 button: on Windows the
  `WM_CAPTURECHANGED` (to the button itself, or to `other` with
  `pre=other`) comes **before** the parent's `WM_DRAWITEM` with
  `ODA_FOCUS`, then `BM_SETSTATE(TRUE)`; the button takes the capture
  before the focus. Wine's v6 button calls `SetFocus` first (Wine 9 local
  run: `WM_DRAWITEM` focus, then `WM_CAPTURECHANGED`); Wine's user32
  button already takes the capture first.
- Every case ends with one `BN_CLICKED` and no capture; `BM_GETSTATE` after
  the down is 0x6c (Wine 9: 0x4c, the hot bit; not part of the patch).
- Not measured: the focus order for push buttons and check boxes (they do
  not draw through the parent, and `WM_SETFOCUS` was not logged).

Consequence: the candidate `comctl32-ownerdraw-existing-capture-v2`
replaces v1 with the Windows order. Whether that is what SOLIDWORKS needs
is a hypothesis (its superclass may arm the press on the focus and disarm
it on the capture change, so only the order matters) - its smoke test
decides.
