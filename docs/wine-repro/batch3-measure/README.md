# Batch 3: native observations before Wine assertions

Original measurements, AI-assisted (Codex), LGPL-2.1-or-later. Programs are
prepared sequentially, one patch per commit, and added to both measure.list
here and the main docs/wine-repro/measure-bundle.list. Native expected values
remain unknown until the maintainer returns the offline Windows outputs.
No production implementation, validation marker or patches/series changes
are made by preparing a measurement.

Current entries:

- msi-null-stream: original compound-storage transform with a NULL OBJECT
  row and populated binary control; [scope](../msi-transform-null-stream/README.md).
- msi-tree-property: own MSI dialog with direct-directory/indirect browse
  bindings and own-process tree automation; [scope](../msi-selection-tree-directory-property/README.md).
- mspatcha-signature: original raw/PE32 signature and normalization subset;
  PA19 decoder coverage remains blocked without an original producer,
  [limits and L3 proposal](../mspatcha-pa19/README.md).
- timezone: all new WMI timezone fields and their relation to the current
  Win32 snapshot, [scope](../wbemprox-timezone/README.md).
- http-same-seek: own loopback server and 12 framing/seek observations,
  [scope](../wininet-http-same-position-seek/README.md).
- folder-selection: standard shell view over three own files and four
  selection states, [scope](../shell32-folderview-selection/README.md).

Build one final bundle after all batch entries are prepared:

```sh
export BATCH3_MEASURE="$HOME/.cache/winum-batch3-$(date +%Y%m%d-%H%M%S)"
bash scripts/build-measure-bundle.sh "$BATCH3_MEASURE" docs/wine-repro/batch3-measure/measure.list
```

Copy the complete output directory to a fresh directory in offline Windows
10 and run run-measure.bat once. Return all result-*.txt and exitcodes.txt,
including failed/partial processes. A ZIP with only these text files is fine.
No vendor MSI, patch, DLL or application is used. Do not copy Windows files
back. Unknown API results are observations, not guessed assertions or L1.

The shared measure-support.h bounds each process with an independent
watchdog, reports setup/cleanup errors, and confines file writes to unique
own temporary directories. A timeout exits 2 without requiring stdio or
DLL shutdown on a possibly stuck thread; it need not print SUMMARY.
Successful observations print SUMMARY complete=1 expectations=none. Each
topic page also specifies which positive controls are necessary before
treating that completed output as usable measurement coverage.

## Final bundle preparation (2026-11-09)

All six entries build for x86_64 and i686 with -Wall -Wextra -Werror, and
the 12-executable bundle's SHA256SUMS verify. These are compile checks, not
native runs or L1 passes. The MSI and shell fixtures briefly show their own
windows; leave mouse/keyboard alone until the batch finishes. HTTP needs
only loopback access, WMI only the local service. No internet or application
installation is needed in the offline Windows VM.

Native outputs are the next dependency before writing Wine assertions or
revising production patches. The mspatcha entry covers only public signature
and normalization APIs, explicitly not PA19 decoding or old-reference-window
behavior; its original patch-input producer and proposed L3 remain a separate
gap. No exception is granted by this bundle. The maintainer's rpcrt4 decision,
files and candidates-4 test branch were left untouched.

## Initial Windows bundle returned (2026-11-09)

The returned architecture pairs match apart from bitness for MSI NULL
streams, timezone, HTTP and folder selection. The MSI tree pair matches
in failure: SQL error 1615 before the dialog. Mspatcha has usable x86_64
output, but no usable i686 text despite its recorded exit 0; the screenshot
shows Program Compatibility Assistant and a zero-length file. These are
MEASURE observations, not blanket L1 passes:

| Topic | Native observation / next gate |
| --- | --- |
| MSI NULL stream | Accepted transform, both rows retained, NULL read error 13/zero bytes and exact positive blob; first candidate now needs exact-body L1 |
| MSI tree | First INSERT rejected; revision 2 adds explicit column lists and needs new UI/property observations |
| mspatcha | Five x64 signature/normalization observations; i686 text incomplete and decoder coverage still absent |
| timezone | Native Bias sign, StandardBias CIM type and nonempty labels differ from production; test must preserve native values |
| HTTP | Cached Content-Length same-position seeks succeed; other tested seeks fail without corrupting bodies or adding requests |
| folder selection | Complete state matrix; native empty selection uses ERROR_NOT_FOUND, while production returns E_FAIL |

Each topic README records the bounded results. Raw logs are not committed.
Work remains one patch at a time: first complete native assertion L1 for the
MSI NULL-stream candidate, then continue candidate work on the remaining
measured contracts. No existing observation is changed to match a patch.

## Targeted follow-up: one assertion L1 and two incomplete measurements

Only three entries need this VM visit; the successful timezone/HTTP/shell
MEASUREs need not be repeated:

```sh
export BATCH3_FOLLOWUP="$HOME/.cache/winum-batch3-followup-$(date +%Y%m%d-%H%M%S)"
bash scripts/build-l1-msi-null-stream.sh "$BATCH3_FOLLOWUP" docs/wine-repro/batch3-measure/followup.list
```

The dedicated builder checks the entire candidate test block against the
shared native source, requires the exact MSI L1 entry in the optional list,
and builds all six EXEs. They embed an original asInvoker/Windows-10
execution manifest. Resource XML and both PE resource tables are checked;
compilation/metadata alone do not prove a cause for the earlier empty stdout.
The declared manifest requests normal caller execution rather than a
compatibility-mode or administrative relaunch.

Copy the whole output into a new Windows folder and run run-measure.bat.
Return all six result-*.txt and exitcodes.txt, even if empty or incomplete.
The first assertion test should complete 43 checks with zero failures/exit 0.
The two other entries remain MEASUREs: MSI must reach both own-dialog
selection states, and mspatcha must print its MEASURE header and all five
cases on each architecture before native coverage can be accepted. Do not
choose a compatibility-mode reinstall if the compatibility assistant appears.
The installed-correctly response already chosen does not invalidate prior
text observations or establish an API result for an empty file.

## Follow-up outcome and MSI UI stop (2026-11-09)

The NULL-stream candidate's exact-body assertion L1 passed on Windows 10
x86_64/i686: 43 checks, zero failures and exit 0 each. Mspatcha's targeted
repeat now has complete identical two-architecture observations for its
signature/normalization subset; PA19 decoding remains uncovered.

The MSI UI fixture reached costing in revision 2 but SHOWDIALOG returned -1
and the process timed out. Revision 3's named-dialog action returns 0 for
both cases on both architectures and the processes complete, but discovery
and child selection both report false. The maintainer dismissed the visible
dialogs. Both processes exit 1 with two automation errors; the unchanged
properties are not a selection oracle. Stop that fixture after its third
native iteration, retain the evidence and hypothesis in the topic README,
and continue other batch entries. No fourth rerun or L1 upgrade is requested.

The next independently measurable candidate is the mspatcha signature API
subset. Its original Wine signature.c test and exact-body standalone wrapper
cover the five native cases, with 50 checks on complete success. Build it
through scripts/build-l1-mspatcha-signature.sh before native assertion L1.
Both strict standalone architectures and the full-series substituted DLL/test
PE targets build, but runtime is pending. This does not close the original
PA19 producer/decoder gap or grant full-patch L1; see the candidate's scope.

The returned mspatcha signature assertion bundle passes all 50 checks on
Windows 10 x86_64/i686, zero failures and exit 0 each (c50734d sources). Its
summary retains pa19_decode_covered=0. The API subset has native L1; full
decoder validation, candidate cross review and Wine runtime pairing remain
open. Continue with the timezone entry; the stopped MSI UI fixture stays
stopped and no production series entry is changed.

The next timezone follow-up measures current display-label sources through
read-only public registry/localization APIs and compares them with WMI
without printing text. It also reports competing Bias relationships, whose
values may coincide in the current timezone. The initial measurements alone
do not establish those label/source contracts. Build just timezone-labels
through timezone-labels.list for one targeted VM run before choosing a fix
or new expected values; the main measure list includes the same entry.
No timezone setting is changed and no candidate/L1 claim is made yet.

The returned label follow-up now establishes exact raw Display equality
for Caption/Description on both architectures; MUI resolution failed in the
sample, and the competing Bias models remain equal in standard time with
zero standard adjustment. The separate timezone v2 candidate corrects the
measured differences and adds an exact-body native assertion bundle through
scripts/build-l1-timezone.sh. Require zero failures and zero skips in the
measured setup before claiming its L1. Unknown seasonal/localized/custom
cases are documented, not silently inferred. The production marker/series
remain unchanged.

The timezone candidate's exact-body assertion bundle at 34f65fe now passes
on Windows 10 x86_64/i686: 104 checks, zero failures, zero skips and exit 0
each. The documented standard-time value assertions all ran. Broader
timezone modes, paired Wine runtime, cross review and promotion remain
open. Continue with the HTTP same-position seek entry; production series
and markers are unchanged.

The next HTTP candidate keeps its production one-line buffer-preservation
fix unchanged and adds all 12 measured HTTP cases to Wine's http.c test.
Build the exact standalone body through scripts/build-l1-http-same-seek.sh.
Full native success requires 200 checks, zero failures, 12 cases and 12
server-observed requests. Target/body/count assertions are ordinary; only
the independent Wine cached chunked/close seek-rejection gap has Wine todos.
Native assertion L1, runtime pairing and review are pending; no production
series or marker change is made.

The HTTP exact-body native assertion bundle prepared at 96d3ca3 now passes
on Windows 10 x86_64/i686: 200 checks, zero failures, 12 cases, 12 observed
requests and exit 0 each. Wine-only todos are ordinary in the native wrapper,
so all controls ran. Runtime pairing, review and promotion remain pending.
Continue with folder-view selection, the final entry of this batch.

The shell selection v2 candidate now corrects the measured empty-selection
HRESULT and adds an exact-body native assertion bundle through
scripts/build-l1-folder-selection.sh. Its four-state test checks counts,
identity sets, exclusive iteration and focus with ordinary assertions.
Native L1 and Wine runtime/review gates remain pending. This does not close
the stopped MSI dialog fixture or the mspatcha PA19 decoder gap, and no
production candidate is promoted by preparing the final batch entry.

The final targeted assertion bundle can combine the first shell selection
L1 with the offsets test's corrected writer-size todo revision. Build via
scripts/build-l1-folder-selection.sh NEW_OUTPUT
docs/wine-repro/batch3-measure/selection-offsets-l1.list. Both candidates'
complete body parity is verified before compilation, and both source sets,
all EXEs and the manifest are hashed. Complete native success requires
40095/zero failures for offsets and 357/zero failures/four completed states
for shell selection, on both architectures. Native execution remains pending;
neither an expected count nor successful compilation is recorded as L1.
