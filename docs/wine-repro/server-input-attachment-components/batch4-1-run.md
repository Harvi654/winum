# Paired candidate run: test-run/batch4-1

Based on main b912850. This branch is a run record and is never merged.
The implementation-only half of the input-components v2 candidate replaces
the production patch at its existing series position. Its test-only half
is applied through patches/tests-first/series to both vanilla and series.
The candidate and both variants therefore have identical attachment.c and
Makefile test entries. Production patches/series is byte-identical to main.

The existing full ten-job L2 workflow covers the neighboring user32,
win32u and gdi32 suites as well as the remaining series suites. The g1
series artifact packs user32:attachment for both PE architectures. Native
assertion L1 is pending; the earlier MEASURE exit 0 is not that gate.
