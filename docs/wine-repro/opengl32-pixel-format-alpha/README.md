# opengl32: pixel format with alpha, chosen twice for one window

**Problem:** Inventor greys out *Enable Viewport GPU Ray Tracing* ("The
graphics card in use does not support this option") and never loads its
GPU ray tracer `aurora.dll` under Wine, also on cards with ray tracing
hardware (tested on an RX 6400, Inventor 2027; see Autodesk-Inventor-for-Linux
`docs/arch-hyprland.md`, "GPU ray tracing").

**What is known** (from Autodesk-Inventor-for-Linux PR #3 by krokosik,
investigated with Codex on an RTX 3060, Inventor 2026.4): Inventor's
capability check chooses a pixel format with `ChoosePixelFormat`
(colour 24, alpha 8, depth 24, stencil 8, double buffered), sets it,
creates a legacy context, then chooses again with
`wglChoosePixelFormatARB` (alpha 8, full acceleration) and calls
`SetPixelFormat` on the same window. Wine's `wglChoosePixelFormat`
(`dlls/opengl32/wgl.c`) prefers an exact colour depth match without alpha
in the first step, the ARB step returns another format with alpha, and the
second `SetPixelFormat` fails. The PR's patch
(`wine-opengl-require-alpha.patch`, opt-in with
`WINE_OPENGL_REQUIRE_ALPHA=1`) skips formats without the requested alpha in
the first step. Together with `Driver\EnableHydra=1` and GLX
(`UseEGL=N`) GPU ray tracing worked on the RTX 3060.

**What `measure.c` finds out on Windows**, on the real GPU driver:

1. the RGBA window formats the driver offers (index, flags, colour,
   alpha, depth, stencil);
2. what `ChoosePixelFormat` returns for colour 24/32 with alpha 8/0;
3. the whole sequence on one window: first choice and `SetPixelFormat`,
   legacy context, GL vendor/renderer, the ARB choice, whether both are
   the same index, the second `SetPixelFormat`, and `SetPixelFormat` with a
   different format on a window that already has one.

The answers decide the fix: if Windows' `ChoosePixelFormat` returns a
format with alpha when alpha is requested, Wine should do the same without
an environment variable (a normal fix with a Wine test). If not, the
difference is elsewhere (for example, whether Windows lets the second
`SetPixelFormat` succeed), and the fix goes there.

## Build and run

```bash
bash scripts/build-measure-opengl32-pixel-format.sh ~/vm-share/pf-measure
```

On Windows 10 with the GPU passed through and its vendor driver active:
run `run-measure.bat`, return `result-x86_64.txt`, `result-i686.txt` and
`exitcodes.txt`. The same EXEs can run under Wine for comparison.

## Results

### Windows 10, AMD Radeon RX 6400, Adrenalin 26.10.1 (2026-11-06)

`windows-2026-11-06-rx6400/`, identical on x86_64 and i686:

- 216 formats; 28 RGBA, double-buffered OpenGL window formats. Every
  format of the AMD driver (flags `0x8025`/`0x8425`) is colour 32 with
  alpha 8. Only the Microsoft generic formats (`0x8465`, with
  `PFD_GENERIC_FORMAT`) include formats without alpha - and those report
  **colour 32 with 8/8/8/0**, i.e. `cColorBits` counts the whole 32-bit
  pixel, not red+green+blue.
- `ChoosePixelFormat` returns index 14 (32, 8/8/8/8, depth 24, stencil 8)
  for colour 24 or 32, alpha 8 or 0.
- The capability-check sequence: first choice 14, `SetPixelFormat` OK,
  context on the RX 6400 (OpenGL 4.6), `wglChoosePixelFormatARB` also 14,
  the second `SetPixelFormat` with the same index succeeds.
- `SetPixelFormat` with a *different* format on a window that has one
  fails with error 2000 (`ERROR_INVALID_PIXEL_FORMAT`) - Windows rejects
  it as Wine does.

Inventor 2027 in the same VM: *Enable Viewport GPU Ray Tracing* is
available and checked (DirectX 12), and ray tracing runs on the **GPU**
(Low quality, a few seconds on a small part).

What it means:

- On Windows the two choices agree because the driver offers no window
  format without alpha, not because `ChoosePixelFormat` weighs alpha above
  colour. The second `SetPixelFormat` would fail on Windows too if they
  differed.
- Under Wine on X11, GLX reports `cColorBits` = `GLX_BUFFER_SIZE`, which is
  24 for an 8/8/8/0 configuration, while Windows reports 32 for the same
  layout (generic formats above). With 24 vs 32, Wine's colour-first
  precedence picks the exact 24-bit format without alpha. If Wine reported
  32 like Windows, colour would tie and alpha would decide, as in the ARB
  path. This is a candidate for a Windows-like fix without an environment
  variable; it is not measured yet on a driver that has 8/8/8/0 window
  formats of its own.
