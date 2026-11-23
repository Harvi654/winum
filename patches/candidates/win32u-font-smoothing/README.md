# win32u font smoothing

Credit @krokosik for the diagnosis and original MEASURE probe in Inventor
[#13](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/issues/13) and
[PR #18](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/pull/18).
See the [full comparison and native L1](../../../docs/wine-repro/win32u-font-smoothing/README.md).

With fontconfig rgba=none, Wine 11.22 let face flags mask explicit
ClearType quality and desktop ClearType. The opposite occurred with
rgba=rgb when desktop smoothing was off. Windows 10 offline measurement
showed explicit ClearType works with desktop smoothing off, and default
quality follows the desktop setting. Fontconfig still supplies fallback
smoothing when desktop smoothing is unconfigured, and subpixel order
when desktop orientation is unconfigured. The LCD filter remains in
FreeType. The candidate does not address other greyscale rasterization
differences, and exact RGB/BGR pixel mirroring is outside the test.

Apply the [test already verified on Windows](../../tests-first/win32u-tests-font-smoothing.patch)
and the runtime patch after the normal series with `--fuzz=0`.
The test's explicit `gdi32_test.exe font smoothing` mode covers all eight
settings and restores the originals. Its normal `font` unit uses current
settings.

| Level | Status |
|---|---|
| L0 | Full patched build queued on the isolated test branch, including x86_64 and i386 Windows modules. |
| L1 | 2026-11-23 offline Windows 10, x86_64 and i686 each 301 checks, no failures, todos or skips, both exit 0. |
| L2 | Paired gdi32, win32u and user32 plus targeted pixel tests with both fontconfig profiles on the test branch. |
| L4 Inventor 2027 | Pending 4K fractional scaling check of ribbon, menus and status bar. |
| L4 AutoCAD | Deferred if the maintainer skips the application check. |
| Cross review | Claude pending before promotion. |
