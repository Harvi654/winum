# winum-11.23-1

Release candidate for Wine 11.23 with 84 winum patches.
See the [signed source verification and upstream audit](wine-11.23-rebase-audit.md).

## New patches

- [RichEdit default tab stop](../patches/riched20-default-tab-stop.md) implements the TOM setter and getter used by Revit.
- [TOM paragraph navigation](../patches/riched20-textrange-expand-paragraph.md) handles six paragraph operations used by Revit's text engine. Revit 2027.4 opened New Project with both changes in the combined 11.22 smoke test.
- [RichEdit target-device scrolling](../patches/riched20-target-device-scroll.md) exposes the full horizontal range of Inventor's text box.
- [OLE RevokeDragDrop](../patches/ole32-revokedragdrop-cross-process.md) follows the Windows behavior when a window belongs to another process, allowing Revit's WebView2 Home page to start.

The Content Center #45 removeAll candidate remains pending the AutoCAD and
Revit application checks.

## Rebase and validation

All 84 patches apply to the official Wine 11.23 source with `--fuzz=0`.
Five patches needed mechanical updates. The server protocol files were
regenerated with the upstream tool in the complete source tree, taking
protocol 988 to 989. The request trace status-name table is preserved.
`scripts/check-series.sh` passes. Both-architecture L0, paired vanilla
versus series L2, and the 11.23 maintainer smoke tests are pending.

Wine 11.23 [changes MSI component directory selection during maintenance](https://github.com/wine-mirror/wine/commit/34c80fd64e16d29b8564fed0c1c52168e68c9d7a).
Installed components reuse their registered locations, and target root
resolution no longer uses InstallLocation as TARGETDIR.

Maintainer L4 on Wine 11.23:

1. In a fresh prefix install Inventor 2027 from the installer image, apply
   Update 2027.2, then install the Nastran 2027 companion. Verify installed
   components remain in place, add-ins load, and Nastran completes mesh,
   solve and results.
2. Exercise Inventor, AutoCAD and Revit on the supported desktop/Wayland
   paths. Include Inventor text editing, AutoCAD MTEXT and TEXT, and Revit
   Home and New Project in software graphics mode.
