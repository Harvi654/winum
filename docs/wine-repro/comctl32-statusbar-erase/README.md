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

## Experiment 1: where the offset appears (2026-11-07)

Codex's first proposal (`docs/wine-repro/cross-review-2026-11-07-vkd3d-ray-payload.md`):
tell apart a wrong window content (GDI/DC side) from a wrong composition
(surface side) across the switch to ray tracing. Tool: `winsnap.c`
(original, public Win32 calls only, prints no window texts). Run in
Inventor's prefix with Inventor's Wine while Inventor runs, three times:
maximised before ray tracing, maximised with the overlap visible,
restored. Each run prints the top window, the status bar and its parent
chain (window/client rectangles, client origin, styles, DPI, window
region), the status bar's DC origin, every visible child window, and
saves the status bar twice: as its window DC holds it, and as the screen
DC returns that area. A clean `status-*.bmp` with an overlapped screen
points at composition; an overlapped `status-*.bmp` points at the window
content (DC origin, clipping); geometry changing by the few pixels of the
offset points at window geometry.

### Snapshot results and limits (2026-11-07)

The maintainer supplied before/after/Restore Down measurements from the
RX 6400 PRIME, GLX setup with patched ray-tracing DLLs. The status-window DC
image is clean before ray tracing, overlapped afterwards while maximised,
and clean after Restore Down. All three screen-DC images are entirely black;
they cannot be used to compare composition with window pixels. Screenshots
from the desktop independently show the overlap and clean restored state.

Before and after, the status rectangle is `(0,1014)-(1920,1038)`, client
`1920x24`, DC origin `(0,1014)`, DPI 96. The top window geometry/styles and
all 30 `TREE` records are identical. Restored: `IsZoomed=0`, `IsIconic=0`,
status origin `(10,1006)`, client `1910x24`. The top-level region report
changes from `none` to `set`; the observer conflates no region with an API
error, so this alone does not identify a faulty region. No raw logs or
application images are committed.

The maintainer clarified the sequence: starting ray tracing in an already
maximised window triggers overlap immediately; Restore Down cleans it;
starting ray tracing in the restored window stays clean; maximising again
brings the overlap back. This is Restore Down, not minimisation.

The original two-image interpretation above is too strong: reading through
`GetDC(status)` does not guarantee an independent pre-composition image, nor
does it reveal the HDC actually used by the application's erase/paint calls.
These measurements exclude a persistent before/after change in the observed
HWND geometry, but do not separate DC clipping, surface mapping and drawing
failure. They also do not observe transient or non-HWND surface changes.

## Experiment 2: actual drawing DCs

The isolated [status-bar DC trace](../../../patches/debug/statusbar-dc-trace/README.md)
adds numeric `+sbdiag` records to Wine's actual ExtTextOut fill/text dispatches.
No application strings or pixels are recorded by this channel. The normal
series and installed Wine are unchanged. The diagnostic build uses the full
production series; the previous Inventor build had fewer patches. **If the
same overlap does not reproduce, stop and report that difference.** An
absence of overlap on this build is not proof about the original build's DCs.

### Obtain the separate build

Download the artifact from the successful **Build status-bar DC trace** run
for the measurement commit. Extract the artifact ZIP into a new directory
`~/sb-snap/dc-trace-build`. It contains binary/source tarballs and checksums.
The following uses build number 1; if `wine/version.env` changes, use the
names printed by that run.

```bash
cd ~/winum
git pull
cd ~/sb-snap/dc-trace-build
sha256sum -c winum-11.20-1-sbdiag-x86_64.tar.xz.sha256
mkdir -p ~/.local/share/winum-sbdiag
tar -xf winum-11.20-1-sbdiag-x86_64.tar.xz -C ~/.local/share/winum-sbdiag
```

Alternatively, on a machine with the Wine build dependencies installed:

```bash
cd ~/winum
bash scripts/build-statusbar-dc-trace.sh ~/sb-snap/dc-trace-build
```

Then verify/extract as above. Full builds take time; the cloud check of the
changed objects is not an Inventor runtime test.

### One run, four observed states

Use the same working RX 6400/PRIME ray-tracing setup as experiment 1,
including the existing patched vkd3d DLLs, EnableHydra and GLX settings.
This procedure does not repeat GPU rebinding, DLL copying or registry setup.
Close Inventor and let its old launcher exit before selecting another Wine.
All launch changes below are confined to this terminal. The launcher still
uses Inventor's existing prefix; its ordinary startup may update that prefix
for the selected Wine. Do not run the two Wine builds in it simultaneously.

Terminal 1 (use a new log directory; if it exists, choose another name in
both terminals):

```bash
export INV_YEAR=2027
export INV_WINE=system
export TRACE_WINE="$HOME/.local/share/winum-sbdiag/winum-11.20-1-sbdiag-x86_64"
export PATH="$TRACE_WINE/bin:$PATH"
export WINE="$TRACE_WINE/bin/wine"
export WINELOADER="$TRACE_WINE/bin/wine"
export WINESERVER="$TRACE_WINE/bin/wineserver"
export INV_GPU=6400
export DRI_PRIME=pci-0000_23_00_0
export INV_LOGDIR="$HOME/sb-snap/dc-trace-run-1"
export WINEDEBUG=-all,+sbdiag,+timestamp,+pid,+tid
test ! -e "$INV_LOGDIR" && mkdir -p "$INV_LOGDIR"
cd ~/inventor-on-linux
bash scripts/launch-inventor.sh
```

`INV_WINE=system` tells this launcher to honour the Wine placed first in
`PATH`. Confirm its startup line names the diagnostic Wine, rather than the
old bundled build. Do not rebuild or copy individual Wine DLLs into the
prefix if a launcher check complains: report the complaint instead.

Terminal 2, after Inventor opens:

```bash
export TRACE_LOG="$(ls -t "$HOME/sb-snap/dc-trace-run-1"/inventor-*.log | head -n 1)"
grep -m 1 ':trace:sbdiag:' "$TRACE_LOG"
```

The grep should find an `SBSTATE`, `SBREGION` or related numeric record once
status text has changed. If there are no records, stop: verify the selected
Wine and channel before another run. The launcher's log uses append writes,
so these markers can be appended safely while Wine writes to it:

1. Open the part, maximise, keep ray tracing off. Mark **before**, then move
   the mouse over the part for about five seconds:

   ```bash
   printf '\nSBMARK before\n' >> "$TRACE_LOG"
   ```

2. Immediately before clicking ray tracing, mark **rt_start**. Once the
   overlap appears, mark **after** and move the mouse for five seconds:

   ```bash
   printf '\nSBMARK rt_start\n' >> "$TRACE_LOG"
   ```

   ```bash
   printf '\nSBMARK after\n' >> "$TRACE_LOG"
   ```

3. Click **Restore Down**, confirm clean text, mark **restored**, then move
   the mouse for five seconds. Ray tracing can remain on:

   ```bash
   printf '\nSBMARK restored\n' >> "$TRACE_LOG"
   ```

4. Maximise again. When the overlap returns, mark **remax** and move the
   mouse for five seconds:

   ```bash
   printf '\nSBMARK remax\n' >> "$TRACE_LOG"
   ```

Close Inventor normally, then create the shareable report:

```bash
python3 ~/winum/docs/wine-repro/comctl32-statusbar-erase/sbdiag-summary.py "$TRACE_LOG" > ~/sb-snap/dc-trace-summary.txt
grep -E '^(SBREPORT|PHASE|SIGNATURE|OMITTED|CHECK)' ~/sb-snap/dc-trace-summary.txt
```

Return `dc-trace-summary.txt` and say which of the four states showed the
fault. Keep the original launcher log local: it can contain unrelated program
output even though `+sbdiag` itself prints no application strings. The parser
accepts only fixed numeric fields and fixed region labels, associates records
by thread/DC, groups complete calls by their entry phase, and reports missing
or rejected records rather than copying arbitrary lines. If signatures are
omitted, increase `--limit` locally; do not send the full original log.

### What the result can distinguish

- Different erase/text `vis`, `device`, matrix, DIB rectangle or surface
  identity after ray tracing, reversed on Restore Down and repeated on
  remaximisation: supports a DC/surface mapping error. Pointer changes by
  themselves can also be ordinary resource replacement.
- Empty or differently shaped effective/DIB clipping for a fill, while text
  remains drawable: supports erase clipping as the reason old text survives.
  A bounding box alone is insufficient; compare count/hash and rectangles.
- `SBFILL ret=0`: the actual fill dispatch failed, even if the enclosing
  `SBRETURN ret=1`. This directs the next trace to that selected driver.
- Same successful fills, mapping and clipping in clean/faulty states:
  weakens those hypotheses; next measure surface pixels immediately around
  the fill and inspect surface copying/presentation. This trace does not
  claim to settle those downstream paths.

After the run, close Terminal 1 and use the previous normal launcher settings
to return to the old Wine. GPU/DLL/registry restoration, if wanted, is the
separate existing procedure; this trace does not invent backup locations.

### Drawing-DC results (2026-11-07)

The maintainer reproduced **A clean -> B overlap -> C clean -> D overlap**
with the diagnostic Wine, RX 6400 PRIME, GLX and the same patched vkd3d DLLs.
The supplied numeric summary contains 696 complete calls, zero rejected or
incomplete records, and no omitted signatures. This also confirms that the
fault reproduces with the full production series used by this build.
Only derived observations are recorded here; no raw application log is added.

| Marker interval | Complete calls | Window/DIB driver attached | Window/DIB driver absent |
| --- | ---: | ---: | ---: |
| unmarked | 206 | 206 | 0 |
| before | 100 | 100 | 0 |
| rt_start | 59 | 7 | 52 |
| after | 99 | 7 | 92 |
| restored | 134 | 119 | 15 |
| remax | 98 | 0 | 98 |

The manual markers are written after observing each state, so transitions
occur in the previous interval too: for example, Restore Down introduces
restored-size draws near the end of `after`, and maximising introduces draws
in `restored` before `remax` is marked. Signatures are grouped by frequency,
not chronologically. The table is not a precise transition timeline.

In the stable clean maximised interval, all 50 fills and 50 text calls have
`window_driver=1`, a non-dummy parent window surface with rectangle
`(0,0)-(1920,1152)`, DIB mapping `(0,1014)-(1920,1038)` and stride 7680.
In the faulty maximised intervals the window driver is absent, the selected
text-dispatch table changes, and `device=(0,0)-(0,0)`. Restore Down returns
the DIB-backed path, mapping `(0,1006)-(1910,1030)` for the narrower status
bar. Maximise again removes it for all 98 calls under `remax`.

The zero device rectangle does **not** mean the HWND became zero-sized.
`win32u/dce.c:update_visible_region` explicitly empties that internal rectangle
when no window surface is selected. The stock bitmap printed on this path
does not prove drawing targets a 1x1 bitmap: the display driver can draw
directly to its host drawable. This path switch is strong correlation with
the fault, not proof that direct drawing itself should always be disabled.

All 354 explicit fills report `ret=1`, including those on the faulty path.
The common visible/effective region remains `(0,2)-(1842,24)` in the clean
and faulty maximised states; no application or meta clip is selected in those
common calls. Mapping remains identity, the fill rectangle remains the whole
status client area, background colour is unchanged, and the common text
dispatch origin is `(0,18)` throughout. Some fully clipped fills occur during
transitions and before the fault as well; they do not explain the stable
overlap. Dispatch success is not a pixel readback or acknowledgement of all
asynchronous X11 rendering effects.

Open-source follow-up read: `win32u/dce.c:update_visible_region`,
`win32u/dibdrv/dc.c:dibdrv_set_window_surface`,
`winex11.drv/bitblt.c:enable_direct_drawing` / `X11DRV_CreateWindowSurface`,
`winex11.drv/window.c:X11DRV_GetDC`, `winex11.drv/xrender.c` picture/clipping
and ExtTextOut dispatch, and server visible-region/top-clipping selection.
Wine can deliberately drop a DIB surface and draw directly when a host client
window exists. X11 DC origin is then derived separately from the window and
top surface rectangles. The trace does not yet identify which condition
caused the switch or measure the host drawable/picture coordinates.
The observed table address alone cannot name the replacement driver.

### Next control: disable XRender for Inventor only

Before another custom build, use the existing diagnostic Wine with just
`HKCU\Software\Wine\AppDefaults\Inventor.exe\X11 Driver` value
`ClientSideWithRender` set to `REG_SZ N`. Wine 11.20 reads this per-application
setting in `x11drv_main.c`; `X11DRV_XRender_Init` returns without installing
the XRender driver when false. Keep `ClientSideGraphics`, UseEGL, GPU, DLLs
and ray-tracing settings unchanged. This does not disable Vulkan/DXR itself,
but the maintainer must verify ray tracing actually starts in the new run.
Close the first launcher, change the setting using that Wine in the same
prefix, stop/wait for its wineserver, then start one fresh launcher with a
new log directory and repeat the same four states/markers. Preserve the old
registry value locally and restore it after the control run.

- Clean text with ray tracing working and `window_driver=0` still observed:
  strongly narrows the fault to the XRender drawing/picture route, rather
  than the generic fact that the DIB driver was removed.
- The same overlap with the setting confirmed `N`: XRender is not required;
  trace the common direct drawable origin, clipping and surface attachment.
- No working ray tracing, or the DIB/direct transition does not occur:
  the negative visual result does not isolate the XRender hypothesis.

No fix is proposed from this correlation, and the production series remains
unchanged.

### XRender-disabled control results (2026-11-07)

With per-application `ClientSideWithRender=N`, the maintainer reported clean
text in **all four phases**, and separately confirmed that ray tracing really
rendered the model progressively, rather than merely enabling its button.
The supplied summary has 818 complete calls, zero rejected/incomplete records
and no omitted signatures. No raw log, screenshot or registry dump is added.

| Marker interval | Complete calls | Window/DIB driver attached | Window/DIB driver absent |
| --- | ---: | ---: | ---: |
| unmarked | 261 | 261 | 0 |
| before | 124 | 124 | 0 |
| rt_start | 83 | 7 | 76 |
| after | 89 | 15 | 74 |
| restored | 95 | 82 | 13 |
| remax | 166 | 14 | 152 |

The manual-marker boundary limitation above still applies, including restored
draws at the end of the final interval. Crucially, the direct-drawing path
still occurs: the common maximised-after and remax calls have
`window_driver=0` and the intentionally cleared device rectangle. Their
common visible/effective clip is still `(0,2)-(1842,24)`, mapping is identity,
background colour is unchanged, and text dispatch is still `(0,18)`.
All 417 explicit fills report success. The dispatch table switches to the
fallback used with XRender disabled, rather than retaining the DIB driver.
Absolute table addresses from separate processes are not compared as symbols.

This control substantially narrows the fault to the **XRender-enabled direct
drawing route**. Removing the DIB window driver is not sufficient to cause
overlap, and the clean result was not obtained by preventing ray tracing or
keeping that driver attached. It does not yet identify a faulty Wine line,
XRender picture/clip operation, or an X server/driver interaction. Success
returns are still not evidence that the intended pixels were overwritten.

`ClientSideWithRender=N` is a **tested per-application workaround on the
diagnostic build**, for the four window states with working GPU ray tracing.
It disables Wine's XRender GDI driver for Inventor; it does not globally
disable the desktop compositor or change the ray-payload shader patch.
It may change 2D text rendering/performance. At this stage the normal Inventor
Wine build, longer CAD use and the other consumer had not been validated with
this setting; the subsequent normal-build smoke test is recorded below.
This is not a Wine fix or an L4/L5 compatibility claim.

To use the workaround in this test prefix after restoring the control value,
close Inventor and set the value again with the selected Wine:

```bash
export WINEPREFIX="$HOME/.wine-inventor2027"
export WINE="$HOME/.local/share/winum-sbdiag/winum-11.20-1-sbdiag-x86_64/bin/wine"
"$WINE" reg add 'HKCU\Software\Wine\AppDefaults\Inventor.exe\X11 Driver' /v ClientSideWithRender /t REG_SZ /d N /f
```

The next source-level measurement should instrument the failing XRender
route itself: actual host drawable and picture identities, DC/picture clip
origins and rectangles, fill destination coordinates/operator/colour, and
glyph destination coordinates. Compare with the matching direct fallback
route and with restored state. Use numeric Wine debug records and public
X11/XRender APIs, without application strings or proprietary binary analysis.
Re-enable the original XRender setting for a fault-path measurement; keeping
the workaround enabled would bypass the path being investigated. No additional
synthetic GPU-mode matrix or production patch is justified by these results.

### Normal-build smoke test and remaining frame artefact (2026-11-07)

The maintainer repeated ordinary CAD use on the original `inventor-gpurt-v3`
Wine, with `ClientSideWithRender=N`, the same ray-tracing DLLs and GLX:

| Check | Maintainer result |
| --- | --- |
| Ordinary interface interaction | Basic interactions OK; frame-state and WebView repaint caveats below |
| Part editing and view manipulation | OK |
| GPU ray tracing and maximisation, status text | OK |
| Assembly | OK |
| Drawing and 2D PDF | OK |
| Save, close, restart and reopen | OK |
| Font appearance and interface responsiveness | No change noticed |

This extends the status-text workaround validation to the original Wine build.
It is a short Inventor smoke test, not a full shared-DLL L4 or a week of L5.

The same report adds an unresolved visual issue: after Restore Down and
maximising again on Arch, parts of the interface are redrawn differently,
including a white strip at the top/left of the viewport. Actual minimisation
and showing the window maximised again, or closing/reopening the window,
clears the reported state. This is distinct from the status-text overlap as
an observation; whether the causes are related is unknown. A white top/left
edge and shifted interface were already reported earlier with the original
XRender-enabled failure (see "Window state" above), so its presence now does
not establish a new regression caused by `ClientSideWithRender=N`.

The maintainer compared the restored/maximised window sizes with a Windows
VM and found them approximately the same. Exposed background next to a
smaller restored window is not by itself corrupt rendering. The supplied
screenshots are observational evidence, not a pixel-accurate geometry test,
and are not committed. The fresh-session control is recorded below.

The application's public hardware dialog names RX 6400 / RADV NAVI24,
DirectX 12, hardware rendering and 32-bit display colour. Its unknown Windows
driver file/version is consistent with the incomplete driver-name metadata
already noted in the launcher's experimental `INV_DRIVER_NAMES` option;
it does not independently establish GPU failure. Display colour depth is not
the application's process architecture, and the dialog's WHQL wording is
not verification of Linux driver certification. These labels do not identify
the source of the frame artefact. No graphics-quality setting, driver-name
override or additional patch is changed on the basis of the screenshot.

### Fresh-session control without starting ray tracing (2026-11-07)

After reboot, on the original gpurt-v3 Wine with `ClientSideWithRender=N`,
GLX and the same enabled graphics configuration, the maintainer confirmed
that repeated Restore Down/maximise transitions are correct when ray tracing
is never started in that session. Once ray tracing is started, subsequent
window-state transitions produce the frame/white-edge artefact; actual
minimisation and showing the window again resets the reported display state.

This ties the remaining frame artefact to state introduced after starting
ray tracing, rather than ordinary fresh-session resizing alone. It survives
the XRender-disabled configuration that avoids status-text overlap. A common
window/client-surface state change could underlie both symptoms, but that is
not proven. Disabling Wine's XRender driver is therefore a status-text
workaround, not a complete correction of window-state rendering. No specific
XRender library, X server, Mesa or Wine implementation defect is claimed yet.

### Native-Wayland comparison prepared (2026-11-07)

The maintainer confirmed the desktop session is Wayland, so the measured X11
driver runs use XWayland. A [native-Wayland control](../winewayland-inventor/README.md)
was prepared on a private copy of the Inventor prefix, with explicit backend
selection and an active-driver verification procedure. The maintainer later
reported working ray tracing but broken WebView, abnormal dialogs, inability
to type dimension values and unsatisfactory window-state transitions. The
verification output/log was not returned, so no exact backend API failure is
identified. The practical direction remains the tested XWayland/X11 path with
the XRender workaround; native-Wayland UI compatibility is outside this fix.

### WebView home-page repaint clarification (2026-11-07)

The maintainer clarified that the WebView2 home-page repaint symptom also
persists locally in the working XWayland configuration: after the dialog
interaction, the home page can remain stale until a click causes it to
refresh. The maintainer had observed it before the current experiments and
had not investigated it; it is not newly introduced by these tests. This
qualifies the earlier interface smoke-test result. Do not
describe the XRender workaround as fixing all UI repainting or imply that
the WebView symptom was observed only by the NVIDIA tester.

This is an open observation alongside the post-ray-tracing frame artefact.
Its dependence on starting ray tracing has not been measured; the fresh-session
frame control does not establish that dependence for WebView. A shared Wine
window-surface/update issue is possible, but neither that nor the earlier
attribution to WebView2's GPU process is proven. Keep the WebView symptom in
the remaining investigation rather than declaring an independent known cause.
