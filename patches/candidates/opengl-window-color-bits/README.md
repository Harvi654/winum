# Candidate: 8-bit RGB window formats without alpha are 32-bit

`win32u-window-format-color-bits.patch`: Wine describes an 8/8/8/0 window
pixel format as `cColorBits` 24 (the GLX/EGL buffer size); Windows says
32. Because `wglChoosePixelFormat` compares colour before alpha, Inventor's
GPU ray tracing check got two different formats from `ChoosePixelFormat`
and `wglChoosePixelFormatARB`, could not set the second one and greyed the
option out. A Windows-like replacement for the opt-in
`WINE_OPENGL_REQUIRE_ALPHA` patch of Autodesk-Inventor-for-Linux PR #3.

Evidence: [docs/wine-repro/opengl32-pixel-format-alpha/](../../../docs/wine-repro/opengl32-pixel-format-alpha/README.md)
(Windows 10 + RX 6400, Wine 11.20 + RX 6400 with EGL and GLX).

## State (2026-11-06)

| Level | Result |
| --- | --- |
| L0 | full build with the series, both architectures, no fuzz: `topic/gpu-rt-colorbits`, CI run 42196690304 (2026-11-06) |
| L1 | Windows 10 + RX 6400 (AMD 26.10.1), x86_64 and i686: 16 checks, 0 failures (`docs/wine-repro/opengl32-pixel-format-alpha/windows-2026-11-06-l1/`). Wine, RX 6400, GLX (2026-11-06): vanilla Wine 11.20 252 checks / 244 failures; with the patch (test build `topic/gpu-rt-colorbits`, CI run 42196690304) 252 / 0 |
| L2 | open: opengl32 tests, paired |
| L4 | Inventor 2027, RX 6400 (2026-11-06): GPU ray tracing option available and Aurora starts (without the patch: greyed out); rendering then stops in vkd3d-proton, outside Wine. Inventor 2027 normal smoke on the RX 580 (part, assembly, save, reopen): OK (2026-11-06). SOLIDWORKS (OpenGL viewport) open |
| cross review | open (written by Claude) |

Note for SOLIDWORKS: it draws with OpenGL, so the formats it is offered
change (24 -> 32 for formats without alpha, and a request for colour 24
may now get a format with alpha, as on Windows). L4 there is required.
