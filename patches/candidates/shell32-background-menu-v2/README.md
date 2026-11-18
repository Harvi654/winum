# Restore New in filesystem folder background menus

Apply the two patches in `series` order. Revision 2 requires both a
filesystem path and the creation helper. Recycle Bin exposes that
helper but its folder-creation methods are stubs. The test also skips
the English caption check in other locales rather than treating any
submenu as New.

The Wine-specific Recycle Bin case checks that the shared background
menu does not gain the New submenu at the normal shell-view command
range. It does not make an assertion about the Windows Recycle Bin.
The measured Windows test uses a filesystem folder.

See [the patch page](../../../docs/patches/shell32-background-menu.md).
The v1 test-run branch remains as the record of the original candidate.
