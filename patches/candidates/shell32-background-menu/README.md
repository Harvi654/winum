# Restore New in shell folder background menus

The first patch adds the Wine test with `todo_wine` for the two command
ranges where Wine drops the submenu. The second patch fixes the menu and
removes those markers. See the [trace and Windows measurements](
../../../docs/wine-repro/shell32-background-menu/README.md) and the
[patch page](../../../docs/patches/shell32-background-menu.md).

On `test-run/shell32-background-menu-1`, Build Wine produces binary and
source artifacts. Validate shell32 background menu runs paired shell32
tests and supplies a Windows L1 bundle with `run-tests.bat`. Test the
candidate in Inventor by creating a temporary folder via the Open
dialog, then verify it appears in the list and can be opened.
