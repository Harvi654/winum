# Candidate: 8-bit RGB window formats without alpha are 32-bit

`opengl32-window-format-color-bits.patch` (v2): Wine describes an 8/8/8/0 window
pixel format as `cColorBits` 24 (the GLX/EGL buffer size); Windows says
32. Because `wglChoosePixelFormat` compares colour before alpha, Inventor's
GPU ray tracing check got two different formats from `ChoosePixelFormat`
and `wglChoosePixelFormatARB`, could not set the second one and greyed the
option out. A Windows-like replacement for the opt-in
`WINE_OPENGL_REQUIRE_ALPHA` patch of Autodesk-Inventor-for-Linux PR #3.

Evidence: [docs/wine-repro/opengl32-pixel-format-alpha/](../wine-repro/opengl32-pixel-format-alpha/README.md)
(Windows 10 + RX 6400, Wine 11.20 + RX 6400 with EGL and GLX).

## v2 (2026-11-06)

Codex's cross review of v1
([cross-review-2026-11-06-opengl-color-bits.md](../wine-repro/cross-review-2026-11-06-opengl-color-bits.md))
found that v1, made in win32u's shared descriptors, also changed what a
memory DC saw (GLX and EGL formats that draw to windows can draw to
bitmaps too): P2-1. v2 makes the change in opengl32's per-DC copy, only
for DCs that are not memory DCs; memory DCs are unchanged. The test's P3
points (check the exact format before the ARB comparison, check the
restore return values) are done. The results below are v1's unless
marked v2.

## v3 (2026-11-06)

Codex's re-review of v2 (section "v2 re-review"): memory DCs and the
test points are fixed, but a short HDC alias (low 16 bits only) could hit
the per-thread cache entry of a deleted window DC whose slot a memory DC
reused: P2-2. v3 never uses short handles as the cache key.

## State (2026-11-06)

| Level | Result |
| --- | --- |
| L0 | full build with the series, both architectures, no fuzz: `topic/gpu-rt-colorbits`, CI run 42196690304 (2026-11-06) |
| L1 (Wine) | v3, RX 6400, default renderer (2026-11-06): vanilla Wine 11.20 134 checks / 124 failures, v3 build (CI run 42214402350) 134 / 0 |
| L1 | v3: Windows 10 + RX 6400 (AMD 26.10.1), x86_64 and i686: 18 checks, 0 failures (`windows-2026-11-06-l1-v3/`). v1: 16 checks, 0 failures (`docs/wine-repro/opengl32-pixel-format-alpha/windows-2026-11-06-l1/`). Wine, RX 6400, GLX (2026-11-06): vanilla Wine 11.20 252 checks / 244 failures; with the patch (test build `topic/gpu-rt-colorbits`, CI run 42196690304) 252 / 0 |
| L2 | open: opengl32 tests, paired |
| L4 | Inventor 2027, RX 6400 (2026-11-06): GPU ray tracing option available and Aurora starts (without the patch: greyed out); rendering then stops in vkd3d-proton, outside Wine. Inventor 2027 normal smoke on the RX 580 (part, assembly, save, reopen): OK (2026-11-06). v3 (2026-11-06, RX 6400, GLX, with the vkd3d-proton ray payload fix): GPU ray tracing option available, renders on the GPU; part, assembly, save OK. SOLIDWORKS (OpenGL viewport) open |
| cross review | v3 OK (Codex, 2026-11-06, section "v3 check"; after P2-1 in v1 and P2-2 in v2). v3 x64 EGL suite paired: 64 failures without, 0 with; GLX/i686 full suites incomplete in Codex's container |

Note for SOLIDWORKS: it draws with OpenGL, so the formats it is offered
change (24 -> 32 for formats without alpha, and a request for colour 24
may now get a format with alpha, as on Windows). L4 there is required.
