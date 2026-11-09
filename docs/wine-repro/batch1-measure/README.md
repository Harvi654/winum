# Batch 1 MEASURE (comctl32, riched20)

Original, AI-assisted (Claude), LGPL-2.1-or-later. One MEASURE program per
batch 1 patch of `docs/patches/test-plan-2026-11.md`, written before any
Wine test, so that the tests take their expected values from Windows:

| Program | Patch | Folder |
| --- | --- | --- |
| toolbar-padding | `comctl32-toolbar-padding-height` | `../comctl32-toolbar-padding-height/` |
| header-null-layout | `comctl32-header-null-layout` | `../comctl32-header-null-layout/` |
| ownerdraw-capture | `comctl32-ownerdraw-existing-capture` | `../comctl32-ownerdraw-existing-capture/` |
| caret-rewrap | `riched20-caret-rewrap` | `../riched20-caret-rewrap/` |
| scf-word | `riched20-scf-word-insertion-point` | `../riched20-scf-word-insertion-point/` |
| textfont-applytmp | `riched20-textfont-applytmp` | `../riched20-textfont-applytmp/` |

`comctl-support.h` (shared, builds on `../batch3-measure/measure-support.h`):
loads comctl32 version 5 or, through an activation context made from an own
temporary manifest, version 6 (the bundle's own manifest asks for neither),
and runs a case in a child process of the same program, so that a crash on
Windows is recorded (`CHILD_END ... exit=c0000005`, `EXCEPTION code=...`)
instead of ending the run.

The entries are in `measure.list` here and in the main
`../measure-bundle.list`. A bundle of only these six:

```bash
bash scripts/build-measure-bundle.sh "$BATCH1_MEASURE" docs/wine-repro/batch1-measure/measure.list
```

(`$BATCH1_MEASURE` is a new, empty folder.) Run `run-measure.bat` in the
Windows 10 VM without touching mouse or keyboard (the ownerdraw and caret
programs use focus and mouse capture of their own windows; no real input is
generated), and return `result-*.txt` and `exitcodes.txt`. The outputs hold
no user, machine or path names; window handles are printed only as roles
(`button`, `other`, ...), except the `hwndInsertAfter` pointer of a
`WINDOWPOS` in header-null-layout, which is expected to be NULL.

Before the VM run, every program was built with `-Wall -Wextra -Werror` for
x86_64 and i686 (Ubuntu MinGW 13) and run once under the distribution's
Wine 9.0 with Xvfb, only to see that it completes (`SUMMARY ... complete=1`,
exit 0). Those outputs are not expectations and are not committed; where
they show the bug the patch fixes, the topic README says so.

## Run of 2026-11-09

All twelve programs completed in the offline Windows 10 VM (`SUMMARY ...
complete=1 api_errors=0`, which `b3_finish` turns into exit 0); x86_64 and
i686 identical apart from `bits=` and pointer width; comctl32 5.82 and
6.16. `exitcodes-2026-11-09.txt` holds `!errorlevel!` instead of the
codes: the batch file that ran used delayed-expansion syntax without
enabling it (the bundle script writes `%ERRORLEVEL%`), so the exit codes
come from the SUMMARY lines. Results and observations are in each topic's
`windows-2026-11-09/` and README:

| Patch | Result | Candidate |
| --- | --- | --- |
| toolbar-padding-height | Windows uses the padding height when it recalculates; recalculates less often than Wine | v2: test, code as v1 |
| header-null-layout | NULL layout and NULL members: FALSE, nothing written | v2: checks the members too |
| ownerdraw-existing-capture | Windows sends the `WM_CAPTURECHANGED` v1 suppresses; it takes the capture before the focus | v2: Windows order instead of the skip (SOLIDWORKS L4 needed) |
| caret-rewrap | no failure; caret at `EM_POSFROMCHAR` | v2: test, code as v1 |
| scf-word-insertion-point | Windows' word rule differs from v1 outside Inventor's case | none yet (open) |
| textfont-applytmp | msftedit: temporary and dropped; riched20: E_INVALIDARG | v2: tomApplyLater keeps it temporary; test in msftedit |
