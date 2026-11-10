# Cross review: shell32 folder-view selection v2 (2026-11-10)

Reviewer: Claude; candidate author: Codex. Candidate as on main `f64f3b9`.

Read in full: the candidate patch (all `shlview.c` hunks, `tests/Makefile.in`
and the new `tests/selection.c`) and its README. Read in Wine 11.20:
`FolderView_SelectItem`, `IShellView_fnSelectItem`, `LV_FindItemByPidl`
use, and `SVGIO_TYPE_MASK` in `shobjidl.idl`. The production patch was
compared by hunk (v2 adds the Wine header, the collector and the
`ERROR_NOT_FOUND` paths). The standalone L1 harness and builders were not
re-read; the native results are taken from the README.

## Findings

- **P1: none.** `ItemCount`/`Items` now honour `SVGIO_SELECTION` through
  the type mask, so `SVGIO_FLAG_VIEWORDER` is ignored as it should be. The
  private collector initialises its outputs, checks the size, separates
  out-of-memory from "no items", and fails when the walk finds fewer items
  than the count (no partly filled array reaches `SHCreateShellItemArray`).
  Child PIDLs stay owned by the view and only the pointer vector is freed,
  after the array has been built. `GetSelection(TRUE)` with nothing
  selected builds a one-item array from `GetCurFolder` and frees that PIDL.
- **P2: none in the implementation's behaviour against vanilla.** Every
  method changed was a stub (`E_NOTIMPL`) or ignored the selection flag,
  except the empty `ALLVIEW` path, which keeps vanilla's `E_FAIL`
  (unmeasured, as the README says). So the patch does not change anything
  vanilla already did like Windows.
- **P3: unmeasured branches.** `E_POINTER` for NULL outputs and
  `E_INVALIDARG` for an unknown PIDL in `GetSelectionState` (with `*flags`
  set to `SVSI_DESELECT` first) have no Windows basis and no test. They
  replace `E_NOTIMPL`, so no regression, but they belong in the patch
  page's open points as guesses to measure.
- **P3: the collector is not atomic** against the list view changing
  during the walk (README); the length check turns that into `E_FAIL`
  rather than a wrong array, which is acceptable.

## Test

Expected values are the measured ones for the four states (none, B with
focus, B+C, cleared with B's focus kept): counts, item identities as sets
(no order assumed), `ERROR_NOT_FOUND` with a NULL array for an empty
selection, the own folder for `GetSelection(TRUE)`, exclusive
`GetSelectedItem` from starts -1..2 against the real view indices, and
selection/focus flags. Wine's `FolderView_SelectItem` with `SVSI_DESELECT`
clears only `LVIS_SELECTED`, so the retained focus of the last state is
what Wine can produce too. One point is not settled by reading: the
first state expects no focused item after `SVUIA_ACTIVATE_NOFOCUS`; if
Wine's list view focuses item 0 when the view is created, that check
fails on the patched build as well. The `test-run/all-1` L2 (shell32 in
g5, run 42788810738) will show it. Cleanup releases every interface,
PIDL, window and own file, and the host's reference count is checked.

## Checks and verdict

All series entries apply at `--fuzz=0` with this candidate substituted,
also with every current candidate together (`test-run/all-1`, 64
entries). Native assertion L1: 357 checks, 0 failures, four states, both
architectures (README, 2026-11-09). No Wine runtime run by this reviewer.

Verdict: **approve**, with promotion after `test-run/all-1` shows
`shell32:selection` without failures on the series side (vanilla has no
such unit) and no new failures in the other shell32 units. If the first
state's focus check fails there, the fix belongs in the test or a
`todo_wine` with a reason, not in this patch.
