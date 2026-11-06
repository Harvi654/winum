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
| L0 | applies without fuzz on Wine 11.20 + series; build: test branch `topic/gpu-rt-colorbits` |
| L1 | open: `scripts/build-l1-opengl-color-bits.sh` (also run under vanilla Wine: must fail) |
| L2 | open: opengl32 tests, paired |
| L4 | open: Inventor 2027 GPU ray tracing on the RX 6400; SOLIDWORKS (OpenGL viewport) |
| cross review | open (written by Claude) |

Note for SOLIDWORKS: it draws with OpenGL, so the formats it is offered
change (24 -> 32 for formats without alpha, and a request for colour 24
may now get a format with alpha, as on Windows). L4 there is required.
