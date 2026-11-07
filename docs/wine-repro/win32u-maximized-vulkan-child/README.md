# Maximized window: OpenGL, Vulkan and D3D12 children

Original, AI-assisted (Codex). LGPL-2.1-or-later.

A synthetic fixture for the [status-bar investigation](../comctl32-statusbar-erase/README.md).
Inventor showed overlapping status text only after GPU ray tracing created a
Vulkan swapchain in a child, and only while maximized, including inside a Wine
virtual desktop. Its erase handler still filled the supplied DC and returned 1.
This fixture isolates plain Vulkan/D3D12 presentation and then concurrent
OpenGL/D3D12 in one window tree.
It contains no Autodesk code, assets, vendor binaries, ray tracing or Wine fix.

## What it does

One x86_64 console EXE creates a `WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN`
top-level window, a child filling the area above a common-controls v6 status
bar, and two timers. A parent-installed status-bar subclass fills its client
rect light grey (`#c8d0d8`) with `ExtTextOutW(ETO_OPAQUE)` on the supplied
`WM_ERASEBKGND` DC and returns 1 (Inventor fills dark; the mechanism is the
same, and a light fill keeps the theme's dark text readable). `SB_SETTEXTW` alternates long text and
`Ready` every 250 ms; no extra manual erase is added to the text setter.
The manifest requests comctl32 v6 and system DPI awareness.

- `--none`: ordinary GDI child; no graphics context or swapchain.
- `--vulkan`: load `vulkan-1.dll` at runtime and use `vkGetInstanceProcAddr`;
  create `VK_KHR_win32_surface` on the **child**, a graphics/present queue,
  and a FIFO swapchain. Clear and present continuously, with one frame in
  flight, queue-idle synchronization for command/acquire reuse, and a present
  semaphore per swapchain image, reused only after reacquiring that image.
  The blue clear slowly changes shade to show that frames continue. No Vulkan
  import library, shader compiler or OpenGL context is used in this mode.
- `--vulkan --delay-vulkan`: same, but create Vulkan at 10 seconds, five seconds
  into the first maximized phase. Before then the child paints with GDI.
- `--d3d12`: dynamically load `d3d12.dll`/`dxgi.dll`, select a hardware
  D3D12 device (feature level 11_0, no WARP fallback), and call
  `CreateSwapChainForHwnd` on the same child. Two RGBA8 backbuffers use
  `DXGI_SWAP_EFFECT_FLIP_DISCARD`; command lists transition PRESENT →
  RENDER_TARGET → PRESENT around each clear. `Present(1, 0)` and a GPU fence
  serialize frames and backbuffer/allocator reuse. Resize waits for the GPU,
  drops recorded/backbuffer references, calls `ResizeBuffers`, then rebuilds
  render-target views. This exercises vkd3d-proton plus DXVK's DXGI, with no
  ray tracing, shaders or proprietary code.
- `--gl`: legacy WGL context on the innermost viewport child (`CS_OWNDC`).
  `ChoosePixelFormat` requests a window RGBA format, 32-bit colour, 24-bit depth,
  double buffering, **zero requested alpha** and no requested stencil.
  `SetPixelFormat` runs once, followed by `wglCreateContext`; no extensions or
  ARB format selection are used. The request is the simple 32-bit/no-alpha
  choice from the [pixel-format investigation](../opengl32-pixel-format-alpha/README.md),
  not Inventor's separate two-stage capability check. The actual format may
  include alpha: it is printed, not forced to zero. Every render tick uses
  `glViewport`, `glClear` (colour/depth) and `SwapBuffers`. Green shades identify GL.
- `--gl --d3d12`: GL starts before the timeline and continues while D3D12 runs.
  The WGL context remains current on the main thread during D3D12 creation and
  presentation. `--d3d12-placement=child` (default) creates a **new HWND inside
  the GL child**, centred at half its width/height, leaving a visible green GL
  border around blue D3D12. `--d3d12-placement=sibling` creates a **new HWND
  with the same parent as the GL child**. GL occupies the left half, D3D12 the
  right; that space is reserved from the beginning so starting D3D12 does not
  itself resize GL. The D3D12 HWND/device/swapchain are all created when D3D12
  starts, at 10 s with `--delay`, otherwise on the first render tick.
  Both layouts keep the requested `--nest` chain above GL; the inside-child
  variant adds one more level for the swapchain. These are deliberately
  different HWNDs: no GL and DXGI presentation target the same HWND.
- `--delay`: common spelling for either Vulkan or D3D12; create the device
  and swapchain at 10 seconds. In `--gl --d3d12` it delays **only D3D12**;
  GL already runs before the first maximize at 5 s. GL-only `--delay` is rejected.
  `--delay-vulkan` remains a Vulkan-only alias. GL cannot combine with `--none`
  or `--vulkan`; placement options require `--gl --d3d12`.
- `--nest=0|1|2`: zero (default), one or two intermediate `WS_CHILD` windows
  between frame and swapchain child. All modes support the same nesting.
  Containers use `WS_CLIPCHILDREN`; each level insets the next child by
  7 px horizontally and 9 px vertically, with symmetric remaining borders.
  This is an explicit synthetic hierarchy, not a claim to reconstruct
  Inventor's exact window styles or ownership. `NEST` logs container screen
  rects; `GEOMETRY` reports the D3D12 HWND once created, otherwise the original
  viewport child. `GL_GEOMETRY` separately reports GL screen/client rects and
  its frame count. In GL-only mode swapchain `extent`/`frames` stay zero;
  `gl_frames` is the useful counter.

The fixed timeline starts after window creation; the title identifies phases:

| Elapsed | State | Observe |
| --- | --- | --- |
| 0–5 s | restored | baseline text and client edges |
| 5–15 s | maximized | compare before/after 10 s with delayed start |
| 15–20 s | restored | does any overlap disappear? |
| 20–30 s | maximized again | does it return? |

At 30 seconds the EXE closes itself. Do not manually resize, minimize, move or
cover the window during a comparison. Vulkan errors, phase geometry, child and
status screen rects, `IsZoomed`, swapchain extent and final frame count go to
stdout. `DEVICE` identifies the selected GPU: confirm it is the RX 580.
`CONTROL` reports the actual comctl32 class version (must be 6) and whether
the status bar has a theme. If `themed=0`, report that and do not treat it as
a themed-v6 comparison. A phase line may still show the previous extent; the following
`event=swapchain` line reports the recreated extent. Expected resize
`OUT_OF_DATE`/`SUBOPTIMAL` results are logged and retried. Other errors exit 2.
A zero exit status is **not** a visual pass; check `completed_timeline=1` and a
nonzero final frame count for each active API (`frames` for Vulkan/D3D12,
`gl_frames` for GL). D3D12 occlusion is logged
on transitions and counted separately; occluded presents do not count as frames.
An early manual close is identified
by `completed_timeline=0` and exits 2. Startup/usage failures also exit 2.

## Build

Requires Python-free Bash, x86_64 MinGW GCC/windres, and system Vulkan headers
(Arch `vulkan-headers`, Debian `libvulkan-dev`). The MinGW installation must
also provide `d3d12.h`, `dxgi1_4.h` and `libdxguid.a` (tested with MinGW-w64
12 headers/GCC 14.2). `libdxguid` supplies interface GUIDs only; D3D12/DXGI
entry points are loaded dynamically. WGL/OpenGL 1.1 also loads dynamically:
only `--gl` loads `opengl32.dll` and creates a GL context, preserving the
original controls' DLL-loading behavior. No additional Windows SDK is required.

```sh
bash scripts/build-repro-maximized-vulkan-child.sh /path/to/new-repaint-bundle
```

An alternate header installation can be selected with a separate export:

```sh
export VULKAN_HEADERS_INCLUDE=/path/to/include
bash scripts/build-repro-maximized-vulkan-child.sh /path/to/another-new-bundle
```

The builder copies only portable Vulkan headers into a temporary include tree;
it does not mix Linux libc headers into MinGW. It refuses existing output paths
and builds with `-Wall -Wextra -Werror`. The bundle contains the EXE, original
source, manifest, README, license and SHA-256 sums. No registry exports or logs
are added to the repository.

## Original Vulkan-only sequence (historical reference)

The current requested runs are the six-run GL+D3D12 sequence below. These
original commands document the preceding Vulkan-only experiment.
Keep the RX 580 selected. Use the same visible, quiet desktop for all runs.
Replace the bundle path below. These commands select the requested Inventor
baseline without changing any existing prefix. Wine may populate its normal
fresh-prefix components; wait for `wineboot` to finish before running the EXE.
No native DLL overrides are required. Keep the Vulkan loader provided by Wine.

```sh
export WINE="$HOME/.local/share/inventor-on-linux/wine-inventor-11.20-6-x86_64/bin/wine"
export REPRO_BUNDLE=/path/to/new-repaint-bundle
export WINEPREFIX="$(mktemp -d "$HOME/.cache/winum-repaint-rx580.XXXXXX")"
export WINEDEBUG=-all
"$WINE" wineboot -i
"$(dirname "$WINE")/wineserver" -w
"$WINE" "$REPRO_BUNDLE/maximized-vulkan-child.exe" --none > "$WINEPREFIX/none.txt" 2>&1
"$WINE" "$REPRO_BUNDLE/maximized-vulkan-child.exe" --vulkan > "$WINEPREFIX/vulkan.txt" 2>&1
"$WINE" "$REPRO_BUNDLE/maximized-vulkan-child.exe" --vulkan --delay-vulkan > "$WINEPREFIX/vulkan-delay.txt" 2>&1
```

Then the GLX variant, using another fresh prefix and the per-EXE `UseEGL=N`
setting used in the investigation (no inline environment assignments):

```sh
export WINEPREFIX="$(mktemp -d "$HOME/.cache/winum-repaint-rx580-glx.XXXXXX")"
"$WINE" wineboot -i
"$(dirname "$WINE")/wineserver" -w
"$WINE" reg add 'HKCU\Software\Wine\AppDefaults\maximized-vulkan-child.exe\X11 Driver' /v UseEGL /t REG_SZ /d N /f
"$WINE" "$REPRO_BUNDLE/maximized-vulkan-child.exe" --none > "$WINEPREFIX/none.txt" 2>&1
"$WINE" "$REPRO_BUNDLE/maximized-vulkan-child.exe" --vulkan > "$WINEPREFIX/vulkan.txt" 2>&1
"$WINE" "$REPRO_BUNDLE/maximized-vulkan-child.exe" --vulkan --delay-vulkan > "$WINEPREFIX/vulkan-delay.txt" 2>&1
```

This fixture itself does not create an EGL/GLX context. The registry variant
checks the driver configuration used by Inventor; it does not prove a different
Vulkan presentation path was selected.

For each run, look at the status strip when long text changes to `Ready`:
old letters must disappear, and the new baseline must stay at the same height.
Also inspect thin strips along the top/left child edges and whether the client
content is offset relative to the status bar. Capture the whole synthetic
window (including title and borders) around **12–14 s**, **17–19 s** and
**25–28 s** if an offset/overlap appears. With delayed Vulkan, also capture
**8–9 s**, before Vulkan starts. Report the mode, phase, visible result and
whether restoration clears it and the second maximization brings it back.
Do not infer a pass from a pixel capture by the EXE: it cannot reliably inspect
Wine's final on-screen composition. It deliberately implements no screenshot
oracle or automated visual assertion.

Return only the fixture's bounded text, for each prefix, for example:

```sh
grep -E '^(START|CONTROL|DEVICE|VULKAN|GEOMETRY|ERROR|SUMMARY)' "$WINEPREFIX/none.txt" "$WINEPREFIX/vulkan.txt" "$WINEPREFIX/vulkan-delay.txt"
```

Finally, a **program sanity check only** in the offline Windows 10 x64 VM:
copy the bundle, open `cmd.exe` in it, and run:

```bat
maximized-vulkan-child.exe --none
maximized-vulkan-child.exe --vulkan
maximized-vulkan-child.exe --vulkan --delay-vulkan
```

Vulkan modes require a working Windows Vulkan driver. A VM without Vulkan
can sanity-check only `--none`; an explicit loader/device error is not evidence
about Wine's fault. No Windows DLLs or VM contents need to be copied out.

## Evidence and stopping point

2026-11-07: the x86_64 bundle compiled with Debian MinGW GCC 14.2 and
Vulkan headers 1.4.309 using `-Wall -Wextra -Werror`. The embedded manifest
and PE imports were inspected; Vulkan is loaded dynamically. Build repeatability,
checksums and refusal of an existing output directory were checked. A second
assistant reviewed the complete source/build script; its present-semaphore reuse
finding was addressed with per-image semaphores. It identified no recognizable
project-specific copied structure; this is not an exhaustive provenance claim.

At the initial build, no GPU/Windows visual run had been recorded; the later
RX 580 results below cover the original direct-child Vulkan fixture. This is a
fixture, not a claim that the fault reproduces or that comctl32 needs changing.
The comctl32 v2 task has not been started.

The RX 580 Vulkan and D3D12 results below motivated concurrent GL+D3D12.
Concurrent-API visual results remain pending. No Wine patch or comctl32 v2 work is included. The RX 6400 still
requires the maintainer's separate decision and GPU rebind.

## GL+D3D12 experiment: isolated prefix and six RX 580 runs

The source of the pins and copy/override policy is Inventor's
[`scripts/lib/common.sh`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/996576aa07b7ff153d341496d23bb76021db1fa2/scripts/lib/common.sh),
`install_dxvk` and `install_vkd3d` at commit
`996576aa07b7ff153d341496d23bb76021db1fa2`:

| Archive | SHA-256 |
| --- | --- |
| `dxvk-2.7.1.tar.gz` | `d85ce7c79f57ecd765aaa1b9e7007cb875e6fde9f6d331df799bce73d513ce87` |
| `vkd3d-proton-3.0.1.tar.zst` | `3cf2315522af5e43605ef6d3c41dad91387040bf97199934f3f7ab76caaa2f0c` |

The helper reads the literal pinned defaults from the supplied checkout's
`common.sh`, without sourcing it or loading Inventor config. It requires these
versions and verifies SHA-256 before extraction or prefix creation. It takes
archives from `DOWNLOAD_DIR` (default `<Inventor checkout>/downloads`) when
present; otherwise it downloads the pinned official GitHub release into a
temporary directory. Existing cache files are never overwritten or deleted,
even on mismatch. A custom installer cache can be selected with a separate
`export DOWNLOAD_DIR=...` line. This uses the **stock pinned 3.0.1** archive,
not the separate patched ray-payload build; no ray tracing is performed here.

Like the installer, it copies DXVK's x64/x32 DLLs (`dxgi`, `d3d8`, `d3d9`,
`d3d10core`, `d3d11`) and vkd3d-proton's x64/x86 `d3d12`/`d3d12core` into
`system32`/`syswow64`, respectively, and sets registry DllOverrides to `native`
for all seven DLLs plus `d3dcompiler_47`. The clear-only EXE uses only DXGI and
D3D12; it does not need a shader compiler DLL. A separate prefix holds **only
this fixture**. The helper refuses any existing destination, paths outside
`~/.cache`, and nesting inside another prefix. It never selects or modifies
`~/.wine-inventor2027`. Failed partial prefixes are retained for diagnosis;
retry with a new directory rather than reusing one.

First build a new bundle and select paths (replace the three checkout/bundle
paths). Helper dependencies: Bash, Python 3, curl, sha256sum, tar and zstd.
Use the RX 580/RADV and the existing Inventor Wine 11.20-6 runtime. Do not set
`WINEDLLOVERRIDES` in the caller: it would take priority over registry settings.

```sh
export WINUM_REPO=/path/to/winum
export INVENTOR_REPO=/path/to/Autodesk-Inventor-for-Linux
export REPRO_BUNDLE=/path/to/new-gl-d3d12-repaint-bundle
export WINE="$HOME/.local/share/inventor-on-linux/wine-inventor-11.20-6-x86_64/bin/wine"
export DOWNLOAD_DIR="$INVENTOR_REPO/downloads"
export WINEDEBUG=-all
export WINEARCH=win64
export VKD3D_DEBUG=warn
export DXVK_LOG_LEVEL=warn
unset WINEDLLOVERRIDES
bash "$WINUM_REPO/scripts/build-repro-maximized-vulkan-child.sh" "$REPRO_BUNDLE"
```

Keep the RX 580 selected. Run the same **six** cases below in each prefix,
most Inventor-like first. Cases 1–2 start GL immediately, maximize at 5 s,
then create D3D12 at 10 s. Cases 3–4 compare immediate D3D12 creation, case 5
is GL alone, and case 6 removes the two intermediate parents. All last 30 s,
so the matrix takes about three minutes per prefix.

EGL/default X11-driver configuration, new prefix. The prefix path itself must
not exist; `mktemp` creates only its parent experiment directory. This reuses
the same verified D3D12-prefix helper as the previous experiment; it does not
touch the Inventor prefix. Always use a new bundle path for the new EXE.

```sh
export REPRO_CASE_ROOT="$(mktemp -d "$HOME/.cache/winum-repaint-gl-d3d12-egl.XXXXXX")"
export WINEPREFIX="$REPRO_CASE_ROOT/prefix"
export DXVK_LOG_PATH="$REPRO_CASE_ROOT"
bash "$WINUM_REPO/scripts/setup-repro-d3d12-prefix.sh" "$INVENTOR_REPO" "$WINEPREFIX"
"$WINE" "$REPRO_BUNDLE/maximized-vulkan-child.exe" --gl --d3d12 --d3d12-placement=child --delay --nest=2 > "$REPRO_CASE_ROOT/01-child-delay-n2.txt" 2>&1
"$WINE" "$REPRO_BUNDLE/maximized-vulkan-child.exe" --gl --d3d12 --d3d12-placement=sibling --delay --nest=2 > "$REPRO_CASE_ROOT/02-sibling-delay-n2.txt" 2>&1
"$WINE" "$REPRO_BUNDLE/maximized-vulkan-child.exe" --gl --d3d12 --d3d12-placement=child --nest=2 > "$REPRO_CASE_ROOT/03-child-immediate-n2.txt" 2>&1
"$WINE" "$REPRO_BUNDLE/maximized-vulkan-child.exe" --gl --d3d12 --d3d12-placement=sibling --nest=2 > "$REPRO_CASE_ROOT/04-sibling-immediate-n2.txt" 2>&1
"$WINE" "$REPRO_BUNDLE/maximized-vulkan-child.exe" --gl --nest=2 > "$REPRO_CASE_ROOT/05-gl-only-n2.txt" 2>&1
"$WINE" "$REPRO_BUNDLE/maximized-vulkan-child.exe" --gl --d3d12 --d3d12-placement=child --delay --nest=0 > "$REPRO_CASE_ROOT/06-child-delay-n0.txt" 2>&1
grep -E '^(CONFIG|START|CONTROL|GL|DEVICE|D3D12|NEST|GEOMETRY|ERROR|SUMMARY)' "$REPRO_CASE_ROOT"/*.txt
```

Then GLX (`UseEGL=N` for this EXE), with another dedicated prefix and the same
six cases. With a real GL context active, this now exercises the EGL/GLX path
rather than just a registry configuration for a Vulkan-only program.

```sh
export REPRO_CASE_ROOT="$(mktemp -d "$HOME/.cache/winum-repaint-gl-d3d12-glx.XXXXXX")"
export WINEPREFIX="$REPRO_CASE_ROOT/prefix"
export DXVK_LOG_PATH="$REPRO_CASE_ROOT"
bash "$WINUM_REPO/scripts/setup-repro-d3d12-prefix.sh" "$INVENTOR_REPO" "$WINEPREFIX"
"$WINE" reg add 'HKCU\Software\Wine\AppDefaults\maximized-vulkan-child.exe\X11 Driver' /v UseEGL /t REG_SZ /d N /f
"$WINE" "$REPRO_BUNDLE/maximized-vulkan-child.exe" --gl --d3d12 --d3d12-placement=child --delay --nest=2 > "$REPRO_CASE_ROOT/01-child-delay-n2.txt" 2>&1
"$WINE" "$REPRO_BUNDLE/maximized-vulkan-child.exe" --gl --d3d12 --d3d12-placement=sibling --delay --nest=2 > "$REPRO_CASE_ROOT/02-sibling-delay-n2.txt" 2>&1
"$WINE" "$REPRO_BUNDLE/maximized-vulkan-child.exe" --gl --d3d12 --d3d12-placement=child --nest=2 > "$REPRO_CASE_ROOT/03-child-immediate-n2.txt" 2>&1
"$WINE" "$REPRO_BUNDLE/maximized-vulkan-child.exe" --gl --d3d12 --d3d12-placement=sibling --nest=2 > "$REPRO_CASE_ROOT/04-sibling-immediate-n2.txt" 2>&1
"$WINE" "$REPRO_BUNDLE/maximized-vulkan-child.exe" --gl --nest=2 > "$REPRO_CASE_ROOT/05-gl-only-n2.txt" 2>&1
"$WINE" "$REPRO_BUNDLE/maximized-vulkan-child.exe" --gl --d3d12 --d3d12-placement=child --delay --nest=0 > "$REPRO_CASE_ROOT/06-child-delay-n0.txt" 2>&1
grep -E '^(CONFIG|START|CONTROL|GL|DEVICE|D3D12|NEST|GEOMETRY|ERROR|SUMMARY)' "$REPRO_CASE_ROOT"/*.txt
```

Expected output for combination runs:

- `CONFIG mode=gl+d3d12 ... placement=child|sibling` and `CONTROL ... themed=1`.
- `GL_FORMAT ... requested_color=32 requested_depth=24 requested_alpha=0` and
  the actual alpha/depth/colour/flags. An actual nonzero alpha is permitted;
  compare the recorded actual formats between EGL and GLX.
- `GL_DEVICE` identifies the OpenGL renderer; confirm it is the RX 580 and
  not a software renderer. `GL started=1` precedes the timeline's first phase.
- `GL_GEOMETRY ... gl_frames=...` counts should advance before and after
  D3D12 creation. In delayed runs they must already be nonzero at the first
  maximize, when D3D12 `frames=0` and `d3d12_child=0` are still intentional.
- `D3D12_WINDOW placement=... created_at_ms=...` appears around 10000 ms with
  `--delay`, near zero otherwise. `D3D12 started=1 ... FLIP_DISCARD` follows;
  `DEVICE api=d3d12 ...` should identify the same GPU as GL.
- After creation `GEOMETRY` records the D3D12 HWND and its swapchain extent;
  `GL_GEOMETRY` remains separate. Both frame counters should keep advancing.
- Final `D3D12 occluded_presents=0` and
  `SUMMARY failed=0 frames=... gl_frames=... completed_timeline=1`, with both
  counters nonzero. GL-only deliberately has `frames=0`, positive `gl_frames`,
  no D3D12 startup and `placement=none`.

Look for the same status-text overlap, changed text baseline and displaced
viewport borders as before. Green is GL; blue is D3D12. A blue centre with a
green border in the inside-child case, or green left/blue right in the sibling
case, is the intended layout. A brief presentation change when the D3D12 HWND
appears at 10 s is not itself the status-bar fault. Capture the whole window at
8–9 s (GL only), 12–14 s (D3D12 running, maximized), 17–19 s (restored) and
25–28 s (maximized again) if something changes. Report the case number and
EGL/GLX setting, the bounded output above and whether restore clears the fault.
Any API error, no D3D12 startup, non-advancing required counter or occluded
presents makes that visual comparison incomplete. A healthy summary is not a
visual pass. If all six cases per prefix stay clean, report the negative result
and stop before changing GPU or writing a Wine patch.

Prior D3D12 extension validation: MinGW `-Werror` build and PE imports checked;
actual pinned archives verified and helper copy/override/refusal behavior
checked with mocked Wine. The maintainer subsequently ran the D3D12 helper and
four EGL D3D12 cases successfully; see Results. The GL extension's build and
review checks do not constitute GL+D3D12 runtime or GPU validation; those new
visual results remain pending. On 2026-11-07 the GL extension compiled with
MinGW GCC 14.2 using `-Wall -Wextra -Werror`; the bundle hashes and absence
of OpenGL/Vulkan/D3D12/DXGI static imports were checked. All shell
snippets passed `bash -n`; both active matrices contain exactly six identical
case commands in the documented order. A second assistant reviewed WGL/DC
lifetime, delayed HWND creation and layout, and identified no recognizable
project-specific copied structure (not an exhaustive provenance claim). Its
background-erase observation was addressed by restricting the new class
background brushes to GL runs; original modes retain their previous painting.


## Results

### 2026-11-07, RX 580, EGL, `wine-inventor-11.20-6`

Fresh prefix, `WINEDEBUG=-all`. `CONTROL version=6.0.2600 themed=1` in all
three runs. Vulkan device reported as `Radeon (TM) RX 480 Graphics`
(1002:67df, the RX 580). The only Vulkan driver installed is RADV
(`vulkan-radeon`); the marketing name comes from libdrm's device table. `--none`: 0 frames, `--vulkan`: 1170 frames,
`--vulkan --delay-vulkan`: 1754 frames; all `completed_timeline=1`,
`failed=0`. Maintainer's visual result: **no overlap and no offset in any
phase of any run.** So a plain Vulkan swapchain in a child of a maximised
window is not enough on this setup.

The first build stopped with "status class is not comctl32 v6": on Wine the
global status bar class has no class HINSTANCE; the check now resolves the
module of the class procedure (`e34acbe`).

### 2026-11-07, RX 580, GLX (`UseEGL=N` for the EXE)

Fresh prefix, `--vulkan --delay-vulkan`: 1174 frames,
`completed_timeline=1`, `failed=0`. Visual result: **no overlap**.

Not reproduced with plain Vulkan under EGL or GLX on the RX 580. Next
candidates (unchanged): a D3D12/DXGI swapchain through vkd3d-proton and
DXVK's dxgi as in Inventor; Inventor-like child nesting and swapchain
lifecycle; the RX 6400 (maintainer's decision, needs a GPU rebind).

### 2026-11-07, RX 580, D3D12 (vkd3d-proton 3.0.1, DXVK 2.7.1 dxgi), EGL

Prefix from `setup-repro-d3d12-prefix.sh` (both archives from the Inventor
download cache, SHA-256 verified). Four runs: `--d3d12` with `--nest=0` and
`--nest=2`, each immediate and `--delay`. All: `D3D12 started=1`,
`FLIP_DISCARD`, `occluded_presents=0`, 1157-1737 frames, `failed=0`,
`completed_timeline=1`. Visual result: **no overlap in any run.**

So neither a plain Vulkan nor a plain D3D12 swapchain in a (nested) child of
a maximised window reproduces the fault on the RX 580. One thing Inventor
has and the fixture does not: while GPU ray tracing runs, Inventor's
viewport also has an OpenGL context (the pixel format work in
`opengl32-window-format-color-bits` is about that context, GLX with
`UseEGL=N`). Next candidate: an OpenGL child and a D3D12 swapchain in the
same window tree at the same time.

### 2026-11-07, RX 580, GL + D3D12, GLX (`UseEGL=N`)

Stock `wine-inventor-11.20-6` (without the colour bits patch: the chosen
format is index 17, 32-bit colour with 8 alpha bits), radeonsi for GL,
RADV/vkd3d-proton for D3D12. Runs: GL + delayed D3D12 inside the GL child
(`--nest=2`), GL + delayed D3D12 as a sibling (`--nest=2`), GL alone. All
`failed=0`, `completed_timeline=1`, GL about 1690 frames, D3D12 about 1120
frames. Visual result: **no overlap.**

Summary of the fixture so far: on the RX 580 alone, no combination of a
maximised window with Vulkan, D3D12, OpenGL or OpenGL + D3D12 children
reproduces the fault. What Inventor's failing runs had in addition:
rendering on a **second GPU** (RX 6400 via `DRI_PRIME`, display on the
RX 580 - a PRIME offload path), GPU ray tracing itself, and the patched
vkd3d-proton. Lukas10999's NVIDIA report has to be checked for a second
GPU too. Next, when the RX 6400 is bound to amdgpu again: the same fixture
runs with `DRI_PRIME=pci-0000_23_00_0` and `INV_GPU`-style device
filters, before going back to Inventor.

### 2026-11-07, RX 6400 through PRIME offload, GL + D3D12, GLX

RX 6400 bound to amdgpu, display on the RX 580, `DRI_PRIME=pci-0000_23_00_0`,
`VKD3D_FILTER_DEVICE_NAME=6400`, `DXVK_FILTER_DEVICE_NAME=6400`, stock
`wine-inventor-11.20-6`, stock vkd3d-proton 3.0.1. GL renderer "AMD Radeon
RX 6400 (radeonsi, navi24)", D3D12 device "AMD Radeon RX 6400 (RADV NAVI24)"
in every run, so both APIs rendered on the RX 6400.

| Run | Result |
| --- | --- |
| GL + delayed D3D12 inside the GL child, `--nest=2` | **hung** at the end: the window stayed in "maximized 20-30s" and did not close at 30 s; last lines: `GL_GEOMETRY phase=3 ... gl_frames=1119`, then vkd3d-proton's "Got 3 swapchain images" (swapchain recreated after the second maximise); no `ERROR`; killed with Ctrl+C |
| GL + delayed D3D12 as a sibling, `--nest=2` | **hung** the same way; killed |
| D3D12 alone, delayed, `--nest=2` | completed, 1154 frames |
| GL alone, `--nest=2` | completed, 1705 GL frames |

A screenshot during the first run (phase "maximized 20-30s", GL and D3D12
both visible) shows a clean status bar. `dmesg` around the runs has
`amdgpu 0000:10:00.0: [drm] *ERROR* Unsupported screen format RA24` (the
RX 580, the display GPU); whether it belongs to these runs is not known.

So OpenGL and D3D12 rendering at the same time on the offload GPU blocks
after the window is maximised again and the swapchain is recreated, while
either API alone does not. The fence wait in the fixture times out after
5 s with an `ERROR` line, and none was printed, so the block is in a call
without a timeout (SwapBuffers, Present or ResizeBuffers). Next: a
backtrace of the hung process (`winedbg`).

Visual result (maintainer): the status text was **correct in all four
runs** - no overlap on the offload GPU either.

#### The hang, repeated and backtraced (2026-11-07, 04:58)

Not deterministic: of five GL + delayed D3D12 runs on the RX 6400, three hung
(the first two, and the first of a loop of three), two completed
(`SUMMARY failed=0 frames=1112 gl_frames=1679`). Nothing was touched during
the runs; the earlier screenshot was taken only after the hang.

`winedbg --command "bt all"` on the hung process (the runtime has no debug
symbols, so only module offsets):

- main thread: `opengl32 +0xbe7cc` <- fixture `+0x1d08` (in `frame_gl`,
  i.e. `wglMakeCurrent` or `SwapBuffers`) <- user32 timer dispatch;
- one vkd3d-proton thread: `winevulkan +0x1cefa` <- `d3d12core +0x60f34`
  (a Vulkan call in flight, most likely the presentation thread);
- the other d3d12core threads wait in ntdll (normal idle waits).

So the GL swap on the main thread blocks while vkd3d-proton's present
thread is inside a Vulkan call, both presenting into one window tree on the
offload GPU. This is a hang, not the status text overlap (the text was
correct in all runs), and it may be outside Wine (Mesa's GLX/DRI3 and
Vulkan X11 WSI with PRIME). Next, if followed up: the Unix side of the same
threads with `gdb -p <wine process> -batch -ex "thread apply all bt"`
(Mesa, xcb), which shows what each one waits for.
