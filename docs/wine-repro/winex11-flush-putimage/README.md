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

## Matched builds on Arch / KDE Wayland

Use a fresh Bash terminal in the normal KDE Wayland session. These commands
build two private runtimes from the same pinned commit and compiler/packages;
only this patch is omitted from the disposable `without-source` worktree.
The main checkout, installed Wine and application prefixes are untouched.
Allow roughly 25–30 GB and several hours for two full builds; keep the same
packages between builds. Nothing is installed from either Wine build.
The packages below follow the project's Arch build workflow; `-Syu` performs
a normal Arch system update. Skip installation if they are already present
and the system is up to date.

```bash
sudo pacman -Syu --needed base-devel git python mingw-w64-gcc \
  alsa-lib libpulse gnutls krb5 dbus fontconfig freetype2 \
  libx11 libxext libxi libxrandr libxrender libxcursor libxcomposite libxinerama libxxf86vm libxkbcommon \
  wayland mesa libglvnd vulkan-icd-loader vulkan-headers \
  gst-plugins-base-libs sdl2 libcups libgphoto2 sane libpcap v4l-utils pcsclite unixodbc ocl-icd opencl-headers giflib

set -euo pipefail
cd "$HOME/winum"
git pull --ff-only
export OUT="$HOME/.cache/winum-putimage-pair-$(date +%Y%m%d-%H%M%S)"
export TEST_COMMIT=b91285035c743528e1da599b85fe9246ba7a258a
export PATCH_NAME=winex11-flush-putimage.patch
mkdir -p "$OUT"
unset LD_PRELOAD WINUM_XI_FAULT WINE_X11_SHAPE_ALPHA WINE_OPENGL_REQUIRE_ALPHA
unset WINEPREFIX WINELOADER WINESERVER WINEARCH WINE_VER_OVERRIDE
printf 'SESSION type=%s xwayland_display=%s\n' "$XDG_SESSION_TYPE" "$DISPLAY"
[ "$XDG_SESSION_TYPE" = wayland ]
[ -n "$DISPLAY" ]
git worktree add --detach "$OUT/with-source" "$TEST_COMMIT"
git worktree add --detach "$OUT/without-source" "$TEST_COMMIT"
python3 - <<'PAIR'
import os
from pathlib import Path
out = Path(os.environ['OUT'])
name = os.environ['PATCH_NAME']
source = out / 'with-source/patches/series'
target = out / 'without-source/patches/series'
lines = source.read_text().splitlines(keepends=True)
assert sum(line.split('#', 1)[0].strip() == name for line in lines) == 1
# Omit exactly one entry only in the disposable comparison worktree.
target.write_text(''.join(line for line in lines if line.split('#', 1)[0].strip() != name))
print('PAIR omitted=' + name + ' other_entries_unchanged=1')
PAIR
git -C "$OUT/without-source" diff -- patches/series > "$OUT/without-series.diff"
source "$OUT/with-source/wine/version.env"
export RUNTIME_NAME="winum-$WINE_VERSION-$WINUM_BUILD-x86_64"
export WINE_SOURCE_TARBALL="$OUT/wine-$WINE_VERSION.tar.xz"
export JOBS=2
for variant in without with; do
  export WORK="$OUT/$variant-work"
  if ! bash "$OUT/$variant-source/scripts/build-wine.sh" "$OUT/$variant-dist" > "$OUT/$variant-build.txt" 2>&1; then
    tail -n 40 "$OUT/$variant-build.txt"
    exit 1
  fi
  (cd "$OUT/$variant-dist" && sha256sum -c "$RUNTIME_NAME.tar.xz.sha256")
  "$OUT/$variant-work/$RUNTIME_NAME/bin/wine" --version
  printf 'BUILD variant=%s commit=%s comparison_patch=%s\n' "$variant" "$TEST_COMMIT" "$PATCH_NAME"
done
unset WORK JOBS WINE_SOURCE_TARBALL
```

Both builds use the official source archive, verified against the pinned
SHA-256 by `build-wine.sh`, and apply their lists with `--fuzz=0`.
`without-series.diff`, the two BUILDINFO/PATCHES.txt files and build output
retain the comparison provenance locally. A failed build stops this recipe;
do not substitute an unrelated system Wine as the unpatched control.
The BUILD record identifies the comparison patch; `variant=without`
omits it and `variant=with` keeps it.

## Build the fixture and run the pair

Continue in the same terminal, with OUT and RUNTIME_NAME still set:

```bash
x86_64-w64-mingw32-gcc -O2 -Wall -Wextra -Werror -municode -static -o "$OUT/putimage.exe" "$OUT/with-source/docs/wine-repro/winex11-flush-putimage/repro.c" -lgdi32 -luser32
cc -O2 -Wall -Wextra -Werror -shared -fPIC -o "$OUT/observe.so" "$OUT/with-source/docs/wine-repro/winex11-flush-putimage/observe-x11.c" -ldl -pthread
export WINEDLLOVERRIDES='mscoree,mshtml='
export WINEDEBUG=-all
fixture_run() {
  export FIXTURE_LOG="$OUT/$variant-render-$render-$1.txt"
  shift
  export FIXTURE_RC=0
  "$WINEBIN" "$OUT/putimage.exe" "$@" > "$FIXTURE_LOG" 2>&1 || export FIXTURE_RC="$?"
  printf 'EXIT code=%s\n' "$FIXTURE_RC" >> "$FIXTURE_LOG"
}
for variant in without with; do
  export WINEBIN="$OUT/$variant-work/$RUNTIME_NAME/bin/wine"
  export WINEPREFIX="$OUT/prefix-$variant"
  "$WINEBIN" reg add 'HKCU\Software\Wine\Drivers' /v Graphics /t REG_SZ /d x11 /f > "$OUT/$variant-prefix.txt" 2>&1
  for render in N Y; do
    "$WINEBIN" reg add 'HKCU\Software\Wine\AppDefaults\putimage.exe\X11 Driver' /v ClientSideWithRender /t REG_SZ /d "$render" /f >> "$OUT/$variant-prefix.txt" 2>&1
    "${WINEBIN%/wine}/wineserver" -k
    "${WINEBIN%/wine}/wineserver" -w
    fixture_run plain
    fixture_run control --gdi-flush
    export LD_PRELOAD="$OUT/observe.so"
    fixture_run observed
    unset LD_PRELOAD
    "${WINEBIN%/wine}/wineserver" -k
    "${WINEBIN%/wine}/wineserver" -w
    grep -HE '^(REPRO|DRAW|CONTROL|SAMPLE|RECOVERY|SUMMARY|INCONCLUSIVE|XPUT|XFLUSH|OBSERVER_ERROR|EXIT)' "$OUT/$variant-render-$render-plain.txt" "$OUT/$variant-render-$render-control.txt" "$OUT/$variant-render-$render-observed.txt" || true
  done
done
unset -f fixture_run
unset LD_PRELOAD WINEPREFIX WINEBIN WINEDLLOVERRIDES WINEDEBUG FIXTURE_LOG FIXTURE_RC
```

Keep the small host uncovered and the pointer outside it. There are six
runs per prefix: plain, explicit-flush control and observed, each with
ClientSideWithRender=N/Y. The loop first completes all unpatched runs,
then the same patched runs, and stops/waits the matching wineserver before
switching runtimes. It prints only fixture/observer records and exit codes.
Use the filenames to distinguish the pair; return that filtered output.

The observer targets the x86_64 Linux Unix side, including pure WoW64.
Use it only for this fixture. ClientSideWithRender selects a configuration;
XPUT evidence is still required to identify the exercised submission path.

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

## After the test

Only disposable prefixes and private runtimes under OUT were used. No
Inventor/SOLIDWORKS DLLs or registry settings need restoring. The commands
unset the diagnostic variables and stop the matching prefix's wineserver.
If interrupted, close the fixture first, unset LD_PRELOAD and
WINUM_XI_FAULT/WINE_X11_SHAPE_ALPHA, and stop that same prefix's wineserver
before starting another Wine program. Keep OUT and its result files until
the comparison is recorded; do not commit raw Wine debug output.

2026-11-09 procedure update: shell syntax and all three omitted-patch
source lists were checked without using the maintainer's desktop session.
The paired runtime results above remain pending.
