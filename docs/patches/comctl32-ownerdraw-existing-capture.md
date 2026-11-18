# comctl32-ownerdraw-existing-capture.patch

<!-- winum-test: l1 -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `comctl32`
- **Origin:** Solidworks-for-Linux (private for now), `patches/`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

PropertyManager: the small checkmark and red X did not react to mouse clicks.

## Cause and fix

An owner-drawn v6 button keeps an existing mouse capture instead of taking it away.

## Wine test

None in the patch.

## Validation

| Level | Result |
| --- | --- |
| L0 | full builds full-1/full-2; winum-11.20-1 series (promoted 2026-11-11): every entry applies with `--fuzz=0` on the official 11.20 archive; the tree is identical to that of `test-run/all-6` (run 42997560598) |
| L1 | batch1, both arch (promoted file: `comctl32-ownerdraw-existing-capture-v2`) |
| L2 | not recorded; winum-11.20-1: ok |
| L3 | superclass/reentrant capture reproducer (x64) |
| L4 | Line, Mate accept/cancel, assembly save/reopen; winum-11.20-1: Inventor smoke; SOLIDWORKS deferred |
| L5 | in use since 2026-10-28 (maintainer to confirm) |
| Cross review | Codex, approve |

### winum-11.22-1 (Wine 11.22)

| Level | Result | Date |
| --- | --- | --- |
| L0 | the whole series on the official Wine 11.22 archive, both architectures, `--fuzz=0` ([run 43703211594](https://github.com/Harvi654/winum/actions/runs/43703211594)) | 2026-11-15 |
| L1 | unchanged from winum-11.20-2: the patch and its test are the same, and Windows did not change | - |
| L2 | [run 43703213332](https://github.com/Harvi654/winum/actions/runs/43703213332), group g3 re-run [43739605851](https://github.com/Harvi654/winum/actions/runs/43739605851), paired against vanilla Wine 11.22: only the known flaky failures (ntdll threadpool, ntdll time, urlmon `url.c:3535` not repeated in the re-run); the workflow shows "failure" because of these | 2026-11-15 |
| L4 | Inventor 2027 on a copy of the maintainer's prefix: import, parts, assemblies (maintainer) - a smoke test of the build, not a retest of this patch's own feature. SOLIDWORKS and AutoCAD: deferred (not being worked on since 2026-11-09, AGENTS.md) | 2026-11-15 |

## Open points

- **2026-11-18: does not work in SOLIDWORKS 2026 on the winum-11.22-1
  release.** The maintainer started SOLIDWORKS on a copy of the working
  prefix (`clean-g6`, updated with `wineboot -u`, GLX for `SLDWORKS.exe`):
  the small PropertyManager checkmark and red X again do not react to
  mouse clicks - the problem this patch fixed. The L4 that showed the fix
  was made with the earlier `buttoncapture` runtime; the series holds v2
  (changed for L1), and its SOLIDWORKS L4 has been deferred since
  2026-11-09. Not analysed yet. To do when SOLIDWORKS is picked up again:
  diff the L4-tested version against v2; check whether another patch of
  the series or a change between Wine 11.20 and 11.22 interferes
  (`--trace-confirmation` in Solidworks-for-Linux,
  `docs/wine-repro/confirmation-input/`); fix, then a real SOLIDWORKS L4.
  Meanwhile: Enter or the right-click menu confirms.

## Evidence

Solidworks-for-Linux `docs/wine-repro/confirmation-input/README.md`; overview in `docs/patch-status.md`.

## History

- **2026-11-11, promoted** into `patches/series` for winum-11.20-1 from `comctl32-ownerdraw-existing-capture-v2` (maintainer approval of `docs/release/winum-11.20-1.md` at `ef64451`).
