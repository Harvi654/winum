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

## Build and run

From the repository root, with official MinGW-w64:

```bash
export OUT="$HOME/.cache/winum-shape-$(date +%Y%m%d-%H%M%S)"
mkdir -p "$OUT"
x86_64-w64-mingw32-gcc -O2 -Wall -Wextra -Werror -municode -static -o "$OUT/shape.exe" docs/wine-repro/winex11-shape-alpha-xwayland/repro.c -lgdi32 -luser32
export WINEBIN="/absolute/path/to/runtime/bin/wine"
export WINEPREFIX="$OUT/prefix-without"
export WINEDLLOVERRIDES='mscoree,mshtml='
export WINEDEBUG=-all,+x11drv
"$WINEBIN" reg add 'HKCU\Software\Wine\Drivers' /v Graphics /t REG_SZ /d x11 /f
"$WINEBIN" reg add 'HKCU\Software\Wine\AppDefaults\shape.exe\X11 Driver' /v ClientSideGraphics /t REG_SZ /d Y /f
"${WINEBIN%/wine}/wineserver" -k
"${WINEBIN%/wine}/wineserver" -w
unset WINE_X11_SHAPE_ALPHA
"$WINEBIN" "$OUT/shape.exe" > "$OUT/shape-default.txt" 2>&1
"$WINEBIN" "$OUT/shape.exe" --ordinary > "$OUT/shape-ordinary.txt" 2>&1
"$WINEBIN" "$OUT/shape.exe" --late-region > "$OUT/shape-visible.txt" 2>&1
grep -E '^(REPRO|SHAPE|CLICK|SUMMARY)|shaped window under Xwayland|using an ARGB visual' "$OUT"/shape-*.txt
```

Run on a Wayland session through Xwayland (the registry explicitly selects
Wine's x11 driver); a native Wayland Wine driver bypasses the patch. Use
two Wine 11.20 builds differing only in this patch, with a fresh prefix
per build. Change the runtime/prefix and result filenames for the second
build. Keep the owner/popup uncovered and inspect the desktop composition,
not just a GDI capture. Screenshots may contain only these own windows.

During each L phase, the missing quadrant should reveal green rather than
an opaque black/red rectangle. An actual mouse click there should log
`CLICK target=owner`; clicking red should log `target=popup`. Messages sent
directly with SendMessage would bypass input hit testing and are not a
substitute. At phase 2 the whole square should be red and accept popup
clicks; at phase 3 the missing corner should be green/click through again.

Optional diagnostic controls for the patched build, each in a fresh Wine
process after the preceding fixture exits:

```bash
export WINE_X11_SHAPE_ALPHA=0
"$WINEBIN" "$OUT/shape.exe" > "$OUT/shape-disabled.txt" 2>&1
export WINE_X11_SHAPE_ALPHA=2
"$WINEBIN" "$OUT/shape.exe" --late-region > "$OUT/shape-visible-optin.txt" 2>&1
unset WINE_X11_SHAPE_ALPHA
```

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
