# wbemprox-timezone.patch

<!-- winum-test: l1 -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `wbemprox`
- **Origin:** Autodesk-Inventor-for-Linux, commit `e2ffae5`, first shipped in `wine-11.19-1`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

3D viewport crash (earlier worked around by patching Autodesk's OGSFactory.dll).

## Cause and fix

WMI class `Win32_TimeZone` added.

## Wine test

None in the production patch. The separate
[v2 candidate](../../patches/candidates/wbemprox-timezone-v2/README.md)
adds a Windows-derived 21-property test in dlls/wbemprox/tests/timezone.c
and corrects the measured Bias sign, StandardBias type and Display labels.
Its exact-body standalone test passes native L1 in the measured stable
standard-time setup. Unmeasured timezone modes and other validation gates
are explicitly documented; the candidate is not in patches/series.

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.19-1` (2026-10-19, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06); winum-11.20-1 series (promoted 2026-11-11): every entry applies with `--fuzz=0` on the official 11.20 archive; the tree is identical to that of `test-run/all-6` (run 42997560598) |
| L1 | v2 body 104/0 (v3 differs only in skip()) (promoted file: `wbemprox-timezone-v3`) |
| L2 | not recorded per patch; winum-11.20-1: ok (83/0, all-2..5) |
| L3 | not recorded |
| L4 | Inventor runs with unmodified Autodesk files (2026-10-15). (Inventor, maintainer); winum-11.20-1: Inventor runs (v1) |
| L5 | in Inventor's runtime since `wine-11.19-1` (2026-10-19); a week of use not formally recorded - maintainer to confirm |
| Cross review | Claude, approve v3 (2026-11-11) |

### winum-11.22-1 (Wine 11.22)

| Level | Result | Date |
| --- | --- | --- |
| L0 | the whole series on the official Wine 11.22 archive, both architectures, `--fuzz=0` ([run 43703211594](https://github.com/Harvi654/winum/actions/runs/43703211594)) | 2026-11-15 |
| L1 | unchanged from winum-11.20-2: the patch and its test are the same, and Windows did not change | - |
| L2 | [run 43703213332](https://github.com/Harvi654/winum/actions/runs/43703213332), group g3 re-run [43739605851](https://github.com/Harvi654/winum/actions/runs/43739605851), paired against vanilla Wine 11.22: only the known flaky failures (ntdll threadpool, ntdll time, urlmon `url.c:3535` not repeated in the re-run); the workflow shows "failure" because of these | 2026-11-15 |
| L4 | Inventor 2027 on a copy of the maintainer's prefix: import, parts, assemblies (maintainer) - a smoke test of the build, not a retest of this patch's own feature. SOLIDWORKS and AutoCAD: deferred (not being worked on since 2026-11-09, AGENTS.md) | 2026-11-15 |

## Open points

Candidate cross review, paired Wine runtime/L2 and consumer validation remain
open. Active daylight time, nonzero standard adjustment, disabled automatic
DST, successful MUI localization and custom-zone fallbacks remain unmeasured.
The native sample does not distinguish negated base bias from negated
standard/active totals outside the tested setup. No production marker
upgrade or candidate promotion is claimed yet.

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).


## Candidate v3 environment-skip correction (2026-11-10)

[V3](../../patches/candidates/wbemprox-timezone-v3/README.md) changes only
the two fixture-limit win_skip calls to skip. All-1's UTC failure is a
test classification error; the measured standard-time value checks do not
apply there. Class/type/presence checks stay ordinary. The maintainer
accepts the existing native L1 for identical Windows semantics, without a
rerun. UTC value coverage (review P2) and unmeasured timezone modes remain
open; paired v3 runtime validation is pending. Production marker unchanged.

## History

- **2026-11-11, promoted** into `patches/series` for winum-11.20-1 from `wbemprox-timezone-v3` (maintainer approval of `docs/release/winum-11.20-1.md` at `ef64451`). Open: no value coverage in the UTC CI; unmeasured DST modes.
