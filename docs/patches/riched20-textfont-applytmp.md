# riched20-textfont-applytmp.patch

<!-- winum-test: l1 -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `riched20`
- **Origin:** Autodesk-Inventor-for-Linux, commit `d029ebd`, first shipped in `wine-11.19-1`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Drawings: text turned black after the spell checker ran.

## Cause and fix

`ITextFont::Reset(tomApplyTmp)` drops temporary formatting and the following `tomApplyNow` no longer writes stale cached properties back.

## Wine test

None in the patch.

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.19-1` (2026-10-19, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06); winum-11.20-1 series (promoted 2026-11-11): every entry applies with `--fuzz=0` on the official 11.20 archive; the tree is identical to that of `test-run/all-6` (run 42997560598) |
| L1 | batch1-6, both arch (promoted file: `riched20-textfont-applytmp-v6`) |
| L2 | not recorded per patch; winum-11.20-1: ok |
| L3 | not recorded |
| L4 | No more black text. (Inventor, maintainer); winum-11.20-1: repeated spell check, no black text (all-1) |
| L5 | in Inventor's runtime since `wine-11.19-1` (2026-10-19); a week of use not formally recorded - maintainer to confirm |
| Cross review | Codex, approve v6 |

## Open points

Wine test missing; temporary formatting (spell-check underline) is dropped, not implemented.

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).

## History

- **2026-11-11, promoted** into `patches/series` for winum-11.20-1 from `riched20-textfont-applytmp-v6` (maintainer approval of `docs/release/winum-11.20-1.md` at `ef64451`). Open: second unread temp cycle todo_wine.
