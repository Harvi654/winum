# Focused v3 setup pair: test-run/batch4-2

Run record only; never merge. Based on main 7f5fc01. Split the v3 candidate
into implementation at its existing production position and identical new
tests through tests-first on both variants. Production patches/series is
unchanged. Limit this run to user32:attachment on i386/x86_64; unchanged
neighbor suites were already paired in batch4-1 and the maintainer supplied
attribution for known unrelated v1 failures.

The existing workflow builds both full Wine variants, packs the exact new
unit for native Windows and reports absolute failures and scenario coverage.
Its branch-only gate rejects incomplete/all-skipped coverage and ordinary
series failures; vanilla must still demonstrate a target failure. Allow
600 seconds per unit for the added bounded event-drain preparation.
The native v3 body needs a new L1. No v2 pass transfers to it.
