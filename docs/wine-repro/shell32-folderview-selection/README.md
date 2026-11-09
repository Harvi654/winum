# Shell folder view selection: Windows MEASURE

Original, AI-assisted (Codex), LGPL-2.1-or-later. Native HRESULTs, empty-array
contracts, flags, identities and selected-item iteration semantics are unknown.
The production patch/page and relevant public Wine shell view creation,
selection and test-host code were read. No production implementation, marker
or series changes.

The fixture creates a unique own temporary directory and three fixed files
(aa.fixture, bb.fixture, cc.fixture). Public IShellFolder/SHCreateShellFolderView
APIs create a standard shell view inside our own visible window. An original
minimal IShellBrowser supplies that window, reference counting and ordinary
host callbacks; unsupported optional services return E_NOTIMPL/E_NOINTERFACE.
It does not automate the user's Explorer, enumerate the desktop or use a
vendor file dialog. Desktop IShellFolder is used only to bind our absolute
directory PIDL; actual enumeration is confined to the own directory.

The view pumps messages for at most ten seconds while waiting for the three
items. It maps actual indices to our relative child PIDLs via CompareIDs,
without relying on sorting. It then requests empty, one-selected (B with
focus), two-selected (B/C), and cleared states. All selection changes use
valid indices and public IFolderView2::SelectItem; there are no injected
mouse events, internal ListView pointers or private shell fields.

For each state, output records:

- ItemCount and Items(IShellItemArray) for all/selected/selected view-order;
- GetSelection with none_implies_folder false and true;
- GetSelectedItem starting at -1, 0, 1 and 2;
- GetSelectionState for each of our three valid child PIDLs.

Array outputs include counts and comparisons of public parsing names to our
fixed file names/own folder leaf, never the names or paths themselves. A
returned array is released even on an unexpected HRESULT. Enumeration is
bounded at eight array items. These are measurements, not assumed S_OK,
S_FALSE/E_FAIL or selection flag contracts. Invalid PIDLs/NULL output
parameters are outside this first functional matrix.

Require READY with exactly three identified own items, successful setup and
selection APIs, all four STATE_END records and complete SUMMARY before
deriving expectations. Inspect returned identities/counts to establish the
requested selections; an S_OK setter alone is not proof of correct content.
A 45-second independent process watchdog covers shell initialization,
pumping, observation and destruction. COM/PIDLs/arrays are released, the
view deactivated/destroyed before its host, and only own files removed.
Host refs are reported rather than guessed as native results. Abnormal exit
may leave own files; it is not a pass.

The folder-selection entry is in both bundle lists. Both architectures build
with -Wall -Wextra -Werror. Run in the final batch-3 VM visit without moving
focus or clicking the fixture window; return both result-folder-selection-*.txt
and exitcodes.txt, including incomplete output. No proprietary data or
native internals are inspected. A requested second-model advisory in this
batch was unavailable because of its usage limit; no independent review is
claimed for this source. Native assertion L1 and runtime gates remain open;
raw logs will not be committed.

## Windows observations (2026-11-09)

The two architectures match apart from bitness, exit 0, api_errors=0. All
three own files are identified at initial indices A=0/B=1/C=2, all four
states complete, and host_refs=1 after normal destruction. Setup and selection
API calls return S_OK. Observations of the queried methods are:

| State | All / selected counts | Items(selected), GetSelection(FALSE) | GetSelection(TRUE) |
| --- | --- | --- | --- |
| empty | 3 / 0 | 0x80070490, NULL array | S_OK, one own-folder item |
| one (B) | 3 / 1 | S_OK, one B item | S_OK, one B item |
| two (B/C) | 3 / 2 | S_OK, B and C | S_OK, B and C |
| cleared | 3 / 0 | 0x80070490, NULL array | S_OK, one own-folder item |

0x80070490 is HRESULT_FROM_WIN32(ERROR_NOT_FOUND), not E_FAIL. Selected
Items and ItemCount with SVGIO_FLAG_VIEWORDER give the same measured counts
and selected identities. All-view arrays contain all three own items; their
observed default order changes from A/B/C to B/C/A after selection. Do not
assert fixed ordering for SVGIO_ALLVIEW without an ordering flag or use that
array order as a substitute for the actual view indices.

GetSelectedItem uses an exclusive starting index in these observations:
empty/cleared return S_FALSE and -1 for each tested start. With B selected,
starts -1/0 return S_OK/index 1, while starts 1/2 return S_FALSE/-1. With B/C,
starts -1/0 return 1, start 1 returns 2, start 2 returns S_FALSE/-1.
GetSelectionState returns S_OK throughout: initially all 0; in the one state
B=0x11, in the two state B=0x11/C=1; after clearing, B retains focus (0x10)
while all selected bits are clear.

The production selection/count implementation is only partially conforming:
its empty selected Items and GetSelection(FALSE) return E_FAIL, which differs
from the measured ERROR_NOT_FOUND HRESULT. A future candidate must retain
these native expectations rather than hiding them or assuming the production
patch already passes. NULL-output and invalid-PIDL behavior were not measured
and must not acquire guessed assertions. This is a functional MEASURE, not
native assertion L1; production code/series/marker remain unchanged. Raw
logs are summarized rather than committed.

## Windows-derived assertion candidate

The [v2 candidate](../../../patches/candidates/shell32-folderview-selection-v2/README.md)
corrects the measured empty selected Items/GetSelection(FALSE) result to
ERROR_NOT_FOUND and adds a Wine selection.c test. Its exact shared body
checks all four measured states and public methods. Array membership uses
identity sets, and selected-index expectations use mapped actual view indices;
no unspecified default order or invalid-input contract is asserted.

scripts/build-l1-folder-selection.sh verifies the full helper/test block
before compiling standalone x86_64/i686 assertion executables. Their native
L1 results are recorded below; the original MEASURE is not an assertion pass. The
view opens and closes itself, so do not click or change focus while it runs.
Require four completed states, zero failures and exit 0 on both architectures.
Both strict builds and test-body parity pass. The candidate README records
ownership, error handling and remaining runtime/review gates. Production
page/marker/series remain unchanged until the appropriate gates pass.
The statically counted complete success path has 357 assertions. The optional combined list
batch3-measure/selection-offsets-l1.list also builds the corrected offsets
body for one VM run, after checking both candidates' exact-body parity.

## Windows exact-body assertion L1 (2026-11-09)

The maintainer's offline Windows 10 results complete on both architectures:
357 checks, zero failures, four completed states and exit 0 each. The
combined recipe available at 8400e60 verifies both test bodies; shell
candidate/builder completion is 6c87a74. Empty selected arrays, own-folder
fallback, selected identities/counts, exclusive index iteration and focus
all pass as ordinary assertions. Native L1 is now established for this
functional matrix, not inferred from the original MEASURE.

Invalid inputs, concurrent mutations and OOM behavior were not newly
measured by this run. Observed Wine before/after behavior, paired shell32
L2, formal cross review and consumer validation remain pending. Production
patch/marker/series stay unchanged. Only summarized text is committed.
