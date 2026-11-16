# winex11-flush-putimage.patch

<!-- winum-test: l3-only: Xlib request buffering has no Windows counterpart; reproducer in docs/wine-repro/winex11-flush-putimage -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `winex11.drv`
- **Origin:** Autodesk-Inventor-for-Linux, commit `aa626af`, first shipped in `wine-11.19-1`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

WebView2 pages (Home tab, licensing) were one frame behind: a hover showed only after the next mouse move.

## Cause and fix

`XFlush` after `X11DRV_PutImage` and `xrenderdrv_PutImage`.

## Wine test

No Windows test for Xlib request submission. The original own-window L3
fixture and independent observer are in
[`winex11-flush-putimage`](../wine-repro/winex11-flush-putimage/README.md).

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.19-1` (2026-10-19, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06) |
| L1 | L3 only: Windows has no Xlib request buffer or Wine X11 driver |
| L2 | not recorded per patch |
| L3 | reproducer added 2026-11-09; 2026-11-11, [run 42999903587](../wine-repro/l3-ci-2026-11-11/README.md), KDE Wayland: explicit XFlush-call behavior confirmed (next same-writer/display XFlush 1.7-7.7 s later without the patch, same millisecond tick with it; actual buffer residence/delivery not measured); pixel sampling inconclusive (DC read stayed at background in both builds) |
| L4 | Fixed with `--disable-gpu`; with WebView2's GPU process another path still lags. (Inventor, maintainer) |
| L5 | in Inventor's runtime since `wine-11.19-1` (2026-10-19); a week of use not formally recorded - maintainer to confirm |
| Cross review | not recorded (written before the cross-review rule) |

### winum-11.22-1 (Wine 11.22)

| Level | Result | Date |
| --- | --- | --- |
| L0 | the whole series on the official Wine 11.22 archive, both architectures, `--fuzz=0` ([run 43703211594](https://github.com/Harvi654/winum/actions/runs/43703211594)) | 2026-11-15 |
| L1 | no Windows L1 (`l3-only`, reason above); the patch is the same as in winum-11.20-2 | - |
| L2 | [run 43703213332](https://github.com/Harvi654/winum/actions/runs/43703213332), group g3 re-run [43739605851](https://github.com/Harvi654/winum/actions/runs/43739605851), paired against vanilla Wine 11.22: only the known flaky failures (ntdll threadpool, ntdll time, urlmon `url.c:3535` not repeated in the re-run); the workflow shows "failure" because of these | 2026-11-15 |
| L4 | Inventor 2027 on a copy of the maintainer's prefix: import, parts, assemblies (maintainer) - a smoke test of the build, not a retest of this patch's own feature. SOLIDWORKS and AutoCAD: deferred (not being worked on since 2026-11-09, AGENTS.md) | 2026-11-15 |

## Open points

No Wine test possible for X11 flushing; X11-side behaviour.

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).

