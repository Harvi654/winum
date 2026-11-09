# L3: an owned shaped dialog under Xwayland

Original, AI-assisted (Codex), LGPL-2.1-or-later. `repro.c` uses only own
Win32 windows. Its default target is a hidden, owned, uncaptioned popup of
dialog class #32770. It sets an L-shaped region before showing it, matching
the patch's ARGB-visual eligibility checks. A green owner lies behind a red
160x160 popup. Every five seconds the region changes: missing bottom-right,
missing bottom-left, full rectangle (SetWindowRgn NULL), missing bottom-right
again. This exercises region replacement, clearing and restoration as well
as the initial hidden-window visual change.

Controls: `--ordinary` uses an own non-dialog class, which the patch must
not promote to ARGB. `--late-region` sets the first region only after
showing the window, excluded by the default hidden-only guard. There are
no layered, GL or Vulkan children here; this fixture does not validate
those excluded rendering paths. No CAD components are loaded.

## Why L3 only

SetWindowRgn exists on Windows, but Xwayland's bounding/input shapes,
compositor alpha and Wine's visual selection do not. A native Windows
conformance test cannot exercise the changed X11/compositor path. This
fixture is an L3 visual/input-region reproducer, not Windows L1 evidence.

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
export OUT="$HOME/.cache/winum-shape-pair-$(date +%Y%m%d-%H%M%S)"
export TEST_COMMIT=b91285035c743528e1da599b85fe9246ba7a258a
export PATCH_NAME=winex11-shape-alpha-xwayland.patch
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

Continue in the same terminal. Leave both own windows uncovered. During
each L-shaped phase, click once on red and once in the missing quadrant;
at the full-rectangle phase click the square again. Repeat for every run.
The sequence lasts about 20 seconds; an early close is inconclusive.

```bash
x86_64-w64-mingw32-gcc -O2 -Wall -Wextra -Werror -municode -static -o "$OUT/shape.exe" "$OUT/with-source/docs/wine-repro/winex11-shape-alpha-xwayland/repro.c" -lgdi32 -luser32
export WINEDLLOVERRIDES='mscoree,mshtml='
export WINEDEBUG=-all,+x11drv
fixture_run() {
  export FIXTURE_LOG="$OUT/$variant-shape-$1.txt"
  shift
  export FIXTURE_RC=0
  "$WINEBIN" "$OUT/shape.exe" "$@" > "$FIXTURE_LOG" 2>&1 || export FIXTURE_RC="$?"
  printf 'EXIT code=%s\n' "$FIXTURE_RC" >> "$FIXTURE_LOG"
}
for variant in without with; do
  export WINEBIN="$OUT/$variant-work/$RUNTIME_NAME/bin/wine"
  export WINEPREFIX="$OUT/prefix-$variant"
  "$WINEBIN" reg add 'HKCU\Software\Wine\Drivers' /v Graphics /t REG_SZ /d x11 /f > "$OUT/$variant-prefix.txt" 2>&1
  "$WINEBIN" reg add 'HKCU\Software\Wine\AppDefaults\shape.exe\X11 Driver' /v ClientSideGraphics /t REG_SZ /d Y /f >> "$OUT/$variant-prefix.txt" 2>&1
  "${WINEBIN%/wine}/wineserver" -k
  "${WINEBIN%/wine}/wineserver" -w
  unset WINE_X11_SHAPE_ALPHA
  fixture_run default
  fixture_run ordinary --ordinary
  fixture_run visible --late-region
  export WINE_X11_SHAPE_ALPHA=0
  fixture_run disabled
  export WINE_X11_SHAPE_ALPHA=2
  fixture_run visible-optin --late-region
  unset WINE_X11_SHAPE_ALPHA
  "${WINEBIN%/wine}/wineserver" -k
  "${WINEBIN%/wine}/wineserver" -w
  grep -HE '^(REPRO|SHAPE|CLICK|SUMMARY|INCONCLUSIVE|ERROR|EXIT)|shaped window under Xwayland|using an ARGB visual' "$OUT/$variant"-shape-*.txt || true
done
unset -f fixture_run
unset WINE_X11_SHAPE_ALPHA WINEPREFIX WINEBIN WINEDLLOVERRIDES WINEDEBUG FIXTURE_LOG FIXTURE_RC
```

The loop gives both builds the same default and control runs, each with
its own prefix. Env value 0 disables this patch; value 2 admits the
visible-first dialog only in the patched build. Confirm `CLICK target=owner`
for the missing quadrant and `target=popup` for red. At phase 2 the square
must be wholly red/clickable; at phase 3 the missing corner must return.
Compare the desktop itself, not just a GDI capture. Return the filtered
records with an observation per run (transparent/opaque and click target).
Exit 0 only establishes fixture completion.

## Expected comparison and limits

On an affected Xwayland/compositor combination, the unpatched (or disabled)
target retains bounding-shape-only behavior: the missing quadrant can show
as an opaque rectangle. With the patch and its default hidden-first target,
the trace should confirm ARGB selection; outside-region pixels should be
transparent and excluded from input. Clearing the region must reset input
clipping and repaint opaque pixels, then reapplying must clip again.

The ordinary-class and default visible-first controls should receive no
ARGB-promotion trace from this patch. With opt-in value 2 the visible-first
dialog is eligible. Under a compositor that already handles X bounding
shape correctly, both builds may look correct: that is an unreproduced
visual symptom, not proof of the alpha path. On a real Xorg server the
Xwayland-specific path should stay inactive. Trace absence also requires
checking XWAYLAND-extension and ARGB-visual availability before concluding
that an eligible window failed.

2026-11-09: compiled both PE architectures with -Wall -Wextra -Werror.
No cloud Wine execution, paired compositor result or new consumer L4 is
claimed. The L3-only marker records the reason and available fixture;
runtime verification remains pending.

The fixture checks class/owner/style/160x160 bounds and visibility before
treating its target as valid; a 40-second watchdog bounds a stalled GUI.
Early close or failed setup is inconclusive (exit 2). Exit 0 means the
sequence completed, not that pixels or compositor hit testing passed.
A second-model source advisory found no recognizable copied code in this
bounded comparison, which cannot prove originality. Its eligibility and
timeout suggestions were applied before the final strict builds.

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
