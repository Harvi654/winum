# Cross review: wbemprox Win32_TimeZone v2 (2026-11-10)

Reviewer: Claude; candidate author: Codex. Candidate as on main `f64f3b9`.

Read in full: the candidate patch (the `builtin.c` hunks, `tests/Makefile.in`
and the new `tests/timezone.c`), its README, and the production patch for
comparison. Read in Wine 11.20: the `Win32_TimeZone` column table and fill
function, `winetest_win_skip` in `include/wine/test.h`, and the time zone
id logic in `kernelbase/locale.c`. The standalone L1 harness and builder
were not re-read; the native results are taken from the README.

## Findings

- **P1 (test): `win_skip` fails on Wine, and the CI runs in UTC.** When the
  setup is not "stable standard time", the test calls `win_skip`, which is
  a skip on Windows but `ok(0)` on Wine (`winetest_win_skip`). The test
  series job runs in an `archlinux:base-devel` container with no local
  time zone, i.e. UTC. UTC has no daylight date, so
  `GetTimeZoneInformation` returns `TIME_ZONE_ID_UNKNOWN`
  (`kernelbase/locale.c`, `!DaylightDate.wMonth`), `comparable` is FALSE,
  and the patched side of every L2 run gets one failure in
  `wbemprox:timezone` that vanilla (which has no such unit) does not -
  a new failure caused by the test, not by the fix. The same happens on a
  Linux machine during summer time. Derived from reading; the
  `test-run/all-1` report (run 42788810738) should show it. Fix: use
  `skip()` for the "not measured here" cases (they are environment
  limits, not Wine bugs), keeping the type and presence checks as `ok()`.
- **P2 (test): in CI the values are never compared.** Under UTC only the
  CIM/variant types and presence are checked, so the regression the patch
  fixes (Display label as Caption/Description, the sign of Bias) is not
  covered by L2 at all; only the StandardBias type is. A second, fixed
  case that is measurable everywhere would help, for example running the
  comparison for `TIME_ZONE_ID_UNKNOWN` as well - but only after a
  Windows MEASURE in a zone without daylight time (e.g. UTC) shows the
  same relation; the README says this mode is unmeasured, so it must not
  be assumed. Alternatively the L2 job could set a time zone with
  standard time in November, which is fragile (changes with the season).
- **P1/P2 (implementation): none.** Caption/Description are dynamic
  strings from the current zone's registry `Display` with bounds, type
  and terminator checks and an empty-string fallback; Bias is
  `-tzi.Bias` (the base bias, as measured in standard time with zero
  StandardBias); StandardBias is `CIM_UINT32`; an invalid time zone fails
  the fill; the record is zeroed and the row's values are freed both on an
  allocation failure and when the filter rejects the row (the v1 leak).
  String ownership is separate for Caption and Description.
- **P3: the measured basis is narrow** (standard time, zero standard
  adjustment, automatic DST on). The implementation reports the base bias
  in all modes; whether Windows reports the active bias in daylight time
  is unknown. The README states this; it belongs in the patch page's open
  points.
- **P3: `StandardBias` is checked as `VT_I4`** with CIM type `CIM_UINT32`,
  which matches the native measurement (the README says so); fine.

## Checks and verdict

All series entries apply at `--fuzz=0` with this candidate substituted,
also with every current candidate together (`test-run/all-1`, 64 entries).
Native assertion L1: 104 checks, 0 failures, 0 skips, both architectures,
in the measured standard-time setup (README, 2026-11-09). `wbemprox` is in
group g5. No Wine runtime run by this reviewer.

Verdict: **implementation approved; the test needs a v3** (`win_skip` ->
`skip` for the unmeasured setups, P1) before promotion, and a re-run of L1
is not needed for that change alone (on Windows `win_skip` and `skip` are
the same). P2 is a coverage gap to record or close with a MEASURE.

## Confirmed by L2 (2026-11-10)

`test-run/all-1` (run 42788810738): `wbemprox:timezone` 84 checks, 1
failure per architecture at `timezone.c:186`, the first `win_skip`. P1
confirmed; see `docs/wine-repro/l2-all-1-2026-11-10/`.

## Re-read of v3 (Claude, 2026-11-11)

v3 (`9c736d1`, Codex) differs from v2 only in the two `win_skip` calls,
now `skip` (diff checked: the `builtin.c` hunks are byte-identical).
That is the P1 change requested above. On Windows both are skips, so
the v2 native L1 (104 checks, 0 failures) stands for v3; L2 all-2 to
all-5: `wbemprox:timezone` 83 checks, 0 failures, both architectures
(the UTC container now skips the value comparisons). P2 (no value
coverage in UTC) stays an open point. Verdict: **approve v3**.
