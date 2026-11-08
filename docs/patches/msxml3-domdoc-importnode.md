# msxml3-domdoc-importnode.patch

<!-- winum-test: l1 -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `msxml3`
- **Origin:** Autodesk-Inventor-for-Linux, commit `b7c6cea`, first shipped in `wine-11.20-3`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Frame Generator: Insert Frame created only an empty folder.

## Cause and fix

`IXMLDOMDocument3::importNode` is implemented (a clone of the node owned by the importing document, without a parent).

## Wine test

`dlls/msxml3/tests/domdoc.c`

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.20-3` (2026-10-19, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06) |
| L1 | Windows 10 VM (Mac host), L1 bundle of run 42483162074 (2026-11-08): msxml3 domdoc 36828 checks both architectures; the patch's `test_importNode` checks pass (the one failure belongs to msxml3-parseerror-filepos) |
| L2 | not recorded per patch |
| L3 | not recorded |
| L4 | Insert Frame creates members (wine-11.20-3 onwards). (Inventor, maintainer) |
| L5 | in Inventor's runtime since `wine-11.20-3` (2026-10-19); a week of use not formally recorded - maintainer to confirm |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

Its Wine test was never run on Windows.

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).

