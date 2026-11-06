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

### Inventor 2027 under Wine on the RX 6400 (2026-11-06)

Arch, KDE Plasma (Xwayland), RX 6400 bound to amdgpu for the test, all
three APIs on it (`INV_GPU=6400`, `DRI_PRIME=pci-0000_23_00_0`), GLX
(`UseEGL=N` for Inventor.exe), `Driver\EnableHydra=1`, DirectX 12 through
vkd3d-proton 3.0.1. The monitor on the RX 6400 was disabled in KDE during
the test: with it, Xwayland used its scale 2.7 for all X11 windows.

| Run | Wine build | *Enable Viewport GPU Ray Tracing* | Aurora |
| --- | --- | --- | --- |
| A | series + PR #3 patch, `WINE_OPENGL_REQUIRE_ALPHA` unset (= vanilla behaviour) | greyed out, "does not support this option" | - |
| B | same, `WINE_OPENGL_REQUIRE_ALPHA=1` | available | starts |
| C | series + `opengl-window-color-bits` candidate, no variable | available | starts |

In B and C Aurora starts (its `ConvertEnvMapLayout` message), then
`vkCreateRayTracingPipelinesKHR` faults in the Vulkan driver and Inventor
exits (afterwards an unimplemented `msvcp140.?_Throw_Cpp_error`). That is
the next stage, outside Wine: vkd3d-proton's DXIL-to-SPIR-V translation of
Aurora's long-vector ray payloads (Autodesk-Inventor-for-Linux PR #3,
`vkd3d-ray-payload.patch`, written for vkd3d-proton 2.14.1).

Result: the pixel format choice is what greys the option out on AMD, and
the Windows-like candidate removes it without an environment variable.

### GPU ray tracing renders (2026-11-06, 12:30)

Same setup as run C, plus vkd3d-proton 3.0.1 with
`dxil-spirv-ray-payload-long-vector.patch` (branch `topic/vkd3d-rt`, CI run
42207914047; `d3d12.dll` 1e1e01be..f545, `d3d12core.dll` 7ea42672..58b2):
Inventor 2027 renders the part with GPU ray tracing, the panel says
**GPU**, Low quality in 2 s - the same as Windows 10 on the same RX 6400.

So two changes are needed on AMD, and nothing else: the Windows-like
pixel format description in Wine (`opengl-window-color-bits`) and the
long-vector fix in vkd3d-proton's dxil-spirv. Not yet tried: Draft/High
quality, noise reduction, an assembly, a longer session.

Later the same session: Draft and High quality, noise reduction on, an
assembly with several parts, orbiting while ray tracing - all render on
the GPU and refresh each time; no errors in the log. Seen once, not
investigated: the status bar text at the bottom left was drawn over
itself (two messages overlapping). Note it if it comes back.
