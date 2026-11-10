# Server input attachment components v5: isolate first-child window choice

Candidate only, final test adjustment (round 3). Supersedes v4 for further
validation. Production patches/series, its patch and the missing test
marker remain unchanged. Server implementation and all 592 literal native
state rows plus 52 detach/error rows are byte-identical to v4.

## Evidence

[Round 2](../../../docs/wine-repro/server-input-attachment-components/l2-v4-2026-11-10/README.md)
completed all 44 scenarios, 592 component snapshots and 2180 positive
comparisons on both architectures. Vanilla has 302 ordinary failures,
series 12. Both omit the explicitly unavailable absolute inactive-state
view; neither skips a scenario. The returned original series JSON has
2 reference-active, 2 reference-focus, 4 membership-active and 4
membership-focus failures per architecture. First messages all identify
children-late1-unparent0 / one-child. It does not store every occurrence's
context: attributing the later occurrences to the other late-child row is
a hypothesis, to be checked by the final pair, not an established fact.

At the first reported checkpoint only child 0 has been created; SetParent,
child destruction and explicit detach have not run. The measured Windows
rows for both children-late1-* / one-child select creator C's top-level
window as active/focus in A and C (CBCD). Wine 11.20's unmodified creation
path attaches creator C to host A's input. assign_thread_input retains A's
nonzero active/focus instead of replacing it with C's. Those create/attach
functions are byte-identical in vanilla and the candidate; the fix only
changes detach and desktop reparent handling. This is an unchanged Wine
window-choice gap, distinct from shared-input membership.

## Scope of the test change

Only the two unfocused first-child checkpoints gain a narrow todo for the
reference's exact native active/focus choice. Windows still must satisfy
those exact assertions. If the optional absolute view runs, only A/C's
same exact choice gets that todo there too. No native value is changed.

At these two checkpoints, the ordinary component assertions use the
reference's observed active/focus as positive witnesses. Each must equal
a live fixture top-level window belonging to the component in the literal
Windows row (A or C here). NULL, child handles and B/D cannot be witnesses.
A and C must share those witnesses; B and D must not. All these membership
assertions, including the guards, remain ordinary. A missing or wrong
connection cannot pass merely because the window-choice todo is present.
Elsewhere exact native actor handles remain mandatory as in v4.

Capture, graph/API/error checks, scenario operations, worker lifecycle and
timeouts are unchanged. The older direct-destruction-B and self-detach
error todos retain their scope. With the absolute view unavailable, expect
16 behavioral todos (12 older plus four exact first-child choices), not
ordinary failures. Counts alone never establish correctness; unexpected
todo passes and failures need inspection. No target assertion is skipped.

V4's explicit absolute-view prerequisite remains: its absence does not
mean full Windows inactive-queue state passed on Wine. Component coverage
still requires 44 completed scenarios, zero scenario skips, 592 component
snapshots and 2180 positives. The native wrapper requires all 592 absolute
snapshots too, absolute_state=1, zero failures and no skips; no earlier
native pass transfers to v5.

## Validation (2026-11-10)

- Native x86_64/i686 builds with -Wall -Wextra -Werror passed, including
  byte parity between the standalone assertion body and candidate unit.
- Full 62-entry series with v5 substituted applies at --fuzz=0. A focused
  feature-disabled Wine build links wineserver and both full user32 test
  EXEs. This is not a full graphical L0 or a runtime result.
- Literal tables, unchanged server sources and operation ordering checked
  against v4. Adversarial symbolic witness checks reject NULL, unrelated
  or unknown handles, disconnected A/C and accidental inclusion of B/D.
- Second-model read-only advisory found no concrete blocker or recognizable
  borrowed implementation in the bounded delta; it cannot prove originality
  and is not promotion cross review. It confirmed native exact assertions
  remain mandatory and ordinary membership cannot be bypassed by the todo.
- V5 native L1, paired L2, changed-test cross review and applicable L4 are
  pending. SOLIDWORKS L4 remains deferred. No runtime pass is claimed.

## Next run and native scheduling

The default scripts/build-l1-input-components.sh and batch4-measure/l1.list
select v5. Keep its native L1 for the next combined VM validation after
textfont-followup2 returns; do not request a separate VM round or rerun v4.
The existing main Build MEASURE bundle workflow accepts the list input.

The final diagnostic pair must install identical v5 tests on vanilla and
series, require complete component coverage, zero ordinary series errors
and a vanilla target signal, and capture actual failure contexts as well
as the histogram. A focused pass does not replace neighboring-suite L2.
If it fails, stop and record the remaining evidence before any further
adjustment; do not add broad todos or guess a server fix. No third paired
run has started yet. The unmeasured third-thread reparent history remains
an open point, outside this change.


## Recorded native and paired results (2026-11-10)

These results supersede the pre-run pending/scheduling notes above.
[Native v5 L1](../../../docs/wine-repro/server-input-attachment-components/l1-v5-2026-11-10/README.md)
passed on Windows 10 x86_64/i386: 16719 checks each, zero failures, todos,
flaky checks or skips, exit 0. All 44 scenarios and both complete scopes
(592 component and 592 absolute snapshots, state 1, 2180 positives) ran.
The corrected three returned files were read in full and privacy checked;
the previous batch1 attachments were not used as this candidate's L1.

[Focused paired L2](../../../docs/wine-repro/server-input-attachment-components/l2-v5-2026-11-10/README.md),
run 42772336963 at ee07735, finished green. Each series unit has 12039
checks, zero ordinary failures, 16 todos and full component coverage.
The single unavailable absolute-view scope is reported, not a scenario
skip or a native-state pass. Vanilla retains the target signal (290
ordinary i386 failures; the other count was not retrieved). Public
source notices, not downloaded original JSON, support these counters.

No further attachment VM rerun is needed for this body. Final changed-test
cross review, neighboring-suite L2 for v5, and applicable Inventor L4
remain pending; SOLIDWORKS L4 stays deferred. Candidate only: production
patches/series and its marker remain unchanged.

## L2: test-run/all-1 (2026-11-10)

[docs/wine-repro/l2-all-1-2026-11-10](../../../docs/wine-repro/l2-all-1-2026-11-10/README.md), run 42788810738, with every current candidate: `user32:attachment` 12039 checks, 16 todos, 0 failures, both architectures; no new failure in user32 and the other groups (neighbouring-suite L2). The one regression of the run is an ntdll i386 time check, unrelated.
