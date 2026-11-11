# urlmon-res-zone-local-machine.patch

<!-- winum-test: l1 -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `urlmon`
- **Origin:** Autodesk-Inventor-for-Linux, commit `f34967c`, first shipped in `wine-11.20-5`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

AutoCAD 2027 (experimental branch): status bar XAML failed and AutoCAD shut down.

## Cause and fix

`res:` URLs without a host map to the local machine zone.

## Wine test

None in the patch.

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.20-5` (2026-10-21, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06); winum-11.20-1 series (promoted 2026-11-11): every entry applies with `--fuzz=0` on the official 11.20 archive; the tree is identical to that of `test-run/all-6` (run 42997560598) |
| L1 | all-2: `urlmon:sec_mgr` 692/0, both arch (promoted file: `urlmon-res-zone-v2` (includes the tests-first test)) |
| L2 | not recorded per patch; winum-11.20-1: ok |
| L3 | not recorded |
| L4 | AutoCAD 2027 starts (experimental). (Inventor, maintainer); winum-11.20-1: AutoCAD, deferred |
| L5 | in Inventor's runtime since `wine-11.20-5` (2026-10-21); a week of use not formally recorded - maintainer to confirm |
| Cross review | Codex, approve after follow-up (bf3c3b9) |

## Open points

AutoCAD (decision 2026-11-09): AutoCAD is not installed and is not being worked on now. The patch stays in the series as it is; it gets its Wine test (L1, L2) like every other patch, and the AutoCAD smoke test (L4) is deferred until there is time for AutoCAD again.

Measured 2026-11-09 (`docs/wine-repro/urlmon-zones/`): Windows puts a `res:///` URL without a module in zone 3 (Internet), not 0, and fails the literal `%prodname%` URL with E_INVALIDARG. The corrected patch is `patches/candidates/urlmon-res-zone-v2/` (cross reviewed by Codex 2026-11-09); test first in `patches/tests-first/urlmon-tests-res-zones.patch`.

Wine test missing; Windows zone for res: without a module not measured by our own program.

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).

## History

- **2026-11-11, promoted** into `patches/series` for winum-11.20-1 from `urlmon-res-zone-v2` (includes the tests-first test) (maintainer approval of `docs/release/winum-11.20-1.md` at `ef64451`). Open: module res: URL todo_wine; `patches/tests-first/series` entry is removed with it.
