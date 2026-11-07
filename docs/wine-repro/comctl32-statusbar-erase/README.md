# MEASURE: status-bar background erase and old text

Original AI-assisted (Codex), LGPL-2.1-or-later. Windows 10 results: 2026-11-07,
see [Windows results](#windows-results-2026-11-07).
This is a public-API observation program, not a Wine test with guessed expected
values. No correction to `patches/comctl32-statusbar-erase-background.patch`
or change to a series is part of this work.

Inventor users report status text drawn over old text (Autodesk-Inventor-for-Linux
PR #3). The current patch gates background painting in `WM_PAINT` on `ps.fErase`.
The hypothesis is that non-erasing invalidations can repaint text without
clearing its previous pixels. This bundle measures Windows before choosing a fix.

## Build and run

```sh
bash scripts/build-measure-comctl32-statusbar-erase.sh /path/to/NEW-bundle
```

Requires Python 3 and both MinGW GCC/windres toolchains. Builds four static
executables: x86_64/i686, each with an embedded v5 or v6 manifest. The v5
manifest has no common-controls dependency; v6 requests version 6. The actual
status-bar class module is queried through `DllGetVersion`, and a mismatched
major version makes the run incomplete. No Wine source tree is required.

Copy the whole bundle into a new folder in the offline Windows 10 VM and run
`run-measure.bat` as a normal user. Use a visible, unlocked, quiet desktop;
keep the small measurement windows unobscured and do not interact with them.
They are temporarily topmost. Both EXE architectures run on Windows 10 x64.
The batch file refuses to overwrite existing result files. Return only:

- `result-x86_64-v5.txt`, `result-x86_64-v6.txt`;
- `result-i686-v5.txt`, `result-i686-v6.txt`;
- `exitcodes.txt`.

Exit 0 means 144 observation rows were collected without a required setup/capture error;
it does **not** mean any particular erase behaviour is correct. Exit 2 means
incomplete measurement. The program writes no files itself, exports no registry,
and captures pixels only from its own synthetic status bar. The bundle contains
only our source/manifests, four EXEs, license, instructions and checksums.

## Matrix and preparation

Each executable tests default theming and per-control `SetWindowTheme("", "")`,
each with two subclass policies: delegate erasing, or fill the whole client
dark (`#182028`) on `WM_ERASEBKGND` and return 1. `CONFIG` reports the actual
theme handle, class version and `theme_hr` (the optional theme-disable HRESULT).
`classic=1` records the request, not a promise that it succeeded: a failed
`theme_hr` means those rows must not be treated as disabled-theme results.
For default-theme configs the call is not made and `theme_hr` is zero.
Each `CONFIG` applies to the following rows up to the next `CONFIG`.
Three part styles are exercised: normal text
(`style=0`), `SBT_NOBORDERS` (256), and `SBT_OWNERDRAW | SBT_NOBORDERS` (4352).
In the owner-draw case our parent draws white text transparently, without filling
the background. That case measures background delegation, not native text drawing.

The old text is sixteen `M`s; the new text is `I` or empty. A fixed 20-pixel
non-antialiased font makes pixel comparisons easier; actual glyph pixels are
derived from captures, not assumed from a font file. Three clean references
are drawn in the same control: old, short, empty. Each clean reference starts
with an explicit dark fill, then a synchronous erasing repaint.

For `settext`, the old text is visible and `SB_SETTEXTW` runs normally.
`send_paint`/`send_erase` count messages before that send returns; totals also
include a subsequent `UpdateWindow` to service any queued invalidation.

For other operations, `WM_SETREDRAW(FALSE)` brackets `SB_SETTEXTW`, followed by
`WM_SETREDRAW(TRUE)` and `RDW_VALIDATE | RDW_NOERASE | RDW_NOINTERNALPAINT`.
This stages new logical text while retaining old screen pixels and clears the
text setter's own pending erase request. `prep_paint`/`prep_erase` disclose
messages during staging; **`prep_diff=0` and `update_before=0` are prerequisites
for interpreting a staged row**. A nonzero difference means staging changed the
old image; do not infer a no-erase bug from that row.

Before the measured operation, a 24x8 magenta marker (`#cb118b`) is drawn well
to the right of the short text. A usable capture has `marker_before=192`.
The operations are:

| `op` | Operation after preparation |
| --- | --- |
| `settext` | Ordinary `SB_SETTEXTW`, then `UpdateWindow` |
| `invalidate_false` / `invalidate_true` | `InvalidateRect(NULL, FALSE/TRUE)`, then `UpdateWindow` |
| `redraw_noerase` | `RDW_INVALIDATE | RDW_UPDATENOW` |
| `redraw_erase` | Same plus `RDW_ERASE` |
| `paint_no_update` | Direct `WM_PAINT` send with no update region, as a separate diagnostic |

`blank=1` targets empty text; `blank=0` targets `I`. Each row starts again from
a clean old-text image. No capture uses `PrintWindow` or `WM_PRINTCLIENT`, which
could trigger an extra paint. `BitBlt` reads the control's client DC into our
own DIB, with `GdiFlush` around capture; its four-pixel border is excluded.

## Reading the observations

- `paint`, `erase`, `drawitem`: delivered messages in the measured operation.
  `paint_update` counts paint entries with an update region, queried without
  requesting erasure. `nested_erase` means erase arrived while the subclass
  was delegating `WM_PAINT`; `outside_erase` arrived outside that call.
- `marker_after` counts marker-coloured pixels left. With a valid staged row,
  a paint delivered, **zero erase messages and a removed marker** demonstrate
  background-region painting during the operation without an erase delivery.
  A surviving marker and old glyph pixels show what was left untouched.
- `old_pixels` is a mask derived from old versus empty references, excluding
  pixels belonging to the new glyph. Require a nonzero mask for a text-retention
  conclusion. `old_retained` counts pixels still exactly matching the old glyph;
  `old_dirty` counts mask pixels differing from the clean new-text reference.
  Neither alone is a universal visual-error oracle: compare erase-enabled rows,
  `diff_clean`, theme state and marker retention too.
- `sample_x/y`, `old_rgb`, `new_rgb`, `after_rgb` show one actual old-only glyph
  pixel in client coordinates, with colours in `RRGGBB`. No glyph mask means
  coordinates -1. `diff_clean` counts all captured pixels differing from the
  clean target image, including a retained marker. `update_after` reports any
  remaining invalidation.

The subclass never calls `BeginPaint`: reading `PAINTSTRUCT.fErase` there would
consume/change the native control's paint transaction. Message nesting does not
by itself prove that comctl32 explicitly sent `WM_ERASEBKGND`; `BeginPaint` can
also cause delivery. Compare no-erase and `paint_no_update` rows to determine
what messages Windows actually delivers, without claiming an unobserved caller.
The bundle records no expected Windows values and does not turn the hypothesis
into an assertion. Use the returned native observations to design the Wine test
first, then consider a correction separately.

## Fixture verification (2026-11-07)

All four EXEs compiled with MinGW 14 using `-Wall -Wextra -Werror`.
Local Wine 11.20 fixture smoke under private Xvfb and fresh prefixes reached
144 rows / zero setup errors in both x64 variants. Marker preconditions,
unchanged staged old images and nonempty old-glyph masks were checked there.
Wine's optional empty-string theme-disable call failed and was recorded;
it was not treated as a disabled-theme observation. The local i686 Wine
launcher exited 1 before a program summary, so its runtime smoke is incomplete.
These are fixture checks only. **Windows 10 x64/i686 values remain unmeasured;
no fix or Wine regression-test expectations have been chosen.**

## Windows results (2026-11-07)

Windows 10, comctl32 5.82.10586 and 6.16.10586, all four executables exit 0,
144 rows and 0 errors each; files in `windows-2026-11-07/`. The i686 files are
identical to the x86_64 ones apart from the `arch_bits` header. Every staged
row has `prep_diff=0`, `update_before=0`, `marker_before=192` and a nonempty
old-glyph mask, so all rows are usable. `theme_hr` is 0 in every classic
config, so `SetWindowTheme(hwnd, "", "")` succeeded and the classic rows are
real unthemed observations (v6 classic reports `theme=0`).

| Configuration | `settext` | `invalidate_false`, `redraw_noerase` | `invalidate_true`, `redraw_erase` | `paint_no_update` |
| --- | --- | --- | --- | --- |
| v5 (unthemed) | no erase message; old text and marker cleared | no erase; cleared | erase; cleared | nothing changes |
| v6 classic (`SetWindowTheme "",""`) | erase delivered (nested); cleared | no erase; cleared | erase; cleared | nothing changes |
| v6 themed | erase delivered (nested); cleared | no erase; **old text and marker kept** (`old_retained=1423`, `marker_after=192`) | erase; cleared | nothing changes |

Background colour after the operation (`after_rgb` at an old-only glyph pixel)
with the dark erase subclass (`custom_erase=1`, fills `#182028`):

| Configuration | after an erasing repaint | clean reference |
| --- | --- | --- |
| v5, v6 classic | `f0f0f0` - `WM_PAINT` paints the control's own background over the subclass's dark fill | `f0f0f0` |
| v6 themed | `182028` - the subclass's dark fill stays; `WM_PAINT` paints no background | `182028` |

The same holds for normal (`style=0`), `SBT_NOBORDERS` (256) and owner-drawn
(4352) parts; in the themed owner-drawn rows the dark fill also survives under
the part, so no theme pane is drawn there.

What this says about Windows:

1. **Unthemed** (v5, and v6 with the theme removed): `WM_PAINT` always paints
   the background itself, whether or not an erase happened. A non-erasing
   invalidation still clears old text. A custom `WM_ERASEBKGND` fill does not
   survive.
2. **Themed v6**: `WM_PAINT` paints no background. The background comes only
   from `WM_ERASEBKGND`; when the application handles it, its fill stays. A
   non-erasing invalidation leaves the old pixels (on Windows too).
3. `SB_SETTEXT` erases in v6 (themed and classic) and does not in v5.

Compared with `patches/comctl32-statusbar-erase-background.patch` (v1):

- Themed v6 (Inventor's case: Wine's default prefix has the aero theme
  active, and the 2026-10 trace showed a themed status bar): v1 matches
  points 2 and 3.
- Unthemed: v1 also skips the background in `WM_PAINT` when there was no
  erase, while Windows paints it. Any non-erasing invalidation then leaves the
  old text - a divergence from Windows that can produce overlapping text.
  This applies to the v5 build and to v6 without a theme.
- v5: v1 makes `SB_SETTEXT` (and the other invalidations) erase; Windows v5
  `SB_SETTEXT` does not.

Not measured yet: the invalidations of `SB_SETPARTS`, `SB_SETICON`,
`SB_SIMPLE`, `SB_SETBKCOLOR`, `SB_SETBORDERS`, `WM_SETFONT` and `WM_SIZE`;
v1 makes all of them erase. Whether the overlap Inventor users see comes from
one of the divergences above is also not known yet: next is the same bundle
under Wine (vanilla and patched), then a trace of Inventor's status bar
messages at the moment of an overlap.

## Wine results (2026-11-07)

The same x86_64 executables under Wine 11.20 on the maintainer's machine
(Arch, fresh prefixes, Wine's default aero theme): `vanilla` is Arch's
Wine 11.20, `patched` the Inventor runtime `wine-inventor-11.20-6` (which
contains v1). Files in `wine-2026-11-07/`, all 144 rows, 0 errors, exit 0.
Row-by-row comparison: `python3 compare.py windows-2026-11-07/result-x86_64-v6.txt wine-2026-11-07/patched-x86_64-v6.txt`.

Differences to ignore: the glyph masks differ by font rasterisation
(`old_retained` 1423 on Windows, 1325-1360 in Wine), and Wine's button face
is `f5f5f5` where Windows has `f0f0f0`. Wine's `SetWindowTheme(hwnd, "", "")`
fails with `0x8007007b` (ERROR_INVALID_NAME) where Windows succeeds, so the
Wine `classic=1` v6 rows are still themed and are compared against the
themed Windows rows; v5 has no theming in Wine either way.

| Build | v5 | v6 themed |
| --- | --- | --- |
| Windows 10 | WM_PAINT always paints the background; non-erasing invalidation clears; dark erase fill painted over; `SB_SETTEXT` does not erase | WM_PAINT paints no background; non-erasing invalidation keeps old text; dark erase fill stays; `SB_SETTEXT` erases |
| vanilla Wine | **same as Windows** | **differs**: WM_PAINT always paints the background (clears, paints over the dark fill); `SB_SETTEXT` does not erase |
| patched (v1) | **differs**: non-erasing invalidation keeps old text, dark fill stays, `SB_SETTEXT` erases | **same as Windows** |

Conclusions:

- For the themed v6 status bar Inventor uses, v1 already matches Windows in
  every measured operation. The overlapping text Inventor users report is
  therefore caused by something this bundle does not exercise: another
  invalidation path (the status bar's other messages, or an invalidation
  from the window system without an erase), or an application paint.
  Next: a `+statusbar` trace in Inventor at the moment of an overlap.
- v1 is wrong for v5 (and would be for an unthemed v6): v2 limits the change
  to v6 with an active theme and leaves v5 as vanilla Wine.
- Separate finding (uxtheme, not part of this patch): `SetWindowTheme` with
  empty strings fails in Wine.

## Inventor traces (2026-11-07)

`WINEDEBUG=err+all,fixme-all,+statusbar,+win`, summarised with
`trace-summary.py` (prints only message numbers, flags and counts).
Inventor 2027, a part, the mouse moved over the model for a few minutes.

| Run | Wine | Display path | Overlapping text |
| --- | --- | --- | --- |
| 1 | `wine-inventor-11.20-6` | EGL, RX 580 | no |
| 2 | `topic/gpu-rt-colorbits` v3 (11.20-6 patches + opengl32 colour bits v3) | EGL, RX 580 | no |
| 3 | same | GLX (`UseEGL=N`), RX 580 | no |
| earlier | same, plus patched vkd3d-proton, `EnableHydra=1` | GLX, RX 6400, GPU ray tracing | yes, once (maintainer, 2026-11-06); also Lukas10999 on NVIDIA |

Run 3, the reference without the fault: one status bar, about 380
`SB_SETTEXTW` and 375 `WM_PAINT`. Every invalidation of the status bar asks
for an erase (`RDW_INVALIDATE RDW_ERASE`, from `SB_SETTEXTW`, `SB_SETPARTS`,
`WM_SETREDRAW` and inside `WM_PAINT`), followed by
`RDW_ALLCHILDREN RDW_UPDATENOW`. No invalidation without `RDW_ERASE` was
seen. Every `BeginPaint` returns `fErase=0`, because Inventor handles
`WM_ERASEBKGND` itself (dark strip) before the status bar paints. Next: the same
trace with GPU ray tracing on the RX 6400, to look for an invalidation
without an erase or a paint not preceded by one.

Run 4 (03:20): same Wine and GLX, RX 6400, patched vkd3d-proton,
`EnableHydra=1`. The overlap appears **only after GPU ray tracing has been
started**; from then on every new status text is drawn over the old ones,
and opening another document only adds more layers - the status bar never
returns to a clean background in that session. The status bar side of the
trace is the same as in run 3: 286 `SB_SETTEXTW`, 243 `WM_PAINT`, every
invalidation with `RDW_ERASE`, every `BeginPaint` with `fErase=0`. So Wine
asks for the erase and someone answers it (`fErase=0`), but after ray
tracing the old text is no longer covered. `WM_ERASEBKGND` is answered by
Inventor's own handler before it reaches comctl32, so the `+statusbar`
channel cannot see it. Next: `+message` with `SpyInclude` limited to
`WM_ERASEBKGND`, `WM_PAINT` and `WM_DRAWITEM`, to see whether the erase is
still delivered after ray tracing starts and what Inventor's handler returns.

### Run 5: what Inventor draws (2026-11-07, 03:30)

Run 4's setup plus `+message` (`SpyInclude=WM_ERASEBKGND;WM_PAINT;WM_DRAWITEM`)
and a limited `+relay` (drawing functions only, `RelayInclude`). Over the
whole session (539 status bar erase/paint handlings, ray tracing started in
the middle) Inventor handles every status bar `WM_ERASEBKGND` the same way:
one `ExtTextOutW` with `ETO_OPAQUE` (a rectangle fill) on the DC that came
with the message, return value 1. `WM_PAINT` draws the text with
`DrawTextW`. The paint boxes Wine hands out do not change either: mostly
`(0,2)-(1842,24)` and `(0,0)-(1920,24)` from start to end. So neither
Inventor's drawing nor the area Wine asks for changes when ray tracing
starts.

### Window state (2026-11-07, 03:39, no trace)

Ray tracing on, overlap visible; then ray tracing off:

- Minimised and restored: Inventor came back **not maximised**, and the
  status text repainted correctly.
- Maximised again: the overlap was back, and the overlapping texts sit a
  few pixels apart vertically ("For Help, press F1" over "Ready"); the rest
  of the UI looked shifted as well (a white line along the top and left
  edge of the viewport).

Two texts a few pixels apart cannot come from a missing erase in one DC.
It looks like old window content shown at a different offset than new
content, i.e. the window's on-screen composition (winex11, Xwayland, KDE)
once a Vulkan swapchain exists in the window and the window is maximised.
**This is probably not a comctl32 problem.** The status bar patch stays as
is for Inventor, with the v5/unthemed correction above.
Lukas10999's second observation on NVIDIA (WebView2 home page not
repainted after closing Application Options until hovered) may be the
same effect. Next: the same steps inside a Wine virtual desktop
(`INV_VDESKTOP`), where the window manager does not maximise the window.

### Run 6: inside a Wine virtual desktop (2026-11-07, 03:45)

`INV_VDESKTOP=1920x1000` (explorer `/desktop`), so KDE does not manage
Inventor's window. Result: the same as on the KDE desktop. Maximised inside
the virtual desktop with ray tracing: overlapping status text; restored
(not maximised): the status bar repaints correctly. So it is **not** the
window manager: it is Wine's own handling of a maximised top-level window
once a Vulkan swapchain (vkd3d-proton) lives in one of its child windows -
win32u/winex11 window surface and client surface geometry are the
candidates (the restored and maximised states differ by the frame offset,
which matches the few pixels between the old and the new text).
Side notes from this run, not followed up: in the virtual desktop
Inventor's New and Open buttons and double-clicking a recent file did not
work, and input felt worse.

Next (proposal): a synthetic reproducer without Autodesk code - a
maximised top-level window with a Vulkan swapchain in a child window and a
status bar whose text changes - run on the RX 580 (plain Vulkan) first,
to see whether ray tracing is needed at all or any Vulkan swapchain is
enough.
