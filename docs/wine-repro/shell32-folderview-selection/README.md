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
