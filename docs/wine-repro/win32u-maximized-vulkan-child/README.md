# Maximized top-level window with a Vulkan child

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
  import library, shader compiler, OpenGL context, D3D12 or DXGI is used.
- `--vulkan --delay-vulkan`: same, but create Vulkan at 10 seconds, five seconds
  into the first maximized phase. Before then the child paints with GDI.

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
nonzero final frame count in Vulkan mode. An early manual close is identified
by `completed_timeline=0` and exits 2. Startup/usage failures also exit 2.

## Build

Requires Python-free Bash, x86_64 MinGW GCC/windres, and system Vulkan headers
(Arch `vulkan-headers`, Debian `libvulkan-dev`).

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

No RX 580, RX 6400, Windows or Wine visual result is recorded yet. This is a
fixture, not a claim that the fault reproduces or that comctl32 needs changing.
The comctl32 v2 task has not been started.

If none of the RX 580 Vulkan runs reproduce, **stop and report that result**.
Next candidates, requiring a separate decision: repeat inside a Wine virtual
desktop; a D3D12/DXGI child swapchain through vkd3d-proton/DXVK; matching the
application's child styles/ownership and swapchain lifecycle more closely; then
an RX 6400 comparison. The GPU rebind is the maintainer's decision, not an
automatic next step. Do not claim that a negative plain-Vulkan result excludes
a vkd3d-specific or ray-tracing-specific trigger.
