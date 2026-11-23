# MEASURE: GDI font smoothing and fontconfig

Credit @krokosik for the diagnosis and the DIB probe in
[Inventor issue #13](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/issues/13)
and [PR #18](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/pull/18).
The contribution links the small Tahoma rendering to the font's gasp table
and to Wine's priority for fontconfig's per-face antialiasing flags. The PR
uses a Nix launcher override. Its networked Windows measurement is
background for this probe. Native expectations come from the maintainer's
offline Windows 10 VM. The
[shared installer plan](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/drafts/shared-installer-plan.md#fonts-2026-11-23)
puts Wine behavior ahead of any settings migration.

The probe draws white text over black in a 32-bit DIB section and reads the
pixels via GetDIBits after deselecting the bitmap. Any unequal RGB channels
classify the case as subpixel; intermediate equal channels give greyscale;
black and white only give bi-level. Naked pixel counts, the number of
intermediate greys, actual face and realized ppem accompany the class.
The two image hashes read channels in RGB and BGR order respectively, so a
channel swap can be checked within a platform and font. An empty image or
a substituted explicit face is unsuitable as a font reference.

It draws with a privately loaded renamed Wine 11.22 Tahoma (gasp and
embedded bitmaps preserved), the locally available Tahoma, and a renamed
Liberation Sans 2.1.5 control with no gasp table. Each face has a row for
every requested height from 8 through 24 pixels and DEFAULT, ANTIALIASED,
CLEARTYPE, CLEARTYPE_NATURAL and NONANTIALIASED quality. For the UI path
the probe requests DEFAULT quality from copies of the message and menu
LOGFONTs returned by SPI_GETNONCLIENTMETRICS and from the LOGFONT returned
for DEFAULT_GUI_FONT. Those three retain their system-selected heights.

The controller reads the initial system settings, then crosses smoothing
off/on, standard/ClearType and BGR/RGB. It writes each via SPI, verifies the
readback, and starts a new rendering process for each tuple. Wine 11.22
initializes cached font settings when the process starts. The controller
restores and re-reads the original settings even following a run failure.
Use a disposable offline VM snapshot for recovery if the controller is
force-killed. A complete run has eight matrices of 258 cases and the
completion line `measure_complete matrices=8 restored=1` per architecture.

## Build and run

From the Winum checkout with MinGW for both architectures and Python FontTools:

```bash
bash scripts/build-measure-bundle.sh /tmp/font-smoothing-measure \
  docs/wine-repro/win32u-font-smoothing/measure.list
bash docs/wine-repro/win32u-font-smoothing/prepare-fonts.sh /tmp/font-smoothing-measure
(cd /tmp/font-smoothing-measure && sha256sum -c SHA256SUMS)
```

Copy the verified folder to the offline Windows 10 VM and run
`run-measure.bat`. Use an interactive local desktop. Return only both
`result-font-smoothing-*.txt` files and `exitcodes.txt`. The kit includes
the two renamed free font files. The test reads Windows' own Tahoma and
system fonts from within the VM.

CI runs the same probe under Xvfb with separately initialized Wine
prefixes for fontconfig `rgba=none` and `rgba=rgb`. The two configs force
the requested order and check the resolved order using fc-match.
A native result is compared to each Wine setting by face request, height,
quality, smoothing type and orientation. The actual UI font and requested
system height are reported so differences in UI font selection are visible.

## Results and gates

| Runtime | none | rgb |
|---|---|---|
| [Vanilla Wine 11.22](results/vanilla-11.22/) | 2,064 cases per architecture | 2,064 cases per architecture |
| [Winum 11.22-2](results/winum-11.22-2/) | 2,064 cases per architecture | 2,064 cases per architecture |
| [Offline Windows 10](results/windows-10/) | 2,064 cases per architecture (one native reference) | Same reference |

The Winum run completed all eight settings and restored the original values
on both architectures. At 12 px with smoothing on and type 2, the renamed
Tahoma produces bi-level output for DEFAULT, CLEARTYPE and CLEARTYPE_NATURAL
with rgba=none, and subpixel output for those three with rgba=rgb. The
message, menu and stock UI samples in this run also change from bi-level
to subpixel. [Winum CI](https://github.com/Harvi654/winum/actions/runs/45198628536/job/137777473702)

All case rows match exactly between vanilla Wine 11.22 and Winum 11.22-2
for each profile and architecture. The counts below are per architecture
and per build. Each run verified and restored its smoothing settings.
[Vanilla Wine CI](https://github.com/Harvi654/winum/actions/runs/45198628536/job/137777473542)

| Fontconfig profile | Bi-level | Greyscale | Subpixel |
|---|---:|---:|---:|
| rgba=none | 1,008 | 1,056 | 0 |
| rgba=rgb | 552 | 264 | 1,248 |

At 12 px, DEFAULT quality for the renamed Tahoma is bi-level for every
SPI tuple with rgba=none, and subpixel for every tuple with rgba=rgb.
Disabling smoothing through SPI therefore did not disable that observed
subpixel output with rgba=rgb.

## Offline Windows comparison

Both architectures completed and returned 0. The native modes agree across
architectures in all 2,064 cases (886 bi-level, 254 greyscale, 924 subpixel on
each). The actual family for the native UI menu and message probes is Segoe
UI, while Wine selects Tahoma. The tables join UI roles by role and desktop
settings and list the different families and heights.

| Profile (both Wine builds) | Cases whose mode differs from native, per architecture |
|---|---:|
| rgba=none | 1,160 |
| rgba=rgb | 404 |

[Every case on x86_64](results/comparison-x86_64.tsv) · [Every case on i686](results/comparison-i686.tsv)

| Selected cases | Offline Windows 10 | Wine rgba=none | Wine rgba=rgb |
|---|---|---|---|
| Tahoma 12 px, explicit CLEARTYPE or CLEARTYPE_NATURAL | Subpixel in all eight settings | Bi-level in all eight settings | Subpixel in all eight settings |
| Tahoma 12 px, DEFAULT, smoothing off or type 1 | Bi-level | Bi-level | Subpixel |
| Tahoma 12 px, DEFAULT, smoothing on and type 2 | Subpixel | Bi-level | Subpixel |
| Message, menu and stock UI DEFAULT, smoothing on and type 2 | Subpixel | Bi-level | Subpixel |

For each of the 462 native subpixel RGB/BGR pairs on x86_64, changing
the orientation changed the pixel hash. Only 95 are exact red/blue
channel swaps under this probe. All 624 subpixel pairs in Wine's fixed
rgba=rgb profile have the same hash before and after the SPI change.
These observations do not justify an exact channel-swap assertion.

## Regression test first

[The gdi32 font test](../../../patches/tests-first/win32u-tests-font-smoothing.patch)
checks colored-edge presence at 12 px with system Tahoma. The ordinary font
unit uses current desktop settings. Explicit `font smoothing` runs all eight
settings in separate children and restores the original settings. The L1
artifact runs this mode on both architectures in the offline VM. [Offline L1 results](results/l1-windows-10/) show 301 checks on each
architecture (eight children with 34 checks apiece plus 29 controller
checks), no failures, todos or skips, and exit 0. The
[win32u runtime candidate](../../../patches/candidates/win32u-font-smoothing/README.md)
is staged separately. Required candidate gates are L0, offline L1, paired
gdi32/win32u/user32 L2, Inventor 2027 L4 and Claude cross review. Record
AutoCAD L4 as deferred if it is skipped.
