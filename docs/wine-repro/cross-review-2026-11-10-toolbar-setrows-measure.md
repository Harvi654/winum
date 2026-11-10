# Cross review: toolbar TB_SETROWS MEASURE (2026-11-10)

Reviewer: Codex; program author: Claude. Reviewed the child-process
revision at `13c9d85`, against the original `75eda83`. This reviews the
measurement instrument and its first native results, not a new setrows
implementation or approval of candidate v2 for release.

## Scope

Read in full: `comctl32-toolbar-setrows/measure.c`, its README and local
list, both files under `windows-2026-11-10-first-run/` containing native
output and their exitcodes, `batch1-measure/comctl-support.h`,
`batch3-measure/measure-support.h`, the original-to-current program diff,
the setrows v2 candidate README and the previous v2 cross review. Also
read the shared MEASURE builder and workflow in full and checked this
entry in the main bundle list.

Revisited complete `TOOLBAR_SetRows`, `TOOLBAR_WrapToolbar` and
`test_setrows`/`tbrows_results` in pinned Wine 11.20
`7298a818b79446bae153b8d5aa27235ab3703bf9`. The rest of those large source
files and the old candidate implementation were not re-audited in full;
their earlier review remains applicable. Only open source and text from
our own synthetic windows were inspected, with no vendor internals.

## Isolation and observations

The parent now enumerates 704 independent cases and four palette
sequences. Each child loads its requested comctl32 version, creates its
own parent, image list and toolbar, and exits before the next case starts.
Version 6 uses an owned manifest/activation context, with its version
printed; version 5 does not inherit that activation from a previous case.
The embedded execution manifest does not request common-controls v6.

`run_child` quotes the executable path, inherits the redirected standard
handles and prints a case identity before process creation. Child output
is unbuffered. The parent waits for process exit, or reports timeout and
terminates that child before continuing. Thus an ordinary synchronous
control hang cannot prevent the next case from being attempted. The
timeout covers child startup and setup as well as the measurement, not
just `TB_SETROWS`. Three seconds is an exploration budget, not a Windows
API timing requirement; a slow or starved VM can exceed it too.

The 64-button buffer accommodates the largest shape (47 buttons plus
four separators). Output string appends are bounded. Requests and
`bLarger` are packed into the correct low/high words. Row counts, returned
rectangle, wrap indices and layout are observations with no guessed
expected values. The palette sequence is explicitly only the known
synthetic subset, not an execution of Inventor. No mouse input, foreign
window inspection, installation or persistent registry changes are used.

## Findings

- **P1: none found in the bounded child-process change.** Fresh processes
  remove the single hung control from the remaining measurements without
  changing the case matrix, toolbar construction or palette operations.
  This is a useful way to obtain the missing v6 observations.
- **P2: child failures do not reach the parent's success/completion
  summary.** In `measure.c:270-281`, a returned child code of 3 only
  increments `TIMEOUTS`; other nonzero exits are not aggregated at all.
  Child `b3_errors` belongs to another process. The parent's `b3_finish`
  can therefore print `api_errors=0 complete=1` and return 0 despite a
  timed-out case, child API error or crash. `CHILD_END`/`CHILD_TIMEOUT`
  preserve the evidence in the full output, so individual completed
  cases remain usable, but the exitcodes file is insufficient to establish
  complete measurements. Distinguish enumeration completion from valid
  returned data: aggregate completed cases, timeouts, child errors and
  exceptions, and make missing results explicit in the summary/status.
  An observed Windows timeout can be recorded as a timeout outcome; it
  supplies no row/geometry oracle. Require a matching successful child
  and complete output before using a case in a Wine conformance test.
- **P2: the first-run hang is localized to a case, not an API.** Both old
  outputs stop after the same 198 cases. The next source-selected tuple
  is `v=5 vertwrap g3-4 rows=1 larger=0`. However, there is no pre-call
  marker: toolbar creation and `TB_ADDBUTTONSW` also precede the first
  `CASE` output. The README's definite attribution to `TB_SETROWS` goes
  beyond the recorded evidence. Add flushed stage markers around creation,
  button insertion and `TB_SETROWS` (and palette steps), or describe it
  as an unlocalized hang while measuring this case. A new `CHILD_BEGIN`
  identifies the tuple but alone still does not identify the hanging API.
  Do not turn the timeout into a Windows assertion about `TB_SETROWS`.
- **P3: coverage is useful but narrower than the inherited patch.** All
  buttons are visible and every separator is a single internal separator.
  Hidden buttons/separators, leading/trailing/consecutive separators and
  the last-hidden-button boundary from the earlier v2 review are absent.
  The palette replaces one button, not the full sequence of replacements.
  Also `layout` groups by item top and collapses separators to `s`/`S`;
  it does not retain each item's complete rectangle or separator extent.
  These limitations do not prevent this matrix from finding grouping
  differences, but it cannot close those earlier coverage points or prove
  separator stretching/resize geometry compatible with Windows. Measure
  the relevant additional cases/rectangles before changing those paths.
- **P3: the outer limit does not guarantee enumeration of all timeouts.**
  The maximum nominal child waits are `704 * 3 + 4 * 12 = 2160` seconds,
  before process overhead, but the parent watchdog is 1800 seconds. Many
  hanging/slow cases can still truncate the matrix. Retaining an outer
  safety bound is reasonable; describe such a run as incomplete, or
  budget the outer guard from the matrix plus overhead. `WAIT_FAILED`
  is also currently labeled timeout by the shared helper, and its
  termination result is unchecked. Distinguish wait errors from expired
  waits when tightening the failure accounting.

## What the returned Windows data establishes

Both architecture files have 201 lines: the header/system/version lines
plus exactly the first 198 matrix cases, in source order. They differ
only in `bits=64`/`bits=32`; all observations agree. Both exitcodes are 2,
consistent with the original 120-second watchdog. Neither has a final
summary, v6 data, nowrap controls or palette sequence output.

The completed v5 horizontal cases already demonstrate why group tests
are necessary. For `wrap` and `flatwrap`, the palette shape at request 12
returns `getrows=12` with layout `8 4s 8 2s 8 2s 8s 7` for both `bLarger`
values. That is eight top-coordinate groups and twelve reported rows;
do not assume the length of `layout` equals `TB_GETROWS`. At request 2,
FALSE returns one row of 47 buttons, while TRUE returns three reported
rows. These are measured v5 outcomes, not evidence for the still-missing
v6 vertical, zero-padding Inventor-shaped sequence. Existing native
`test_setrows` success does not validate separator handling either.

## Checks and verdict

Built the single-entry bundle with the repository builder for x86_64 and
i686, including its embedded manifest, with `-O2 -Wall -Wextra -Werror`:
both PE executables compile/link. Checked the native records against the
matrix prefix and compared both architecture files. No Windows or Wine
program was run in the cloud, and no new native result is claimed.

Verdict: **the isolation is suitable for continued native exploration;
address the P2 completion accounting and hang attribution before calling
the whole measurement complete or deriving an API-specific regression.**
Use only completed, error-free cases for expected values. The missing
v6/palette results and inherited coverage points remain open. This review
changes no measurement source, candidate, release marker or production
series.
