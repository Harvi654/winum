# winex11: a captionless popup shown without activation stays managed

Original synthetic C reproducer, written with AI assistance; LGPL 2.1 or later.
No Autodesk, DevExpress or Microsoft code, binaries, symbols or disassembly
were used.

`menu.c` creates three windows the way DevExpress creates its menus in Vault
Explorer 2027 (seen with the trace patch that logs which rule of
`is_window_managed()` made a window managed):

| Window | Created | Before the first show | Shown with |
|---|---|---|---|
| `caption` (control) | overlapped, style `04c00000` after creation | unchanged | `SWP_SHOWWINDOW \| SWP_NOACTIVATE`, topmost |
| `menu` | overlapped, `WS_EX_TOOLWINDOW` | style set to `WS_POPUP \| WS_CLIPSIBLINGS` (`94000000` when shown) | same |
| `menu-layered` | the same plus `WS_EX_LAYERED` | the same | same |

On Windows a popup without caption, frame or sysmenu that is shown without
activation is an ordinary popup. Wine marks every overlapped window managed at
creation (it always has `WS_CAPTION` then) and never switches it back, so the
menus become managed X windows. Under KWin they end up below KWin's own
override-redirect shadow window: the menu opens, but is not visible.

`check.sh` runs the program and asks the X server about each window
(`check.py`, libX11 through ctypes): override-redirect or managed, and whether
the window manager lists it in `_NET_CLIENT_LIST`.

## Build and run

```sh
x86_64-w64-mingw32-gcc -O2 -Wall -o menu.exe menu.c -lgdi32 -luser32
WINEPREFIX=/tmp/wxr-prefix bash check.sh ./menu.exe
```

Use a new throwaway prefix, not a CAD prefix. The three grey windows are shown
for four seconds; don't click them.

## Results, 2026-11-07

Arch (CachyOS), KDE Plasma 6 on Wayland with Xwayland, Wine 11.20 with the
Inventor project's winex11 patches (`wine-11.20-6`):

| Window | Unpatched | With the candidate |
|---|---|---|
| `caption` | managed, WM client | managed, WM client |
| `menu` | managed, WM client | override-redirect |
| `menu-layered` | managed (not yet listed by the WM) | override-redirect |

In Vault Explorer 2027 with the dark theme, all menus and context menus were
visible with the candidate, also when opened a second time.

## Paired L2, 2026-11-08

`user32_test.exe win` and `msg` (Wine 11.20, x86_64), each run twice with the
unpatched and the patched `winex11.so` (otherwise identical `wine-11.20-6`
runtime), fresh prefix per build, on the live KDE Plasma 6 / Xwayland desktop
(screen lock off, nobody at the machine):

| Build | `win` round 1 | `win` round 2 | `msg` round 1 | `msg` round 2 |
|---|---|---|---|---|
| unpatched | 6 failures | 7 failures | 0 | 0 |
| candidate | 7 failures | 7 failures | 0 | 0 |

The failing `win` lines are the same in both builds: 10689/10690 (mouse
hit test through a transparent window) in every run, and one focus or
activation block that varies from run to run (11079, or 11495/11506 with
`WM_ACTIVATEAPP`). Every line the candidate failed also failed without it.
An earlier run with the screen lock active failed more of the same
focus/activation tests in both builds and was discarded. The summary counts
are one or two higher than the five `Test failed` lines in each log; the
remaining failure was not attributed (the logs were not kept).
