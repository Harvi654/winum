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

(none yet)
