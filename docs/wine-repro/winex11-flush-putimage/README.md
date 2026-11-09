# L3: send small PutImage updates before the next event

Original, AI-assisted (Codex), LGPL-2.1-or-later. An own Win32 host starts
an own second process which creates a child under the host's window. The
writer obtains a DC once and sends six tiny 8x8 red/blue DIB updates, 1200 ms
apart, without a message pump, DC release or explicit GdiFlush between
them. Tiny writes avoid filling Xlib's request buffer by accident. An
independent host-process timer samples the foreign child's DC through GetPixel
and prints the first sample plus subsequent color changes. A DC read is a
pixel-observation proxy: paired runtime/call evidence must establish that
this Wine configuration reads the direct drawable. The host is a separate
process, so it does not explicitly flush the writer's X connection. A final GdiFlush is a recovery boundary.

This targets the no-window-surface/direct-drawable case in
X11DRV_PutImage and xrenderdrv_PutImage. The `--gdi-flush` control explicitly
flushes after every write. The optional Linux observer logs actual XPutImage
and XFlush calls without adding an X request. Its logging can affect timing;
compare uninstrumented runs as well. It observes explicit calls, not every
internal Xlib buffer drain. No vendor process, window or text is inspected.

## Why L3 only

Windows has GDI but has no Xlib request buffer or Wine X11 driver. Native
Windows execution cannot establish whether an X11 submission was flushed.
This is a Linux/X11 timing reproducer, not a Windows L1 conformance test.

## Build and prepare

On Arch, dependencies are official `mingw-w64-gcc`, `gcc` and `libx11`.
Build from the repository root; output goes outside the repository:

```bash
export OUT="$HOME/.cache/winum-putimage-$(date +%Y%m%d-%H%M%S)"
mkdir -p "$OUT"
x86_64-w64-mingw32-gcc -O2 -Wall -Wextra -Werror -municode -static -o "$OUT/putimage.exe" docs/wine-repro/winex11-flush-putimage/repro.c -lgdi32 -luser32
cc -O2 -Wall -Wextra -Werror -shared -fPIC -o "$OUT/observe.so" docs/wine-repro/winex11-flush-putimage/observe-x11.c -ldl -pthread
```

Select a matched pair of Wine 11.20 builds differing only in this patch.
Keep each run in its own new prefix; never replace an installed runtime's
DLLs. Substitute the runtime path below. Repeat the same recipe for the
other build with a new prefix and output directory/filenames.

```bash
export WINEBIN="/absolute/path/to/runtime/bin/wine"
export WINEPREFIX="$OUT/prefix-without"
export WINEDLLOVERRIDES='mscoree,mshtml='
export WINEDEBUG=-all
"$WINEBIN" reg add 'HKCU\Software\Wine\Drivers' /v Graphics /t REG_SZ /d x11 /f
"$WINEBIN" reg add 'HKCU\Software\Wine\AppDefaults\putimage.exe\X11 Driver' /v ClientSideWithRender /t REG_SZ /d N /f
"${WINEBIN%/wine}/wineserver" -k
"${WINEBIN%/wine}/wineserver" -w
"$WINEBIN" "$OUT/putimage.exe" > "$OUT/plain.txt" 2>&1
"$WINEBIN" "$OUT/putimage.exe" --gdi-flush > "$OUT/control.txt" 2>&1
```

Keep the small host uncovered; leave the pointer outside it during each
20-second-bounded run. Repeat the two fixture runs with
ClientSideWithRender=Y (same reg command, replace N by Y; stop/wait that
prefix's wineserver first). This exercises XRender-enabled and disabled
configurations; the configuration alone is not proof of which conversion
path handled a particular DIB.

For call evidence, with the same executable and prefix:

```bash
export LD_PRELOAD="$OUT/observe.so"
"$WINEBIN" "$OUT/putimage.exe" > "$OUT/observed.txt" 2>&1
unset LD_PRELOAD
grep -E '^(REPRO|DRAW|CONTROL|SAMPLE|RECOVERY|SUMMARY|INCONCLUSIVE|XPUT|XFLUSH|OBSERVER_ERROR)' "$OUT/observed.txt"
```

The library is for the x86_64 Linux Unix side, including pure WoW64 builds.
It is not a 32-bit Unix preload for classic dual-Unix-architecture Wine.
Use it only for the fixture, not Inventor, WebView2 or another application.

## Expected comparison and limits

Without the patch, small writer requests may stay buffered during the idle
interval: DRAW advances while SAMPLE retains the old color, then catches
up at a later request or the recovery flush. With the patch, the writer's
PutImage path explicitly flushes the same display after submission, and
SAMPLE should track red/blue within the polling/rendering latency. The
GdiFlush control should remove buffering delay in either build.

Match XPUT and following XFLUSH by process and display; the host reader's
flushes do not count as writer flushes. DRAW tick is GetTickCount64;
SAMPLE is relative to REPRO start_tick. The observer clock is a separate
Linux monotonic clock, so compare its own call ordering rather than
assuming identical tick origins. No hard millisecond deadline is asserted.

If no XPUT appears, the changed path was not demonstrated. If GetPixel
returns invalid, or all three runs stay at the background color, the
observer/path is inconclusive. If the unpatched build already flushes by
another path, that run did not reproduce delayed presentation; it is not
evidence of a patch failure. The DC observation does not establish final Wayland compositing. This does not cover WebView2's separate GPU repaint
problem, which was reported to persist independently.

2026-11-09: strict MinGW build passed for x86_64/i686. The Linux x86_64 observer also compiles with -Wall -Wextra -Werror
against the distribution Xlib headers. No
fixture execution in cloud, no paired timing result or new L4 is claimed.
Page marker records the reason for L3, not a completed runtime pass.

The final summary records total/valid polls; no usable polling, timer failure
or command timeout is inconclusive (exit 2), even if the writer finished.
GdiFlush control/recovery return values are logged. A second-model source
advisory found no recognizable copied code (not proof of originality);
its observer-completeness and pathname-bound suggestions were applied.
The advisory is not a paired runtime result.
