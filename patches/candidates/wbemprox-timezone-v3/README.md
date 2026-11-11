# Win32_TimeZone v3: environment skips stay skips on Wine

> **Promoted 2026-11-11** into `patches/series` for winum-11.20-1. This folder stays as the record of the candidate.

Candidate only; production patch, patches/series and marker are unchanged.
Supersedes [v2](../wbemprox-timezone-v2/README.md) for further validation.
The implementation, measured native values and test conditions are exactly
v2's. Only its two unmeasured-setup win_skip calls become skip.

## Reason and limits (2026-11-10)

[The v2 review](../../../docs/wine-repro/cross-review-2026-11-10-wbemprox-timezone-v2.md)
P1 was confirmed by [all-1](../../../docs/wine-repro/l2-all-1-2026-11-10/README.md):
CI runs in UTC, outside the stable standard-time state measured on Windows.
win_skip reports a failure on Wine; skip reports the genuine environment
limit on both platforms. The absent readable Display source likewise
means an unmeasured label comparison, not a Wine failure.

Class existence, enumeration, property presence and all 21 measured
CIM/VARIANT types remain ordinary assertions in every environment. The
value and label comparisons retain their exact original prerequisites;
no unmeasured UTC relation or new expected value is inferred. This fixes
the false CI failure, not the P2 lack of value coverage in UTC. Active
DST, custom zones, disabled automatic DST, nonzero standard adjustment and
successful MUI/fallback behavior retain v2's disclosed measurement limits.
A UTC run with an explicit value-scope skip is partial coverage.

## Native validation and checks

The maintainer explicitly accepts v2's native L1 without a rerun for this
skip-only change: both calls have identical Windows semantics. Its measured
stable-standard-time run passed 104 checks, zero failures/skips and exit 0
on x86_64/i686 (2026-11-09); the value comparisons are unchanged. The
standalone wrapper maps skip to the same native function as before, and
the default builder now verifies parity with v3. Both native builds still
compile with -Wall -Wextra -Werror; no native or Wine runtime rerun is
claimed. All 62 production entries apply with v3 substituted at --fuzz=0 on
pinned Wine 11.20 source 7298a818b79446bae153b8d5aa27235ab3703bf9.

The next paired L2 must confirm the UTC test completes with an explained
scope skip and zero ordinary failures; all-1 ran v2, so its failure is not
a v3 runtime pass. Cross-review P2 remains recorded, not closed by the
renaming. Promotion and remaining consumer validation are still separate
maintainer decisions.
