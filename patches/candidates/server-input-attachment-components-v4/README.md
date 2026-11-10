# Server input attachment components v4: separate observations, round 2

Candidate only. Supersedes v3 for further testing; production patches/series,
the production patch and its marker are unchanged. Both server implementation
hunks are byte-identical to v2/v3. This revision changes the test only and
requires review plus new native L1 and paired L2.

## Evidence and decision

V3 reached all 44 scenarios without skips but retained 301 i386 / 305
x86_64 active/focus failures. The first is in the isolated graph's probe
of B, observer A, before any attachment exists: A becomes NULL rather
than retaining its own active/focus window. Original structured evidence
is in the investigation's l2-v3-2026-11-10 directory.

This is the maintainer's second option, **round 2 of at most three**:
separate component/API observations from exact unrelated inactive-queue
state. Do not revisit the classified unrelated mousemove/toolbar/urlmon/
ntdll v1 failures. Textfont's BSTR fix and new cycle assertions will be
batched only after the VM's textfont-followup2 result; no textfont test
round is started here.

## Two explicit observation scopes

The component/API view always runs. At every snapshot it selects the
current actor (or a live positive native reference after an actor exits),
checks that actor's exact native active/focus handles, and compares every
observer against those same known handles. Expected positive/negative
relations are read from the literal Windows row, not a computed graph or
observed Wine state. An unrelated queue's own retained window versus NULL
is immaterial to that relation. Capture remains a literal unconditional
check, as do all attach/detach results and errors. A NULL actor cannot
produce a false shared-input pass: the positive actor assertions fail.

All 592 native rows have a live positive reference. They contain 2180
positive comparisons; 322 rows include sharing beyond the reference
itself, covering groups as well as independent queues. Original graph
operations, native tables and callback order are unchanged. Relative
assertions use the same narrow direct-destruction-B todo as the original
absolute view; self-detach's error todo is unchanged. No additional
behavior is excused with a todo.

The absolute view remains in the Wine test too. The first no-edge
joined-probe-B control, before any attachment/detach, determines whether
A retains its own fields and foreground as on Windows. If this independent
prerequisite is absent, emit one explicit skip for the absolute-state
subtest only. All component/API scenarios and positive checks continue;
a failure after a real detach cannot disable them. When supported, run
all original absolute active/focus assertions as well. This is a runtime
prerequisite, not a compile-time removal of all foreground tests.

**Limitation:** with absolute_state=0, Wine does not validate an unrelated
inactive queue's retained window versus NULL. Report that missing scope
explicitly; do not call a component pass a pass of the full original state
matrix. Native L1 always runs the component predicates and must also run
every absolute checkpoint, so the actual Wine-side predicates are tested
on Windows too.

## Coverage and validation

- ATTACHMENT_COVERAGE: completed/skipped scenarios, total 44.
- ATTACHMENT_SCOPE: component_snapshots, absolute_snapshots,
  absolute_state and positive_checks. Complete component coverage means
  592 snapshots and 2180 positives. With the absolute prerequisite absent,
  the first three setup/control snapshots precede that decision; the
  remaining absolute checks are explicitly unavailable, not passed.
- Native wrapper requires 44 completed, no scenario skips, 592 component
  and 592 absolute snapshots, absolute_state=1, zero failures and exit 0.
  A missing absolute prerequisite is incomplete native L1 (exit 2).
- Wine component L2 requires full component coverage and no ordinary
  series failure, while vanilla must still demonstrate the target gap.
  A single absolute-subtest skip is reported separately from scenario
  coverage. Do not hide missing component work behind a success summary.
- The usual 12 Wine todos apply when the absolute view is unavailable;
  an enabled absolute view checks the same direct-destruction gap again.
  Counts describe the selected scope, not additional allowed behaviors.

Build checks (2026-11-10): all 62 entries apply with --fuzz=0; wineserver
and both full user32 test EXEs link in a focused feature-disabled build;
the same native body builds on x86_64/i686 with -Wall -Wextra -Werror and
is checked for byte parity by the L1 builder. No cloud Wine execution or
full graphical L0/runtime pass is claimed.

Second-model read-only advisory found no concrete blocker or recognizable
copied code in the bounded comparison (not proof of originality). It
independently checked positive references and scope preservation. This
is not formal promotion cross review. Native v4 L1, paired v4 L2, changed-
test review and applicable L4 remain pending; SOLIDWORKS L4 is deferred.

## Native bundle and scheduling

The default scripts/build-l1-input-components.sh and
batch4-measure/l1.list now select v4 sources; previous bodies remain
under their original paths. The main Build MEASURE bundle workflow accepts
that list input; no batch-specific workflow is added.

```bash
export INPUT_L1="$HOME/.cache/winum-input-v4-l1-$(date +%Y%m%d-%H%M%S)"
cd "$HOME/winum"
git pull --ff-only
bash scripts/build-l1-input-components.sh "$INPUT_L1"
```

Native validation of this changed body is mandatory. Include it in the
next combined Windows round after textfont-followup2 is returned, rather
than requesting another run of obsolete v3. Require both coverage scopes
and zero failures/skips; the earlier v2 L1 is not reused. The paired CI
run's Wine-unit L1 bundle is an alternative to the standalone wrapper.

The unmeasured third-thread reparent history remains outside this change.
If component checks still require unreliable foreground behavior, inspect
the actual round-2 results before choosing the maintainer's third option.
Do not claim the problem solved or consume the last round on a guess.


## Focused round-2 outcome (2026-11-10)

[Run 42724139459](https://github.com/Harvi654/winum/actions/runs/42724139459)
finished with 12 ordinary series failures on each architecture, compared
with 302 on vanilla. All 44 scenarios and 592 component snapshots ran;
there were zero scenario skips. Each variant reports one unavailable
absolute-state scope, as designed. L2 remains blocked. Representative
series failures concern children-late1-unparent0 / one-child; obtain the
full failure list before deciding the final adjustment. Counters and
limits are recorded in
[the result note](../../../docs/wine-repro/server-input-attachment-components/l2-v4-2026-11-10/README.md).
Native v4 L1 remains pending; no third round has been started.
