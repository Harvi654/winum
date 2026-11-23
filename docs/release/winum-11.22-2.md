# winum-11.22-2

Wine 11.22, `WINUM_BUILD=2`: the 66 patches of winum-11.22-1 plus seven
new ones, 73 in all. Wine itself and its source pin are unchanged
(`WINE_SHA256` as for 11.22-1).

## New in this build

Inventor 2027, Open and Save dialogs:

- [`shell32-parentrelativeforui`](../patches/shell32-parentrelativeforui.md) -
  folder names in the breadcrumb bar instead of empty buttons.
- [`shell32-background-menu`](../patches/shell32-background-menu.md) (v2) -
  New Folder in the background menu of file-system folders.
- [`shell32-label-edit-enter`](../patches/shell32-label-edit-enter.md) -
  Enter commits the new folder name instead of opening the folder.
- [`shell32-autocomplete-responsive`](../patches/shell32-autocomplete-responsive.md)
  (v2, known deviation, maintainer's decision recorded on the page) -
  typing a file name no longer freezes the dialog.

DWG TrueView and AutoCAD:

- [`msxml3-replacechild-same-node`](../patches/msxml3-replacechild-same-node.md) -
  `replaceChild` with the same node; AutoCAD's `Profile.aws` and Inventor's
  sketch preview through DWG TrueView no longer grow or fail. Contributed
  by @Lukas10999 (winum PR #4) - thank you.

AutoCAD 2027:

- [`ucrtbase-cxx-exception-size`](../patches/ucrtbase-cxx-exception-size.md) -
  Save As no longer ends AutoCAD with a fail fast in CoreCLR: x86-64 UCRT
  reports the 32-byte exception frame record Windows reports.
- [`server-cross-thread-killtimer`](../patches/server-cross-thread-killtimer.md) -
  no "Connection Lost" alert two minutes after start: a windowless timer
  can be cancelled from another thread of the process, as on Windows.

## Validation

| Level | Result | Date |
| --- | --- | --- |
| L0 | all 73 patches apply to the official 11.22 archive with `--fuzz=0`; built for both architectures as the combined test build [run 45064416572](https://github.com/Harvi654/winum/actions/runs/45064416572) (the same code; the ucrtbase test moved into its patch afterwards) and by "Build Wine" on the release branch | 2026-11-22/23 |
| L1 | every new patch's Wine test passed on offline Windows 10, x86_64 and i686 (per patch page) | 2026-11-18 to 22 |
| L2 | per patch, paired against the series without it: shell32 and msxml3 in their own runs, ucrtbase [44905557665](https://github.com/Harvi654/winum/actions/runs/44905557665), the timer with the neighbouring server and windowing suites [45049314260](https://github.com/Harvi654/winum/actions/runs/45049314260); no new failures | 2026-11-18 to 22 |
| L4 | Inventor 2027 on a copy of the maintainer's prefix with the release build of the first five ([run 45052921398](https://github.com/Harvi654/winum/actions/runs/45052921398)) and with the combined build of all seven ([run 45064416572](https://github.com/Harvi654/winum/actions/runs/45064416572)): the dialog fixes, start, part, assembly, drawing, save/reopen, restart (maintainer, 2026-11-22). AutoCAD 2027 with the combined build: passed (maintainer, 2026-11-23). SOLIDWORKS deferred (AGENTS.md) | 2026-11-22/23 |
| Cross review | every new patch read in full by a second model (per patch page) | 2026-11-18 to 22 |

`scripts/check-series.sh`: 73 entries with L1 (62), an L3-only reason (9)
or a known deviation (2), 0 without.

## Known issues

- SOLIDWORKS: the checkmark and cross buttons of the property manager do
  not react since 11.22-1; recorded as an open point on
  [`comctl32-ownerdraw-existing-capture`](../patches/comctl32-ownerdraw-existing-capture.md),
  not fixed in this build.

## Not in this build

Candidates that stay candidates: the AF_UNIX and iphlpapi work for
RobotStudio (the program does not start with them yet), `wldap32`,
the winex11 unmanaged-window candidate (PR #3), and the per-monitor work
area for Inventor issue #31.

## Open after the release

- Autodesk-Inventor-for-Linux pins winum-11.22-2 (`wine/version.env`,
  `wine/SHA256SUMS`, the `winum` flake input) in one commit, after this
  release exists; Autodesk-AutoCAD-on-Linux likewise.
- Wine 11.23 (expected around 2026-11-27): rebase, then full validation.
- Upstream: Wine merge request 10753 (`privatePath` probing for
  side-by-side assemblies) would let Autodesk's ODIS bootstrapper start;
  watch it for 11.23.
