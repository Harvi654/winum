# Tests for every patch before the first release (plan, 2026-11-08)

Goal: before `winum-11.20-1`, every patch in `patches/series` either has a
Wine test that passed on Windows 10 (L1) or a written reason why it cannot
have one, with a reproducer instead (L3). After that, no patch enters
`series` without one of the two (a CI check enforces it, below). Source of
the counts: `docs/patches/README.md` (2026-11-06) and the L2 runs of
2026-11-08 (`docs/wine-repro/l2-series-2026-11-08.md`).

## Where the 62 patches stand

| State | Count | Patches |
| --- | --- | --- |
| Test passed on Windows 10 (L1) | 25 | msxml3-schema-cache-namespace, glu32-nurbs-curves, win32u-fixed-negative-mat2, win32u-child-topmost-ignore, comctl32-themed-checkbox-buffered-paint, winex11-stale-map-activation, the 17 MSI feature patches (44-60), oleaut32-typelib-keep-released, opengl32-window-format-color-bits |
| Test exists, never ran on Windows | 8 | kernelbase-account-domain-sid, msxml3-domdoc-importnode, ntdll-locate-xstate-feature-enabled, ole32-storage-thread-lock, ole32-transacted-truncate-dirty, shell32-getimage, msxml3-parseerror-filepos, shell32-automation-zip-copy |
| Test written, passed on Windows, not yet in `series` | 1 | comctl32-statusbar-erase-background (v2 candidate: L1 and L2 passed) |
| No test, testable on Windows | 23 | batches 1-4 below |
| No test, no Windows counterpart (L3 instead) | 5 | winex11-flush-putimage, winex11-shape-alpha-xwayland, winex11-xinput-bad-device, winex11-owner-managed-async, explorer-tasklist-parenting-hosts |

## Batches (each: MEASURE on Windows where values are unknown, then the test, then the fix checked against it)

Batch 0 - no new code: run the L1 bundle of the 8 existing tests (built by
the series job `test (series, g1)`, artifact `tests-series-g1/l1-bundle/`)
in the Windows VM; fold the status bar v2 into its patch.

Batch 1 - comctl32, riched20, urlmon (Inventor):
- `comctl32-toolbar-setrows-wrapable` - **first**: it breaks Wine's own
  `TB_SETROWS` test (L2 regression); measure rows with separators on Windows,
  then fix or limit the patch;
- `comctl32-toolbar-padding-height`;
- `comctl32-header-null-layout`;
- `comctl32-ownerdraw-existing-capture`;
- `riched20-caret-rewrap`, `riched20-scf-word-insertion-point`,
  `riched20-textfont-applytmp`;
- `urlmon-maptozone-null-terminate`, `urlmon-res-zone-local-machine`.

Batch 2 - COM and runtime:
- `oleaut32-typedesc-pointee`, `oleaut32-typedesc-z-offsets` (a type library
  built in the test with `ICreateTypeLib2`);
- `ntdll-actctx-threadingmodel-case` (manifest in the test, as in
  `kernel32/tests/actctx.c`);
- `combase-stub-activation-context`;
- `rpcrt4-ndrstubcall3` (may need a hand-written stub; if a real test is not
  possible, L3 with a reason);
- `mshtml-document-open-optional-result`.

Batch 3 - installers, system information, network, shell:
- `msi-transform-null-stream`, `msi-selection-tree-directory-property`;
- `mspatcha-pa19-32bit-and-large-files` (test patch files must be created
  without Microsoft tools inside the repository; if that is not possible,
  L3 with a reason);
- `wbemprox-timezone` (compare with `GetTimeZoneInformation`, so the VM's
  time zone does not matter);
- `wininet-http-same-position-seek` (the wininet test server);
- `shell32-folderview-selection`.

Batch 4 - wineserver changes that show in user32 behaviour:
- `server-mousemove-on-show-hide` (`WM_MOUSEMOVE` after show/hide, measured);
- `server-input-attachment-components` (`AttachThreadInput` chains, its L3
  sequence test as the starting point).

L3 only (no Windows counterpart): the four winex11 patches and the Explorer
patch get a reproducer in `docs/wine-repro/` where they do not have one yet
(owner-managed-async and the Explorer patch have one), and their pages say
why there is no Wine test.

## Rules for the work

- Test first, measured on Windows; a test that fails on Windows is wrong.
- The assistant who did not write a test reviews it (Claude/Codex
  alternate per batch).
- Every batch ends with an L1 bundle run in the VM and a paired L2 run
  (`test-series`), and `docs/patches/` updated.
- If a test shows a patch differs from Windows (as the toolbar one does),
  the patch is fixed or limited before the release, not the test.

## Closing it: a check in CI

`scripts/check-series.sh` (to be added with batch 0): for every entry of
`patches/series` the patch page must record either `L1 | yes` or
`L3 only:` with a reason; otherwise the check fails. It runs in the Wine
build workflow, so a release tag cannot be built from a series that breaks
the rule, and AGENTS.md states it.
