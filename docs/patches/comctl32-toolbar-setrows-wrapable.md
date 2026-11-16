# comctl32-toolbar-setrows-wrapable.patch

<!-- winum-test: l1 -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `comctl32`
- **Origin:** Autodesk-Inventor-for-Linux, commit `eddb691`, first shipped in `wine-11.19-1`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Drawing text editor: the symbol palette was one long column instead of a grid.

## Cause and fix

`TB_SETROWS` keeps its wrap points and picks the fewest buttons per row with which the separator groups fit the requested rows; later resizes keep that number of columns.

## Wine test

None in the patch.

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.19-1` (2026-10-19, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06); winum-11.20-1 series (promoted 2026-11-11): every entry applies with `--fuzz=0` on the official 11.20 archive; the tree is identical to that of `test-run/all-6` (run 42997560598) |
| L1 | all-5: every setrows check passed, both arch (one unrelated x86_64 failure in the old `test_WM_PAINT`, outside messages) (promoted file: `comctl32-toolbar-setrows-v5`) |
| L2 | not recorded per patch; winum-11.20-1: ok (all-5) |
| L3 | not recorded |
| L4 | 5 columns and groups as on Windows. (Inventor, maintainer); winum-11.20-1: Inventor palette as on Windows (all-5) |
| L5 | in Inventor's runtime since `wine-11.19-1` (2026-10-19); a week of use not formally recorded - maintainer to confirm |
| Cross review | Codex, approve (9b96bc7); v4 has no review, replaced |

### winum-11.22-1 (Wine 11.22)

| Level | Result | Date |
| --- | --- | --- |
| L0 | the whole series on the official Wine 11.22 archive, both architectures, `--fuzz=0` ([run 43703211594](https://github.com/Harvi654/winum/actions/runs/43703211594)) | 2026-11-15 |
| L1 | unchanged from winum-11.20-2: the patch and its test are the same, and Windows did not change | - |
| L2 | [run 43703213332](https://github.com/Harvi654/winum/actions/runs/43703213332), group g3 re-run [43739605851](https://github.com/Harvi654/winum/actions/runs/43739605851), paired against vanilla Wine 11.22: only the known flaky failures (ntdll threadpool, ntdll time, urlmon `url.c:3535` not repeated in the re-run); the workflow shows "failure" because of these | 2026-11-15 |
| L4 | Inventor 2027 on a copy of the maintainer's prefix: import, parts, assemblies (maintainer) - a smoke test of the build, not a retest of this patch's own feature. SOLIDWORKS and AutoCAD: deferred (not being worked on since 2026-11-09, AGENTS.md) | 2026-11-15 |

## Open points

Wine test missing; Windows column choice compared visually only.

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).

## History

- **2026-11-11, promoted** into `patches/series` for winum-11.20-1 from `comctl32-toolbar-setrows-v5` (maintainer approval of `docs/release/winum-11.20-1.md` at `ef64451`). Open: 4 vertical cases differ from Windows (3+4 groups, 4 rows); Wine's width wrap of vertical toolbars before `TB_SETROWS`.
