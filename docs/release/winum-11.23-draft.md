# Winum 11.23 release draft

The release-preparation branch still uses the pinned, verified Wine
11.22 source and increments the Winum build counter from 1 to 2. Wine
11.23 is expected around 2026-11-27. Update the source pin and checksum
after verifying the upstream release, then rerun the full validation
against that source before tagging.

## Staged for the series

- [Shell32 breadcrumb labels](../patches/shell32-parentrelativeforui.md).
- [Shell32 New Folder in filesystem background menus, v2](../patches/shell32-background-menu.md).
- [Shell32 inline name confirmation](../patches/shell32-label-edit-enter.md).
- [Shell32 responsive filename autocomplete, v2](../patches/shell32-autocomplete-responsive.md).
  This carries the maintainer's known-deviation decision. Provider calls
  stay on the edit STA and are batched, so a slow individual COM call
  can still block that thread.

The Wine tests and MEASURE sources accompany the series changes. Claude's
2026-11-22 cross reviews are recorded on the four patch pages.

## Pending validation

- [MSXML3 same-node replacement, Lukas10999's PR #4](https://github.com/Harvi654/winum/pull/4).
  L1 results for the Windows-informed test revision are on the PR. The
  contributor's PR is the integration path. Final test adoption, paired
  L2 on the release series and the Inventor preview and smoke checks
  remain to be recorded.
- [x64 UCRT C++ exception frame size](../../patches/candidates/ucrtbase-cxx-exception-size/README.md).
  Run the Wine regression tests on Windows 10 for x64 and i686, then
  request cross review of the measured version.
- [Cross-thread cancellation of windowless timers](../../patches/candidates/server-cross-thread-killtimer/README.md).
  The paired minimal probe passed on x64 and i686 and AutoCAD 2027
  passed a signed-in restart. Full paired server, user32 and win32u L2
  and Inventor 2027 and 2026 L4 are pending. Request cross review after
  those results.

## Diagnostics and history

The [SCM session-reporting experiment](../archive/server-services-session-report/README.md)
is archived with its patch, probe and contributor credit. It helped the
service start under SCM. The desktop alert persisted until fixing
cross-thread timer cancellation.

[AutoCAD diagnostics and private manual CI](https://github.com/Harvi654/Autodesk-AutoCAD-on-Linux/blob/c715932/tools/README.md) · [Inventor export trace and installer PR](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/pull/32)

Topic and test-run branches retain the original evidence and paths.
