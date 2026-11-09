# Focused v4 component/absolute pair: test-run/batch4-3

Round 2 run record only; never merge. Based on main b75a5e1. Split v4
into implementation at the existing production position and identical
new tests through tests-first on vanilla and series. Production series
is unchanged. Run only user32:attachment, both PE architectures; do not
chase the known unrelated full-series v1 issues.

Require 44 completed/no skipped scenarios, 592 component snapshots and
2180 positive comparisons. Reject ordinary series failures and require
a vanilla target signal. Absolute state is a separate reported scope:
state 0 has one explained subtest skip; state 1 must cover all 592 absolute
snapshots. No extra skip is accepted. Coverage JSON includes both scopes.
An unavailable absolute view is not a claim that inactive-queue retention
passed on Wine; native v4 L1 must run both views. The exact unit is packed
for the next combined Windows validation after textfont-followup2.


The gate also emits filtered scope counters and up to three representative
failure messages per architecture as Actions notices. They are readable
on the public job page without downloading raw logs or using the API;
original structured coverage and result artifacts remain available.
