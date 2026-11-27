# server-mousemove-on-show-hide.patch

<!-- winum-test: l1 -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `server`
- **Origin:** Autodesk-Inventor-for-Linux, commit `e236679`, first shipped in `wine-11.19-1`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Sketch: an edited dimension showed its new value only after the mouse moved.

## Cause and fix

Like Windows, the window under the cursor gets a mouse move when a window is shown or hidden (one per burst, 100 ms later).

## Wine test

None in the patch.

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.19-1` (2026-10-19, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06); winum-11.20-1 series (promoted 2026-11-11): every entry applies with `--fuzz=0` on the official 11.20 archive; the tree is identical to that of `test-run/all-6` (run 42997560598) |
| L1 | all-4: two runs, both arch, 0 failures (promoted file: `server-mousemove-on-show-hide-v5`) |
| L2 | not recorded per patch; winum-11.20-1: ok (all-4, all-5) |
| L3 | not recorded |
| L4 | Values update at once. (Inventor, maintainer); winum-11.20-1: sketch value updates at once with 32 ms (all-2) |
| L5 | in Inventor's runtime since `wine-11.19-1` (2026-10-19); a week of use not formally recorded - maintainer to confirm |
| Cross review | Codex, approve (2c8870f) |

### winum-11.22-1 (Wine 11.22)

| Level | Result | Date |
| --- | --- | --- |
| L0 | the whole series on the official Wine 11.22 archive, both architectures, `--fuzz=0` ([run 43703211594](https://github.com/Harvi654/winum/actions/runs/43703211594)) | 2026-11-15 |
| L1 | unchanged from winum-11.20-2: the patch and its test are the same, and Windows did not change | - |
| L2 | [run 43703213332](https://github.com/Harvi654/winum/actions/runs/43703213332), group g3 re-run [43739605851](https://github.com/Harvi654/winum/actions/runs/43739605851), paired against vanilla Wine 11.22: only the known flaky failures (ntdll threadpool, ntdll time, urlmon `url.c:3535` not repeated in the re-run); the workflow shows "failure" because of these | 2026-11-15 |
| L4 | Inventor 2027 on a copy of the maintainer's prefix: import, parts, assemblies (maintainer) - a smoke test of the build, not a retest of this patch's own feature. SOLIDWORKS and AutoCAD: deferred (not being worked on since 2026-11-09, AGENTS.md) | 2026-11-15 |

## Open points

L4 2026-11-10: 16 ms (v2 as first written) does not fix the sketch redraw in Inventor 2027; 16/24 ms fail, 32/100 ms work. v2 now defaults to 32 ms (`patches/candidates/server-mousemove-on-show-hide-v2/README.md`).

Wine test missing; the 100 ms delay is empirical.

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).

## History

- **2026-11-11, promoted** into `patches/series` for winum-11.20-1 from `server-mousemove-on-show-hide-v5` (maintainer approval of `docs/release/winum-11.20-1.md` at `ef64451`). Open: hidden-window move todo_wine; counts saturate at 16 (P3); host input can add moves (P3).

## Wine 11.23

v6 (2026-11-27): Rebased to Wine 11.23, context only, logic unchanged. The delayed show/hide arm compares the upstream old and new visibility values. The Windows test expectations are unchanged. The source series applies with `--fuzz=0`; the L0 build and paired checks are pending.
