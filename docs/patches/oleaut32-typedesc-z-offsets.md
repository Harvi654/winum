# oleaut32-typedesc-z-offsets.patch

<!-- winum-test: missing -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `oleaut32`
- **Origin:** Autodesk-Inventor-for-Linux, commit `4627129`, first shipped in `wine-11.19-1`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Design Accelerator: "Failed to create shaft" (together with typedesc-pointee).

## Cause and fix

The typedesc table offsets of a VT_PTR/VT_SAFEARRAY pointee and a VT_CARRAY element are read as 32-bit values from two INT16 words; Wine used only the low word, which is negative for tables larger than 32 KB (Inventor's API type library).

## Wine test

None in the production patch. The
[v2 candidate](../../patches/candidates/oleaut32-typedesc-z-offsets-v2/README.md)
adds a type library compatibility test comparing stored type descriptions
with Windows. It constructs its own records and aliases using public APIs,
checks actual offset boundary coverage and reloads a fresh-path copy. No
vendor type library is used.

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.19-1` (2026-10-19, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06) |
| L1 | Corrected candidate body: Windows 10 x86_64/i686, 40095 checks each, 0 failures, exit 0 (2026-11-09; assertion revision 1ef60e0, combined recipe at 8400e60). Original body passed 40091 checks. Production has no integrated test yet |
| L2 | not recorded per patch |
| L3 | not recorded |
| L4 | Design Accelerator Shaft works. (Inventor, maintainer) |
| L5 | in Inventor's runtime since `wine-11.19-1` (2026-10-19); a week of use not formally recorded - maintainer to confirm |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

Candidate test passed native L1; formal cross review, vanilla/patched runtime
comparison and paired L2 remain open before promotion. The production marker
stays missing because its patch does not yet include that test.

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).


## History

- 2026-11-09: Recorded complete native MEASURE and assertion L1 for the v2
  candidate. All three production implementation hunks are unchanged; both
  full-series recipes apply at fuzz 0 and focused oleaut32 DLL/test builds
  succeeded. No production patch or series change is made by this record.
- 2026-11-09: Diagnosed candidates-4's size-zero failures as an independent
  record-writer layout gap. The candidate now keeps kind/count and all
  descriptor checks ordinary and marks only the native size-4 check todo_wine.
  Reader implementation hunks and fixture/API inputs are unchanged. Revised
  exact-body L1 and runtime pairing remain pending; the earlier native pass
  applies to the original combined-assertion revision.
- 2026-11-09: The corrected exact-body assertion revision also passes native
  L1 on both architectures: 40095 checks, zero failures and exit 0 each.
  Native checks retain size 4; the todo is Wine-only. Corrected Wine runtime,
  paired L2, formal cross review and promotion remain pending.
