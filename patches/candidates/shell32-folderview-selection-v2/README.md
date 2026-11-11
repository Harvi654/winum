# Shell folder-view selection v2: Windows-derived functional test

> **Promoted 2026-11-11** into `patches/series` for winum-11.20-1. This folder stays as the record of the candidate.

Original, AI-assisted (Codex), LGPL-2.1-or-later. Candidate only; production
patch, test marker and patches/series are unchanged. Initial work was saved
as WIP in 0776d2c before completion of this candidate's documentation/checks.

## Native basis and changes from v1

Our own [MEASURE](../../../docs/wine-repro/shell32-folderview-selection/README.md)
completed on offline Windows 10 x86_64/i686 on 2026-11-09. Both outputs match
apart from bitness: four complete states, no API errors and host_refs=1.
Empty selection, B selected/focused, B/C selected and cleared selection
establish counts, identities, exclusive selected-index iteration and focus
flags. Empty selected Items and GetSelection(FALSE) return
HRESULT_FROM_WIN32(ERROR_NOT_FOUND), with NULL-initialized outputs remaining
NULL. GetSelection(TRUE) with no selected item returns one own-folder item.
The current production patch returns E_FAIL in the two empty-selection paths.

V2 corrects only those selected-empty HRESULTs. The unmeasured empty ALLVIEW
result remains E_FAIL; v1's NULL-output and invalid-PIDL branches are retained,
without new native assertions about them. The private collector now returns
status separately from count, initializes its outputs, distinguishes OOM
from an empty selection and rejects an incomplete traversal. This restores
allocation-error propagation lost in v1. Count/size overflow is checked.
The view owns the child PIDLs; the collector/caller frees only the outer
pointer vector. Array construction copies the item identities before that
vector is freed. The existing own-folder fallback retains its PIDL cleanup.

The collector is not an atomic snapshot against reentrant view mutations.
Its fixed-view functional test does not establish such concurrency behavior;
an underreported count could still describe only the earlier subset. No
race, OOM-injection, invalid-input or empty-directory coverage is claimed.

## Wine test

The new dlls/shell32/tests/selection.c unit uses our original IShellBrowser
host and public shell-view APIs over three own files. It does not automate
Explorer, enumerate the desktop's contents, access private ListView fields,
inject mouse input or use application data. Desktop IShellFolder only binds
the own absolute directory PIDL. All views, arrays, shell items, COM refs,
PIDLs, windows and own files are released on normal completion; abnormal
termination may leave only its own temporary fixture objects.

All target assertions are ordinary ok(), including the selected-empty error.
The test checks ItemCount/Items for ALLVIEW, SELECTION and selected VIEWORDER,
both GetSelection modes, GetSelectedItem starts -1/0/1/2 and each valid own
child PIDL's selection state. Array identity sets are checked, never the
unspecified default order. Real view indices are mapped to the own PIDLs,
so selected-index expectations do not presume an alphabetic sort order.
The cleared state requires B's measured retained focus while all selection
bits are clear. No NULL output-pointer or invalid-PIDL expectation is added.

Vanilla's missing selection methods/count handling and v1's E_FAIL empty
result are predicted failures; observed Wine before/after runtime remains
pending. A successful MEASURE is not counted as execution of these assertions.

## Native L1 bundle

```sh
bash scripts/build-l1-folder-selection.sh /path/to/NEW-output
```

The dedicated builder extracts the complete helper/test block from the
candidate and verifies exact equality with test-body.h before compiling
that unchanged body for x86_64/i686 using -Wall -Wextra -Werror. The common
original manifest requests asInvoker/Windows-10 execution. Sources, license
and SHA256SUMS accompany the bundle. No BEGIN/END markers are put in Wine
source. The main measurement list also contains folder-selection-l1.

Run run-measure.bat in a fresh offline Windows 10 directory. The two runs
briefly show their own view windows: leave mouse/keyboard/focus alone and
let the program close each window itself. Return
result-folder-selection-l1-x86_64.txt, result-folder-selection-l1-i686.txt
and exitcodes.txt even on failure. Returned native results are recorded below.
Complete successful execution has 357 checks, four states, zero failures
and exit 0. This is a static path count, not a native result. The
standalone process has an independent 45-second watchdog; a timeout exits 2
and is not a pass. Message pumping and readiness waits are bounded; the
fixture must identify exactly its three items before testing selection.

For one VM visit including the corrected offsets assertion body:

```sh
bash scripts/build-l1-folder-selection.sh /path/to/NEW-output docs/wine-repro/batch3-measure/selection-offsets-l1.list
```

The optional list must contain the exact folder-selection entry. If it
contains tdoffset-records-l1, the builder additionally verifies that test's
complete candidate/body parity before compiling either program. Both own
source sets and the execution manifest are hashed with all four EXEs. The
offsets sources are kept in a separate source subdirectory to avoid name
collisions. Return both pairs of result files and exitcodes.txt; revised
offsets success is 40095 checks, zero failures, while shell selection is
357 checks, zero failures and completed_states=4. Previous successful native
expectations are retained; native passes are recorded only from the returned
assertion results, not inferred from those expectations.

## Application and checks

Replace shell32-folderview-selection.patch at its existing full-series
position with this candidate. Do not apply both versions or edit
patches/series. The full 62-entry series applies with --fuzz=0 using the
candidate on upstream Wine 11.20 commit
7298a818b79446bae153b8d5aa27235ab3703bf9, archive SHA-256
9f516d63e9f814f4dad70ce0a1943211b8a6236008ac3025cf58004091c661db.
Both strict standalone builds and exact-body parity pass. Complete shell32.dll
and shell32_test.exe PE targets link for x86_64/i386, including the selection
unit. Configure used --enable-archs=x86_64,i386 --without-x --without-wayland
--without-freetype: a focused PE build, not a full graphical Wine L0 build.
Wine 11.20 lacks SHCIDS_CANONICALONLY in its headers, so the new test unit
locally defines the public constant with MinGW's identical 0x10000000 value.
The shared body and measured API input remain unchanged. Bundle checksums
and shell syntax checks pass; compilation is not native or Wine runtime.

A second-model read-only advisory checked the complete candidate, shared
test, harness, builder, own MEASURE and both full native outputs. It found
the four-state oracles and body parity aligned, borrowed/allocated ownership
and error paths sound, and no blocking issue. The known non-atomic collector
limit is recorded above. It found no conspicuous nontrivial external-code
resemblance beyond ordinary COM/Wine patterns and reuse of our own original
fixture. This is not an originality certificate, native L1 or formal cross
review for promotion.

Paired shell32 L2, observed vanilla/patched behavior, Claude's formal cross
review and consumer validation remain open. No production marker upgrade,
candidate promotion or series change is claimed.

## Windows exact-body assertion L1 (2026-11-09)

Both architecture results from the combined recipe available at 8400e60
pass on the maintainer's offline Windows 10 VM: x86_64 and i686 each report
SUMMARY checks=357 failures=0 completed_states=4 and exit 0. The candidate
and combined builder were completed in 6c87a74. All four functional states
ran, including the ordinary ERROR_NOT_FOUND empty-selection assertions,
selected/folder array identities, index iteration and focus checks.

This is native assertion L1 of the exact shared body, not just the earlier
MEASURE or the static expected count. No raw logs or Windows files are
committed. The fixed-view native pass does not expand coverage to invalid
inputs, concurrent view mutations, OOM injection or other unmeasured cases.
Wine runtime pairing and the review/consumer gates above remain open; the
production patch, missing production test marker and series are unchanged.

## L2: test-run/all-1 (2026-11-10)

[docs/wine-repro/l2-all-1-2026-11-10](../../../docs/wine-repro/l2-all-1-2026-11-10/README.md), run 42788810738, with every current candidate: `shell32:selection` 357 checks, 0 failures, both architectures (the first-state focus check holds on Wine); no new failure in the other shell32 units.
