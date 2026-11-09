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

Windows output is pending. Stop here for this patch: derive assertions
only from that output, then prepare/review a candidate and paired Wine
runtime checks. Production patch, page marker and `patches/series` stay
unchanged.
