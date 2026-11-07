# Maximized top-level window with a Vulkan or D3D12 child

Original, AI-assisted (Codex). LGPL-2.1-or-later.

A synthetic fixture for the [status-bar investigation](../comctl32-statusbar-erase/README.md).
Inventor showed overlapping status text only after GPU ray tracing created a
Vulkan swapchain in a child, and only while maximized, including inside a Wine
virtual desktop. Its erase handler still filled the supplied DC and returned 1.
This fixture asks whether an ordinary child Vulkan swapchain is sufficient.
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

- `--none`: ordinary GDI child; does not load Vulkan.
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
- `--delay`: common spelling for either Vulkan or D3D12; create the device
  and swapchain at 10 seconds. `--delay-vulkan` remains a Vulkan-only alias.
- `--nest=0|1|2`: zero (default), one or two intermediate `WS_CHILD` windows
  between frame and swapchain child. All modes support the same nesting.
  Containers use `WS_CLIPCHILDREN`; each level insets the next child by
  7 px horizontally and 9 px vertically, with symmetric remaining borders.
  This is an explicit synthetic hierarchy, not a claim to reconstruct
  Inventor's exact window styles or ownership. `NEST` logs container screen
  rects; `GEOMETRY` always reports the innermost swapchain child.

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
nonzero final frame count in either graphics mode. D3D12 occlusion is logged
on transitions and counted separately; occluded presents do not count as frames.
An early manual close is identified
by `completed_timeline=0` and exits 2. Startup/usage failures also exit 2.

## Build

Requires Python-free Bash, x86_64 MinGW GCC/windres, and system Vulkan headers
(Arch `vulkan-headers`, Debian `libvulkan-dev`). The MinGW installation must
also provide `d3d12.h`, `dxgi1_4.h` and `libdxguid.a` (tested with MinGW-w64
12 headers/GCC 14.2). `libdxguid` supplies interface GUIDs only; D3D12/DXGI
entry points are loaded dynamically. No additional Windows SDK is required.

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

## Maintainer sequence: RX 580 first

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
rg '^(START|CONTROL|DEVICE|VULKAN|GEOMETRY|ERROR|SUMMARY)' "$WINEPREFIX/none.txt" "$WINEPREFIX/vulkan.txt" "$WINEPREFIX/vulkan-delay.txt"
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

The RX 580 Vulkan results below motivated the D3D12/nesting extension now
authorized by the maintainer. D3D12 and nested-window visual results remain
pending. No Wine patch or comctl32 v2 work is included. The RX 6400 still
requires the maintainer's separate decision and GPU rebind.

## D3D12 experiment: isolated prefix and RX 580 commands

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
export REPRO_BUNDLE=/path/to/new-d3d12-repaint-bundle
export WINE="$HOME/.local/share/inventor-on-linux/wine-inventor-11.20-6-x86_64/bin/wine"
export DOWNLOAD_DIR="$INVENTOR_REPO/downloads"
export WINEDEBUG=-all
export WINEARCH=win64
export VKD3D_DEBUG=warn
export DXVK_LOG_LEVEL=warn
unset WINEDLLOVERRIDES
bash "$WINUM_REPO/scripts/build-repro-maximized-vulkan-child.sh" "$REPRO_BUNDLE"
```

EGL/default X11-driver configuration, new prefix. The prefix path itself must
not exist; `mktemp` creates only its parent experiment directory:

```sh
export REPRO_CASE_ROOT="$(mktemp -d "$HOME/.cache/winum-repaint-d3d12-egl.XXXXXX")"
export WINEPREFIX="$REPRO_CASE_ROOT/prefix"
export DXVK_LOG_PATH="$REPRO_CASE_ROOT"
bash "$WINUM_REPO/scripts/setup-repro-d3d12-prefix.sh" "$INVENTOR_REPO" "$WINEPREFIX"
for depth in 0 1 2; do
    "$WINE" "$REPRO_BUNDLE/maximized-vulkan-child.exe" --none "--nest=$depth" > "$REPRO_CASE_ROOT/none-nest$depth.txt" 2>&1
    "$WINE" "$REPRO_BUNDLE/maximized-vulkan-child.exe" --d3d12 "--nest=$depth" > "$REPRO_CASE_ROOT/d3d12-nest$depth.txt" 2>&1
    "$WINE" "$REPRO_BUNDLE/maximized-vulkan-child.exe" --d3d12 --delay "--nest=$depth" > "$REPRO_CASE_ROOT/d3d12-delay-nest$depth.txt" 2>&1
done
```

Then GLX (`UseEGL=N` per EXE) in **another fresh prefix**, with the same DLL
versions and copies:

```sh
export REPRO_CASE_ROOT="$(mktemp -d "$HOME/.cache/winum-repaint-d3d12-glx.XXXXXX")"
export WINEPREFIX="$REPRO_CASE_ROOT/prefix"
export DXVK_LOG_PATH="$REPRO_CASE_ROOT"
bash "$WINUM_REPO/scripts/setup-repro-d3d12-prefix.sh" "$INVENTOR_REPO" "$WINEPREFIX"
"$WINE" reg add 'HKCU\Software\Wine\AppDefaults\maximized-vulkan-child.exe\X11 Driver' /v UseEGL /t REG_SZ /d N /f
for depth in 0 1 2; do
    "$WINE" "$REPRO_BUNDLE/maximized-vulkan-child.exe" --none "--nest=$depth" > "$REPRO_CASE_ROOT/none-nest$depth.txt" 2>&1
    "$WINE" "$REPRO_BUNDLE/maximized-vulkan-child.exe" --d3d12 "--nest=$depth" > "$REPRO_CASE_ROOT/d3d12-nest$depth.txt" 2>&1
    "$WINE" "$REPRO_BUNDLE/maximized-vulkan-child.exe" --d3d12 --delay "--nest=$depth" > "$REPRO_CASE_ROOT/d3d12-delay-nest$depth.txt" 2>&1
done
```

Each matrix takes about 4.5 minutes. As in the original Vulkan runs, the
EGL/GLX registry setting is an X11-driver configuration comparison; this EXE
does not create an OpenGL context. Vulkan can also be compared at any nesting
depth using `--vulkan --delay --nest=1` (or `2`).

For each prefix, return only these fixture lines and the visual result, naming
the depth and delayed/immediate case:

```sh
rg '^(CONFIG|START|CONTROL|DEVICE|D3D12|NEST|GEOMETRY|ERROR|SUMMARY)' "$REPRO_CASE_ROOT"/*.txt
```

Expected healthy D3D12 output includes `CONFIG mode=d3d12 delay=0|1
nesting=0|1|2`, `CONTROL ... themed=1`, `DEVICE api=d3d12 ...` identifying the
RX 580 (possibly named RX 480, vendor/device `1002:67df`), and
`D3D12 started=1 ... swap_effect=FLIP_DISCARD`. Every phase should have a
`GEOMETRY` record, extents should match the innermost child, frame counts must
advance after creation, and the final lines should show
`D3D12 occluded_presents=0` and `SUMMARY failed=0 ... completed_timeline=1`.
In delayed runs `frames=0` before 10 s is intentional. Any `ERROR`, no
D3D12 startup, non-advancing frames or occlusion makes that comparison
incomplete. A healthy summary still says nothing about visual correctness.
Observe/capture at the same times as above; if all cases remain clean, report
the negative result and stop before another GPU or a Wine fix.

D3D12 extension validation: MinGW `-Werror` build and PE imports checked;
actual pinned archives verified and helper copy/override/refusal behavior
checked with mocked Wine. A second assistant reviewed the D3D12 lifetime,
nesting and helper isolation; occlusion and cleanup-error reporting findings
were addressed. This does **not** constitute D3D12 runtime or GPU
validation. The RX 580 D3D12/nesting matrix remains for the maintainer.

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
