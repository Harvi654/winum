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
| Vanilla Wine 11.22 | Pending CI | Pending CI |
| Winum 11.22-2 | Pending CI | Pending CI |
| Offline Windows 10 | Pending native reference (fontconfig does not apply) | Same reference |

After native comparison, derive a Wine test only from stable measured cases.
L0, offline L1, paired gdi32/win32u/user32 L2, Inventor 2027 L4 and Claude
cross review apply to any resulting candidate. Record AutoCAD L4 as
deferred if it is skipped.
