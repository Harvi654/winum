# comctl32-toolbar-padding-height.patch

<!-- winum-test: l1 -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `comctl32`
- **Origin:** Autodesk-Inventor-for-Linux, commit `8efbd55`, first shipped in `wine-11.19-1`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Drawing text editor: the symbol palette was taller than on Windows and needed a scroll bar.

## Cause and fix

Button height padding is taken from `TB_SETPADDING` like the width.

## Wine test

None in the patch.

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.19-1` (2026-10-19, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06); winum-11.20-1 series (promoted 2026-11-11): every entry applies with `--fuzz=0` on the official 11.20 archive; the tree is identical to that of `test-run/all-6` (run 42997560598) |
| L1 | batch1-6, all-3..5 (`comctl32:toolbar`), both arch (promoted file: `comctl32-toolbar-padding-height-v5`) |
| L2 | not recorded per patch; winum-11.20-1: ok |
| L3 | not recorded |
| L4 | Palette as on Windows (screenshots compared by the maintainer). (Inventor, maintainer); winum-11.20-1: Inventor palette (all-1, all-5) |
| L5 | in Inventor's runtime since `wine-11.19-1` (2026-10-19); a week of use not formally recorded - maintainer to confirm |
| Cross review | Codex, approve v5 |

## Open points

Wine test missing.

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).

## History

- **2026-11-11, promoted** into `patches/series` for winum-11.20-1 from `comctl32-toolbar-padding-height-v5` (maintainer approval of `docs/release/winum-11.20-1.md` at `ef64451`). Open: none; the button size difference found on 2026-11-10 was setrows (fixed there).
