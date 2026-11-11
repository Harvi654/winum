# shell32-folderview-selection.patch

<!-- winum-test: l1 -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `shell32`
- **Origin:** Autodesk-Inventor-for-Linux, commit `f9ea1fd`, first shipped in `wine-11.19-1`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Open/Place: clicking a file did not fill in the file name, double-click did nothing.

## Cause and fix

`IFolderView2::GetSelection`/`GetSelectedItem`/`GetSelectionState` implemented; `Items`/`ItemCount` honour `SVGIO_SELECTION`.

## Wine test

None in the production patch. The separate
[v2 candidate](../../patches/candidates/shell32-folderview-selection-v2/README.md)
adds a Windows-derived test in dlls/shell32/tests/selection.c and corrects
the empty selected-array result from E_FAIL to ERROR_NOT_FOUND. Its
exact-body native test passes all four measured functional states on both
architectures. The candidate is not yet in patches/series.

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.19-1` (2026-10-19, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06); winum-11.20-1 series (promoted 2026-11-11): every entry applies with `--fuzz=0` on the official 11.20 archive; the tree is identical to that of `test-run/all-6` (run 42997560598) |
| L1 | 357/0, both arch (promoted file: `shell32-folderview-selection-v2`) |
| L2 | not recorded per patch; winum-11.20-1: ok (`shell32:selection` 357/0) |
| L3 | not recorded |
| L4 | Click and double-click work. (Inventor, maintainer); winum-11.20-1: click/double-click (v1) |
| L5 | in Inventor's runtime since `wine-11.19-1` (2026-10-19); a week of use not formally recorded - maintainer to confirm |
| Cross review | Claude, approve |

## Open points

Candidate native L1 is passed, but observed vanilla/patched behavior, paired
shell32 L2, formal cross review and consumer validation remain open.
Invalid-input, concurrent-mutation and OOM-injection coverage is not claimed.
No production marker upgrade or promotion is made by the native result.

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).

## History

- **2026-11-11, promoted** into `patches/series` for winum-11.20-1 from `shell32-folderview-selection-v2` (maintainer approval of `docs/release/winum-11.20-1.md` at `ef64451`). Open: unmeasured NULL/invalid PIDL branches.
