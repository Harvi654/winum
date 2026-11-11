# ntdll-actctx-threadingmodel-case.patch

<!-- winum-test: l1 -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `ntdll`
- **Origin:** Autodesk-Inventor-for-Linux, commit `240d730`, first shipped in `wine-11.19-1`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Stress Analysis: deadlock, Inventor stopped responding.

## Cause and fix

The manifest parser compares `threadingModel` case-insensitively (`free` = Free), as Windows and Wine's registry path do.

## Wine test

None in the production patch. The
[v2 candidate](../../patches/candidates/ntdll-actctx-threadingmodel-case-v2/README.md)
adds cases to kernel32/tests/actctx.c from our 42-case Windows MEASURE,
as requested in cross review. It reads comclassredirect_data directly and
checks both comClass and clrClass value spellings, with separate
todo_wine controls for rejection and omitted-model defaults.

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.19-1` (2026-10-19, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06); winum-11.20-1 series (promoted 2026-11-11): every entry applies with `--fuzz=0` on the official 11.20 archive; the tree is identical to that of `test-run/all-6` (run 42997560598) |
| L1 | all-2: `kernel32:actctx` 3863/0, both arch (promoted file: `ntdll-actctx-threadingmodel-case-v2`) |
| L2 | not recorded per patch; winum-11.20-1: ok (kernel32 in g3 since all-2) |
| L3 | not recorded |
| L4 | Stress Analysis runs. (Inventor, maintainer); winum-11.20-1: Stress Analysis runs (v1 parser, unchanged) |
| L5 | in Inventor's runtime since `wine-11.19-1` (2026-10-19); a week of use not formally recorded - maintainer to confirm |
| Cross review | Claude, approve + re-read of the follow-up (2026-11-11) |

## Open points

Candidate test passed native L1; formal cross review, vanilla/patched runtime
comparison and paired L2 remain open before promotion. The production marker
stays missing because its patch does not yet include that test.

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).


## History

- **2026-11-11, promoted** into `patches/series` for winum-11.20-1 from `ntdll-actctx-threadingmodel-case-v2` (maintainer approval of `docs/release/winum-11.20-1.md` at `ef64451`). Open: missing-attribute defaults and invalid-value rejection stay todo_wine.

- 2026-11-09: Recorded native MEASURE and assertion L1 for the v2 candidate.
  The production case-folding implementation is unchanged. All 62 patches
  apply at fuzz 0 with the candidate and focused ntdll DLL/test builds passed
  for both architectures. No production patch or series change is made here.

- 2026-11-09, review follow-up: Relocated the candidate matrix to kernel32's
  existing activation-context test and removed the raw-prefix reader and
  delimiter markers. Expected values and production parser code are
  unchanged. Complete kernel32 test EXEs and ntdll DLLs linked for both
  architectures; refreshed native reader L1 and kernel32.actctx L2 remain open.

- 2026-11-09, native refresh: The revised direct reader passed 575 checks
  with zero failures and exit 0 on both Windows 10 architectures. Kernel32
  actctx L2 remains open.
