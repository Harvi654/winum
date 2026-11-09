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
was started on test-run/batch4-1 at 36b639e (2026-11-09), result pending.
The pair installs identical new test bytes in vanilla and series, while
only the series side receives the implementation. Both paired source trees
were strictly applied and checked against the full candidate before push.
The tests-series-g1 artifact includes a native user32:attachment L1 bundle. Its server-neighbor
coverage includes user32, win32u and gdi32. No maintainer desktop session
was used by the cloud agent.
