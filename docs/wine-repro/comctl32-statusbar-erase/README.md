# MEASURE: status-bar background erase and old text

Original AI-assisted (Codex), LGPL-2.1-or-later. Windows results are **pending**.
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
