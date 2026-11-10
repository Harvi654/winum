# Input attachment components: Windows MEASURE

Original, AI-assisted (Codex), LGPL-2.1-or-later. No proprietary windows,
files or binaries are used. This is a measurement program, **not a Wine
conformance test**, and contains no expected Windows graph/state values.

The production patch changes the server's removal of one input attachment:
keep alternative paths connected, split complete components rather than
only one queue, preserve the active component's original input object and
restore the edge on allocation failure. Detach-all and queue destruction
are neighboring paths, not changed by this patch.

Read before writing: `patches/server-input-attachment-components.patch`,
its patch page, Wine 11.20 server/queue.c attachment/assignment handlers,
the existing user32 input attachment tests, and the project's original
cross-process focus probe/graph shim referenced by the patch page. This
new harness uses public APIs rather than the shim's internal model as an
oracle. The former probe's early/late focus and explicit unparent versus
direct destruction distinction is retained in the child cases.

## Matrix

Four fresh GUI worker threads A/B/C/D own visible top-level windows. The
controller serializes operations through bounded SendMessageTimeout calls;
workers keep pumping messages. Every graph is recreated, with no explicit
attachments left over from the previous case.

Each of ten graphs runs in both detach directions with two active-window
anchors (40 cases):

| Graph | Operation / control |
| --- | --- |
| isolated nonedge | detach A-B with no edge |
| pair | attach then detach A-B, active A or B |
| chain | A-B-C, detach B-C, active A or C |
| bridge | A-B-C-D, detach B-C, active A or D |
| cycle | A-B-C-A, remove A-B, then B-C |
| duplicate | attach A-B twice, remove it three times |
| opposite edges | attach A-B and B-A, remove each |
| connected nonedge | A-B-C, attempt to remove nonexistent direct A-C |
| self detach | A-B, attempt to remove A-A |
| queue-exit neighbor | A-B-C, exit C; active A or C |

Snapshot all live workers after joining and immediately after each detach,
**before requesting any replacement focus**. Capture is set on the anchor
before the cut, observed passively, then released. Controlled focus probes
on each live thread's own window follow, with another all-thread snapshot.
This separates preservation of existing state from the ability to share
new focus. Foreground-window role is always recorded: if foreground
activation was denied, that observation cannot prove foreground routing.

Four more cases create two children owned by C under A and B respectively,
using the resulting implicit cross-thread attachments. Focus child-A early
or only after creating child-B. Either unparent child-B with SetParent(NULL)
before destroying it or destroy it directly. Record passive and refocused
state at each step. D is an unrelated-thread control. These four cases are
same-process; they do not claim cross-process WebView2 or Chromium coverage.

Not measured: forced allocation failure (no public API for the server's
allocator), another desktop, a thread without a GUI queue, real keyboard
delivery, focus inside WebView2 or any CAD consumer. The exit neighbor is
an observation of unchanged queue-destruction behavior, not a proposed fix.
With anchor C, its own active/focus window is destroyed too; interpret that
separately from the anchor-A case.

## Build only this batch

Official MinGW-w64, both architectures; the list supplies `-Wextra` in
addition to the builder's `-Wall -Werror`:

```bash
export BATCH4_MEASURE="$HOME/.cache/winum-batch4-$(date +%Y%m%d-%H%M%S)"
cd "$HOME/winum"
git pull
bash scripts/build-measure-bundle.sh "$BATCH4_MEASURE" docs/wine-repro/batch4-measure/measure.list
```

The dedicated **Build batch 4 MEASURE bundle** workflow builds this list
automatically when this source/list/workflow changes on main. It also has
workflow_dispatch. The new source is named input-components.c; the general bundle workflow's
measure*.c/l1*.c trigger does not match it, so this change does not build
all previous batches too. The general workflow itself is unchanged. This separate automatic job also makes the artifact available
when the cloud cannot call api.github.com to dispatch the general job.

## Offline Windows 10 run

Download artifact `batch4-measure-bundle`, extract it to a fresh directory
in the VM and run `run-measure.bat` as the ordinary user. Leave mouse and
keyboard alone until it finishes. No administrator or compatibility mode
is requested. Return only these three text files:

- `result-input-components-x86_64.txt`
- `result-input-components-i686.txt`
- `exitcodes.txt`

Output records only own roles, numeric return/error values and state:
`CASE`, `EDGE`, `FOREGROUND`, `ACTIVE`, `API`, `STATE`, `END`, `MATRIX`,
`SUMMARY` and any `ERROR`.
The last-error sentinel is deadbeef; a NULL previous window is not itself
treated as an API failure. Last error has failure meaning only where the
API defines it; unchanged sentinel after success is not a failure.
`API operation_ok` is the observed local focus/active/capture match for
operations 0/1/2/5 and the raw Boolean/handle-presence result for the others.
It is not an invented Boolean return from SetFocus/SetCapture. `previous`
records their actual return handle as a role. Activation prints separate
foreground and active subcall results. Transport or required child-operation
failure aborts that case; focus/activation/capture outcomes remain data. Rejected detach/nonedge/self calls are measured,
not assigned an expected result. Success exit 0 means the harness finished
without setup/observation errors, **not** that a graph implementation passed
an assumed model. A watchdog or worker timeout exits 2. A complete run has
44 returned and 44 complete scenarios, paired CASE/END complete=1 records
and no ERROR lines. A returned but incomplete case is not silently counted
as a completed measurement.

## Validation and next gate

2026-11-09: compiled both architectures with `-Wall -Wextra -Werror`, using
the batch-only bundle builder. No Wine runtime was executed in cloud.
The source and matrix received a second-model advisory under AGENTS.md's
larger-original-code rule. It found no recognizable copied code in this
bounded comparison (not proof of originality); its command-result and
completion-status suggestions were applied before the final strict build.
That advisory is not a native correctness result.

The MEASURE-only stage ended when the maintainer returned Windows output;
the follow-up below records it and the candidate. Production patch, page
marker and `patches/series` stay unchanged.

## Returned Windows output and v2 assertion candidate (2026-11-09)

`windows-2026-11-09/` contains the original structured results and exit
codes from run 42659893206 after a full schema/privacy check. No user or
machine names, paths or e-mails were found. Both architectures have 44
complete scenarios, zero harness errors and exit 0; their content is
identical apart from bits=32/64. The accompanying README and SHA256SUMS
record provenance and exact files.

The Wine assertion body copies 592 observed state checkpoints (2352 live
thread snapshots) and 52 detach result/error records literally. The test
does not derive its oracle from Wine or a model of input components.
`patches/candidates/server-input-attachment-components-v2/` adds the unit
`user32:attachment`, keeps the explicit component walk and preserves the
old association when a child is moved to the desktop. The latter is
required by the observation that SetParent(NULL) leaves A/B/C sharing
input; v1 would newly clear B's active state even though vanilla already
has its measured active A. Direct destruction differs on Windows and
remains a narrow separate Wine todo for B, not for the surviving A/C pair.
Self-detach's existing error-code difference is another narrow todo.

All 62 series entries with v2 substituted applied using --fuzz=0 in a fresh
pinned Wine 11.20 tree. The resulting changed files match the compiled
tree byte for byte. A focused feature-disabled build linked wineserver
and the full user32 test executable for x86_64 and i386. It is not a full
graphical L0 or a Wine runtime pass. The standalone assertion body builds
with -Wall -Wextra -Werror on both PE architectures; the native builder
checks byte parity with the candidate and hashes its output.

```bash
export INPUT_L1="$HOME/.cache/winum-input-l1-$(date +%Y%m%d-%H%M%S)"
cd "$HOME/winum"
git pull
bash scripts/build-l1-input-components.sh "$INPUT_L1"
```

Alternatively the main Build MEASURE bundle workflow accepts
`docs/wine-repro/batch4-measure/l1.list` as its list input. The old
build-batch4-measure.yml is unchanged. The maintainer subsequently returned
Wine user32:attachment L1 results from the run's tests-series-g1 bundle:
**10195 checks, 0 failures, 0 todos/flaky/skips and exit 0 on both x86_64
and i386**. The ordinary completion check also passed for all 44 scenarios.
See `l1-2026-11-09/` for privacy-checked original summaries, exit codes,
hashes and provenance. This is the candidate's native assertion pass;
the standalone wrapper has not separately run. The earlier MEASURE exit 0
is not used as an assertion result. [L2 run 42695491325](https://github.com/Harvi654/winum/actions/runs/42695491325)
completed on test-run/batch4-1 at 36b639e (2026-11-09) with overall failure:
compare exited 1 after all ten matrix jobs completed. The returned report
and both g1 JSON summaries confirm that series user32:attachment finishes
but fails 2377 ordinary active/focus checks on i386 and 2370 on x86_64;
vanilla fails 2569/2561. Both execute 10195 checks with the same 12 expected
todos, no skips/flaky checks, and exit 255. Capture failures drop from 96
to zero on both architectures. The earliest active/focus mismatch is
already in isolated observer D before explicit attachments, in both
variants; initial activation/foreground conditions need investigation.
The neighboring i386 win unit also adds four mouse-message failures.
See l2-2026-11-09/ for privacy-checked original summaries, absolute counts
and next diagnostic. L2 is not cleared; native Windows L1 remains passed.
The candidate README records the result, retrieval limit and Claude's
conditional cross-review approval (d9119b4), including both P3 open points.
The pair installs identical new test bytes in vanilla and series, while
only the series side receives the implementation. Both paired source trees
were strictly applied and checked against the full candidate before push.
The tests-series-g1 artifact includes a native user32:attachment L1 bundle. Its server-neighbor
coverage includes user32, win32u and gdi32. No maintainer desktop session
was used by the cloud agent.


## V3 foreground setup, round 1 (2026-11-09)

The maintainer classifies the unrelated batch4-1 failures as existing
production v1 issues; see the L2 directory's attribution note. The current
blocker is attachment's unreached starting state. The separate
[v3 candidate](../../../patches/candidates/server-input-attachment-components-v3/README.md)
adds bounded setup and an explicit skip only before any edge/child exists.
The server implementation, literal Windows tables and todos are unchanged.
Post-attachment activation remains one-shot; failures remain hard.

Original v2 sources and evidence are retained; revised sources are in v3/.
The default native builder/list now target v3. Native v3 L1 and paired L2
must be rerun. The coverage line separates completed and skipped scenarios;
44 completed and zero skipped are required before claiming validation,
including CI. The standalone wrapper fails on any skip. No v2 pass
transfers to v3.

Focused round-1 pair:
[run 42713756358](https://github.com/Harvi654/winum/actions/runs/42713756358),
test-run/batch4-2 at 83d7025, based on main 7f5fc01. Both source variants
were strictly applied before push; test bytes match the final compiled
v3 body exactly, and only series has the implementation. The branch-only
workflow runs attachment on both PE architectures, rejects incomplete
coverage/ordinary series failures and requires a vanilla target signal.
Its coverage gate was checked against complete, all-skipped and ordinary-
failure samples without executing Wine.

Result pending. Artifact tests-series-g1 includes l1-bundle/run-tests.bat
for the revised native Wine unit. Extract to a fresh Windows VM directory
and return the two result-user32-attachment-*.txt files plus exitcodes.txt;
require ATTACHMENT_COVERAGE completed=44 skipped=0 attempted=44 and no
failures/skips. The CI artifact roots additionally contain
attachment-coverage-series.json / attachment-coverage-vanilla.json and the
usual results-*-g1.json for assessing Linux completion separately.

### Round-1 status checked 2026-11-10

After the requested toolbar/textfont cross reviews, the focused batch4-2
run was still in progress (one of two matrix jobs complete). The series
job 129489804779 completed its build/test/bundle step, then failed
"Require complete attachment coverage and inspect absolute result" with
exit 1. The standard summary alone does not identify which coverage or
absolute-result condition failed.

[tests-series-g1](https://github.com/Harvi654/winum/actions/runs/42713756358/artifacts/13718849558)
is available. Its root attachment-coverage-series.json contains both
architectures' completion/skip counts and absolute test results; that is
the next required input to distinguish an unreached prerequisite from
post-setup failures before changing the test again. The native v3 L1
bundle is in the same artifact. No second setup round or new unrelated
patch investigation was started on the basis of the aggregate failure.


### Round-1 coverage received (2026-11-10)

The returned attachment-coverage-series.json is recorded in
l2-v3-2026-11-10/. Both architectures completed all 44 scenarios with zero
scenario skips: 10235 checks, 12 expected todos, ordinary failures 301
(i386) / 305 (x86_64). The first failure is the isolated graph's local
focus probe of B, with observer A becoming NULL rather than retaining its
own active/focus window. Thus preparation now succeeds; the remaining
independent-queue state difference calls for the maintainer's second
option, separating component/API checks from absolute inactive-queue state.


## V4 scope separation, round 2 (2026-11-10)

[V4](../../../patches/candidates/server-input-attachment-components-v4/README.md)
always checks component relations against a positive live native actor,
literal capture and the measured API/error results. Exact unrelated
inactive-queue state is a separate runtime view gated by the first no-edge
probe-B control before any attachment. Failure of that control skips only
the absolute subtest; all 44 component scenarios still run, including
post-detach positive/negative membership. No target failure becomes a skip.

The native wrapper requires both views over all 592 checkpoints; the
component predicates therefore execute on Windows as well as Wine.
ATTACHMENT_SCOPE makes a missing absolute view explicit. Under Wine with
absolute_state=0, unrelated inactive window-versus-NULL retention is not
validated there, even if the component view passes. Original tables,
server hunks, graph operations and behavioral todo scope are unchanged.
All 592 rows have positive references, with 322 sharing beyond the actor.

Both PE native builds pass -Werror; all 62 patches strictly apply, and
both user32 EXEs plus the server link in the focused source build. No
runtime pass is claimed. V4 native L1 and paired L2 remain pending. Keep
its new native test for the combined VM round after textfont-followup2,
rather than rerunning v3 now. Previous sources/results are retained.


Focused round-2 pair:
[run 42724139459](https://github.com/Harvi654/winum/actions/runs/42724139459),
test-run/batch4-3 at 2d2fc9d, based on main b75a5e1. The run is in
progress; no runtime result is recorded yet. Both source variants contain
identical v4 test bytes, while only series receives the implementation.
The branch-only workflow runs attachment on both PE architectures. This
focused diagnostic does not replace the wider neighboring-suite L2.

The coverage gate requires 44 completed scenarios, zero scenario skips,
592 component snapshots and 2180 positive-reference checks. An available
absolute view must cover all 592 snapshots; the independent-control skip
is accepted only with its explicit unavailable state and three preceding
absolute snapshots. Series must have no ordinary failures; vanilla must
retain a target signal. The artifact roots contain
attachment-coverage-series.json / attachment-coverage-vanilla.json. Public
job notices also expose the filtered per-architecture counters and up to
three representative failures, so the scope can be assessed without API
artifact access. None of these checks substitutes for native v4 L1.

The series artifact also packages the revised native user32:attachment
unit. Hold that VM rerun for the next combined round after the pending
textfont-followup2 measurement results; do not rerun the obsolete v3
bundle. The textfont BSTR fix has not been added to a new run.


Round 2 has finished: [recorded counters and limits](l2-v4-2026-11-10/README.md).
Both architectures complete all 44 scenarios, with 12 ordinary series
failures versus 302 vanilla. The absolute-state scope is unavailable on
both variants. L2 is not cleared. The complete series coverage JSON is
needed to classify the remaining twelve failures before the final round;
three public examples point to the child reparent scenario, not proof
that all failures have that cause. No new VM or third runtime round is
requested yet.
