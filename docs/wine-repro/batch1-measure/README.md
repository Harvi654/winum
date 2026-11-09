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
