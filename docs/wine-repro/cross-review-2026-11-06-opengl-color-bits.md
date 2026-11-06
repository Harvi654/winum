# Cross-review: OpenGL window colour bits (2026-11-06)

Reviewer: Codex; candidate author: Claude. Reviewed `main` at `1620260`,
including the entire candidate patch and its README, the measurement README,
`measure.c`, and the committed Windows L1 results. The candidate SHA-256 is
`5a30f0a7ec4e0de48c289fb88c1f4296bbaec8ff46beeba036bad23ad2c0ee3a`.
No patch, script or series was changed by this review.

Reference: public Wine `wine-11.20`, commit
`7298a818b79446bae153b8d5aa27235ab3703bf9`, plus the repository's 27 series
patches. Reviewed the complete affected functions and their callers in
`dlls/win32u/opengl.c`, `dlls/winex11.drv/opengl.c`,
`dlls/opengl32/{wgl.c,unix_wgl.c,unix_private.h}` and the surrounding OpenGL
tests; searched all three DLL directories for `cColorBits` consumers.
No Microsoft internals, disassembly or vendor binaries were inspected.
Source line numbers below refer to the candidate-applied source.

## P2-1: bitmap formats are changed, through both GLX and EGL

**Must be resolved before series.** The guard in
`dlls/win32u/opengl.c:3478`, `display_funcs_init`, excludes offscreen-only
entries but does not exclude formats supporting both windows and bitmaps.
This contradicts the patch header's promise that bitmap formats keep their
sizes. It changes descriptors and selection on real memory DCs, not just
an unused driver record.

- GLX: `x11drv_init_pixel_formats` (`dlls/winex11.drv/opengl.c:843`)
  inserts bitmap-capable clones inside `onscreen_count`. Its
  `check_fbconfig_bitmap_capability` checks single buffering, pbuffer support
  and buffer size versus visual depth. `x11drv_describe_pixel_format`
  also advertises `PFD_DRAW_TO_WINDOW` when the underlying config supports
  windows. RGB888/no-alpha bitmap clones therefore pass the new guard.
- EGL: `describe_egl_config` (`dlls/win32u/opengl.c:1374`) adds
  `PFD_DRAW_TO_BITMAP` whenever `draw_to_pbuffer` is true. Window-capable
  aliases inside the onscreen range can therefore also be bitmap-capable,
  including the GDI alias. This is not confined to GLX clones.
- `opengl32/wgl.c:1663`, `get_pixel_formats`, copies the same global
  descriptors for a memory DC and exposes all formats there; it does not
  undo the normalization. `wglDescribePixelFormat`, legacy selection and
  `WGL_COLOR_BITS_ARB` all observe the changed values.

Independent x64 public-API probe, Xvfb at depth 24, 2026-11-06:

| Observation | S: series | C: series + candidate |
| --- | ---: | ---: |
| GLX memory-DC RGB888/no-alpha bitmap formats, colour 24 / 32 | 16 / 0 | 0 / 16 |
| GLX bitmap choice: index / colour bits | 10 / 24 | 10 / 32 |
| EGL memory-DC RGB888/no-alpha bitmap formats, colour 24 / 32 | 80 / 0 | 20 / 60 |
| EGL bitmap choice: index / colour bits | 15 / 24 | 5 / 32 |
| Actual selected DIB bits per pixel, both backends | 24 | 24 |

To reproduce: on a fresh Xvfb display, select a 4x4, 24-bit `CreateDIBSection`
into `CreateCompatibleDC(window_dc)`. Enumerate `DescribePixelFormat` on
that memory DC, filtering RGBA, RGB 8/8/8, alpha 0 and `PFD_DRAW_TO_BITMAP`.
Call `ChoosePixelFormat` with `PFD_DRAW_TO_BITMAP | PFD_SUPPORT_OPENGL |
PFD_SUPPORT_GDI`, colour 24 and no double buffering. Describe the selected
index and compare it with `GetObject` on the DIB. Repeat with
`HKCU\Software\Wine\X11 Driver\UseEGL` set to `N` and `Y`, in separate
prefixes/processes. The probe also queried `WGL_COLOR_BITS_ARB`: it agreed
with `DescribePixelFormat` throughout.

This proves a scope leak, not memory corruption or a measured Windows
bitmap incompatibility. No native memory-DC/24-bit-DIB expected value is
recorded in the supplied measurements. Resolve the scope and obtain that
measurement before asserting what Windows should return. Merely excluding
every `PFD_DRAW_TO_BITMAP` record would also exclude many intended EGL
window formats; account for shared capabilities rather than applying that
filter blindly. Add coverage for the memory-DC case. The new test currently
asserts 32 for these dual-capability records too, so it cannot detect this
scope error.

## Remaining production paths

- Offscreen-only records beyond `onscreen_count` remain unchanged. Pbuffers
  can also use window-capable records: those records' advertised colour
  bits change even when subsequently used for a pbuffer. In the probe,
  shared window/pbuffer RGB888/no-alpha records changed 24 to 32 for 48
  GLX and 60 EGL entries; EGL's 20 offscreen-only entries stayed 24.
  Native config IDs, pbuffer capability and underlying storage do not change.
- Nulldrv's `nulldrv_describe_pixel_format` (`win32u/opengl.c:2014`)
  advertises bitmap/GDI/OpenGL, never `PFD_DRAW_TO_WINDOW`; its RGB24
  formats do not match the guard. This conclusion is from source inspection,
  not a separate nulldrv runtime test.
- EGL config enumeration, X11's `x11drv_egl_describe_pixel_format` visual
  depth check, GLX config/visual selection and `visual_from_pixel_format`
  use backend config/visual information before or independently of the
  normalized global descriptor. No dependence on normalized colour 24
  was found in visual selection.
- `internal_context_create` (`win32u/opengl.c:258`) rejects colour sizes
  below 24. Both 24 and 32 pass; format order and driver IDs stay unchanged.
  Framebuffer attachments use component sizes. Memory-DC transfers use the
  actual bitmap/image data. No allocation or bitmap stride calculation based
  on the changed `cColorBits` was found in these DLLs.
- `wglChoosePixelFormat` ranks by colour before alpha, so normalization
  intentionally changes ranking. `wgl_pixel_format_get_attrib` returns
  the normalized value for `WGL_COLOR_BITS_ARB`; ARB filtering/sorting sees
  it too. Existing memory-DC special handling for colour 16 is unaffected.
  Existing `test_colorbits` requires alpha and does not exercise RGB888/0.

## Test review and P3 portability follow-up

`test_choosepixelformat_alpha` restores the saved context and draw DC before
deleting its own context and window. Its early skip occurs before switching
contexts. At its actual suite call site, the previous tests leave an ordinary
`wglMakeCurrent` binding, so no distinct read DC needs restoration there.
No context contamination was observed: the extracted test body, unchanged,
was wrapped with two additional assertions checking the original HGLRC/HDC.
Both assertions passed on S and C with both backends. Including those checks,
GLX was 62 assertions, 52 failures to 0; EGL was 74, 64 failures to 0.

**P3:** at `opengl32/tests/opengl.c:1064-1108`, `with_alpha` only establishes
some RGB888 window format with nonzero alpha. It does not establish an
accelerated, double-buffered colour-32/alpha-8/depth-24/stencil-8 match for
the later ARB request. A catalog lacking that combination can reach the
unconditional success/equal-index assertions. Check that exact capability
before making those assertions; do not invent a native fallback value.
Also assert the restoration/deletion return values so a future restoration
failure is attributed here. This is a portability/diagnostic gap, not an
observed failure of the committed Windows 10 L1 run: both architectures
record 16 checks, zero failures on the RX 6400.

## Independent builds and paired execution

All results here are from 2026-11-06. Candidate application after the 27
series patches succeeded with `patch --batch --fuzz=0 -p1`, without offsets
for the candidate. Debian 13 container, GCC/MinGW 14, configuration
`--enable-archs=x86_64,i386 --enable-tests --with-x --with-opengl` alongside
the existing full-feature configuration. Successfully built S and C
`win32u.dll` and `opengl32.dll` for both architectures, both
`opengl32_test.exe` executables, and the shared Unix `win32u.so`,
`opengl32.so` and `winex11.so` components. Only `win32u.so` contains the
production change; the PE DLL sources are unchanged. This was a component
build, not an independent full-runtime L0 build. The candidate README's
full L0 CI result remains separately recorded evidence.

S/C execution used copies of the cached Wine 11.20 diagnostic MSI-selection-8
runtime, with the same freshly built series OpenGL/Win32u PE components and
OpenGL/X11 Unix components installed into both. Byte comparison verified
that the two complete runtime trees differed only in `win32u.so`:

- S SHA-256: `ce9c04fd99a2620b80384a09f04ccb4cd998098f3e7ac2d6399471d12e18c96e`
- C SHA-256: `9e7838514ccc7bdd46ffe4a74ea4b3b10e5be7f9205594c04ba308bcf76a1413`

The identical candidate-applied test executable was used on each side.
Every invocation had a new prefix, HOME/XDG directories and private Xvfb
display; no existing/user prefix was used. Servers were stopped after each
run. Results are numeric summaries; no raw Wine logs are committed.

| Full `opengl32_test.exe opengl` pair | S | C | Assessment |
| --- | --- | --- | --- |
| x64 EGL | 9623 checks, 64 failures, 65 todo, 39 skipped; exit 64 | 9915 checks, 0 failures, 65 todo, 39 skipped; exit 0 | No observed failures in C; coverage changed by 292 checks |
| x64 GLX | 180 s timeout, no final summary; killed with exit -9 | Same | Incomplete; 59 interim failures at the same 43 source locations on both sides |
| i386 GLX and EGL | exit 1, no test summary | exit 1, no test summary | Incomplete: no executed-suite pass established |

All 64 EGL baseline failures were inside the added test: `opengl.c:1075`
(60), `1089`, `1090`, `1101`, `1105` (one each). No existing-test failure
was observed in that completed pair. GLX reached a failed thread wait at
`opengl.c:4432` in `test_window_dc`, which runs before the new alpha test;
the identical interim failures do not establish a completed GLX pass.
The assertion-count change prevents
calling it an identical-coverage L2 pass. The component runtime and failed
i386 execution also limit this result; it is not full release validation.

## Verdict

**Not ready for series: resolve P2-1, the demonstrated GLX/EGL memory-DC
scope leak.** The intended alpha-selection sequence succeeds in the local
standalone tests and the x64 EGL suite, and context restoration works.
Those results do not establish that bitmap descriptors should change on
Windows, nor close the incomplete paired-suite coverage.

## v2 re-review

Reviewed `f7efc4d032d616735cd850cf8f696b1808238522` after updating `main`.
Read the complete replacement
`patches/candidates/opengl-window-color-bits/opengl32-window-format-color-bits.patch`,
its dated v2 explanation, updated candidate README and script/series changes.
Patch SHA-256:
`4fe64162323a5ce80cc2468cfff2197be60e9aef272f59738016bf04813259a2`.
Only this review document is changed. The public Wine reference and clean-room
constraints above still apply. Line numbers below are from v2-applied source.

### Ordinary DCs: the original scope problem is fixed

The normalization now runs after fetching a private copy in
`opengl32/wgl.c:1692-1707`, only in the non-memory-DC branch. Neither the
driver's formats nor win32u's shared descriptors are modified. Repeating the
same public-API probe with a selected 4x4, 24-bit DIB gave:

| Observation | S: series | C: series + v2 |
| --- | ---: | ---: |
| GLX memory DC: RGB888/0 bitmap formats, colour 24 / 32 | 16 / 0 | 16 / 0 |
| GLX memory DC: bitmap choice index / colour | 10 / 24 | 10 / 24 |
| EGL memory DC: RGB888/0 bitmap formats, colour 24 / 32 | 80 / 0 | 80 / 0 |
| EGL memory DC: bitmap choice index / colour | 15 / 24 | 15 / 24 |
| GLX window DC: RGB888/0 window formats, colour 24 / 32 | 48 / 0 | 0 / 48 |
| EGL window DC: RGB888/0 window formats, colour 24 / 32 | 60 / 0 | 0 / 60 |
| Selected DIB bits per pixel, both backends | 24 | 24 |

No integer/float attribute versus descriptor mismatch was observed on either
DC type. Five hundred window/memory alternations, each including repeated
descriptor reads and an integer attribute read, produced zero cache mismatches
on both S and C, GLX and EGL. C consistently returned window colour 32 and
memory colour 24. P2-1 is resolved for ordinary, full HDC values.

### Format consumers and pbuffer paths

- `wglDescribePixelFormat` (`wgl.c:2171`) gets the per-DC copy directly;
  `wglChoosePixelFormat` (`1453`) enumerates through that function.
- `wglChoosePixelFormatARB` (`2101`) filters and sorts pointers into the same
  copy. `wglGetPixelFormatAttribivARB` (`2191`) also calls `get_pixel_formats`;
  its float counterpart (`2237`) delegates to the integer function. Thus
  `WGL_COLOR_BITS_ARB` and legacy descriptions agree for the same DC/index.
- Pbuffer creation is different: `wglCreatePbufferARB` (`wgl.c:257`) passes
  the unchanged format index to `wrap_wglCreatePbufferARB`
  (`unix_wgl.c:731`) and the backend. It does not reinterpret the normalized
  colour size or allocate storage from it. Subsequent format queries on a
  pbuffer DC use the same per-DC query path described above.
- `win32u/opengl.c:2237`, `pbuffer_create`, opens a display DC through
  `NtGdiOpenDCW`; `wglGetPbufferDCARB` returns that DC. It is `OBJ_DC`, not
  `OBJ_MEMDC`, so v2 also normalizes its window-capable onscreen records.
  This is consistent with the header's non-memory-DC rule. Offscreen-only
  records remain unchanged. The scope probe created 4x4 pbuffers successfully:
  GLX index 9 and EGL index 11 stayed the same; their descriptor/ARB colour
  changed 24 to 32 and their DC type was 3 (`OBJ_DC`).
- The cache is one entry per thread, in `NtCurrentTeb()->glReserved1`, keyed
  by HDC. For full Wine handles, DC and memory-DC type bits differ
  (`ntgdi.h:65,80`; `win32u/gdiobj.c:52`). A normal DC switch misses the cache
  and builds a fresh copy. Different threads do not share this client cache.
  However, the following accepted-handle boundary prevents a blanket claim
  that the HDC key can never mix the two DC types.

### P2-2: a short HDC alias can reuse a window cache entry for a memory DC

**Fix before series.** At `opengl32/wgl.c:1678-1683`, a cache hit compares
only the supplied HDC value. Although `NtGdiIsMemDC` is queried immediately
before that comparison, its current result is not checked against the cached
DC kind. The new memory-DC branch is bypassed on a hit.

Wine deliberately accepts GDI handles with a zero high word:
`win32u/gdiobj.c:58`, `handle_entry`, accepts either no high word or the
matching full `Unique` value. Such a short alias has only the table slot,
without generation/type bits. After deleting a display DC and reusing its
slot for a memory DC, the short alias still compares equal to the cache key.
The handle validates as a memory DC but receives the old normalized window
descriptors. Full returned HDCs were not affected in the probes above.

Independent public-API reproduction, with a 24-bit DIB selected into the
replacement memory DC, succeeded on both GLX and EGL:

| Current memory-DC query | S colour / format count | C colour / format count |
| --- | --- | --- |
| GLX, cached short alias | 24 / 80 | 32 / 80 |
| GLX, full returned HDC (cache miss) | 24 / 240 | 24 / 240 |
| EGL, cached short alias | 24 / 150 | 32 / 150 |
| EGL, full returned HDC (cache miss) | 24 / 200 | 24 / 200 |

Steps to verify in one thread, on a fresh prefix/display:

1. `dc = CreateDCA("DISPLAY", NULL, NULL, NULL)`;
   `alias = (HDC)((ULONG_PTR)dc & 0xffff)`.
2. Enumerate `DescribePixelFormat(alias, ...)` and retain an RGB888/0 format
   supporting both windows and bitmaps. This primes the window cache.
3. `DeleteDC(dc)`; `mem = CreateCompatibleDC(NULL)`. Check that the low 16
   bits of `mem` equal the old alias; otherwise retry with a newly primed DC.
4. Select a 4x4, 24-bit DIB into `mem`. `GetObjectType(mem)` is 10
   (`OBJ_MEMDC`) and the selected bitmap reports 24 bits per pixel.
5. Describe the retained index using `alias` first and the full `mem` second.
   C returns colour 32 then 24 for the same current DC and format index.

The stale format-count cache problem predates v2, as the S counts show.
V2 newly makes that stale entry carry normalized colour bits into a memory
DC, violating its scope guarantee. No Windows short-handle behaviour is
assumed or used as an expected value. Canonicalize cache identity to the
validated full handle, avoid caching short aliases, or at least include the
validated DC kind in cache-hit eligibility; add the reuse sequence to coverage.

### P3 test changes

The earlier capability/restore follow-up is addressed for the exercised
catalogs. `opengl32/tests/opengl.c:1079-1087` now requires double buffering,
non-generic RGB888/alpha8, depth24/stencil8 before comparing choices, and
`1110-1113` checks both restoration and context deletion return values.
The normal suite call site's original draw/read binding remains ordinary.

The unchanged extracted v2 test body, with two external assertions checking
the saved current HGLRC/HDC after return, gave GLX 64 checks, 52 failures to
0, and EGL 76 checks, 64 failures to 0. Both external restoration checks
passed on both engines. An additional synthetic catalog wrapper removed
double-buffer support from alpha-bearing descriptors: C took the intended
skip with zero failures (GLX 51 checks / 1 skip; EGL 63 / 1), and retained
the original context. This checks guard control flow, not native Windows
values. The committed Windows L1 results are still for v1, not a new v2 L1.

### Builds and paired suites (2026-11-06)

Applied v2 after the unchanged 27-patch series with `--fuzz=0`, no candidate
offsets. GCC/MinGW 14 in the same Debian 13 container successfully compiled
and linked `opengl32.dll` and `opengl32_test.exe` for x86_64 and i386.
Used the same component-runtime pairing as the first review, with the shared
Unix components restored to S. Complete-tree comparison found differences
only in the x86_64/i386 PE `opengl32.dll` files. This is not a fresh full
release-runtime L0 build. Both sides ran identical v2 test executables in
new private Xvfb displays and fresh prefix/HOME/XDG directories; all eight
suite runs completed server cleanup.

| Full `opengl32_test.exe opengl` pair | S | C | Assessment |
| --- | --- | --- | --- |
| x64 EGL | 9625 checks, 64 failures, 65 todo, 39 skipped; exit 64 | 9917 checks, 0 failures, 65 todo, 39 skipped; exit 0 | Completed; no observed new failures, coverage increased by 292 checks |
| x64 GLX | 180 s timeout, exit -9, no final summary | Same | Incomplete; identical 59 interim failures at 43 locations |
| i386 GLX and EGL | exit 1, no test summary | Same | Incomplete; no suite pass established |

All 64 EGL baseline failures are in the added test: `opengl.c:1076` (60),
`1097`, `1098`, `1109`, `1115` (one each). C has no failing locations.
GLX again reached the thread-wait failure in `test_window_dc` (`4442`),
before the added test. These results do not close GLX/i386 L2 or establish
identical assertion coverage. No native measurements or application smokes
were performed by this re-review.

### v2 verdict

**Not ready for series: P2-2 still allows a window cache entry to reach a
memory DC through an accepted short HDC after slot reuse.** Ordinary full-HDC
memory behaviour and the original P3 follow-up are fixed; window colour-bit
normalization, pbuffer creation and context restoration work in the probes.
The completed x64 EGL pair has zero candidate failures, subject to the
component-runtime and coverage limits above.
