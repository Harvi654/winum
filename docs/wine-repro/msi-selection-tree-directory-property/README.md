# MSI SelectionTree directory property: Windows MEASURE

Original, AI-assisted (Codex), LGPL-2.1-or-later. Native expected property
values are unknown. No production patch, validation marker or series changes.

Two original temporary MSI packages define our own directory tree, two
features with directory-key-path components, and one small modal dialog.
The direct case binds SelectionTree and PathEdit to INSTALLDIR. The indirect
case binds SelectionTree to BROWSE (initially INSTALLDIR), with an indirect
PathEdit. Features point at INSTALLDIR and ALTDIR. This selects the actual
directory-bound versus browse-property branch changed by the patch.

Only CostInitialize, FileCost, CostFinalize and the named WinumSelect UI
action execute. Revision 3 requests the own dialog through public MsiDoAction;
it does not call INSTALL or create/execute an installation sequence. There is
no file-copy or product-registration action or vendor MSI. MsiOpenPackageEx
ignores existing machine state for this own product. MsiSetInternalUI's
previous process setting is restored afterward.

An actor thread enumerates only the main thread's own-process windows,
finds the fixed fixture title through bounded WM_GETTEXT and the tree/button,
selects the first and second
feature via bounded standard treeview messages and closes Done. No input is
injected into other programs, and no system-wide hook is used. The main
thread owns all MSI handles and reads properties before/after the dialog;
the actor never accesses them. Its state stays alive until the thread joins.
The join pumps messages and has a 20-second bound; an atomic stop ends the
actor's discovery loop if the UI action has already returned. A separate
60-second process watchdog covers modal UI, automation and cleanup.

Outputs give API return codes, UI discovery/selection success and comparisons
against the before values and our two fixed directory identifiers. No real
path, machine/user name or general window text is printed. Both property
outcomes remain observations. Missing dialog/tree/selection, API errors or
timeouts invalidate coverage; zero exit alone is not assertion L1. Require
two CASE_END rows, two successful selected_child controls and complete
before/after properties. Revision 3 prints the MsiDoAction return and requires
that API to succeed; the earlier SHOWDIALOG message return was a different
contract and is not used as a property oracle.

All databases/handles and own files are released. Normal errors are reported;
an abnormal watchdog termination can leave the own temporary directory. Both
architectures build with -Wall -Wextra -Werror. The program is in both bundle
lists as msi-tree-property. Run it in the final batch-3 VM visit and return
both text outputs and exitcodes.txt even on failure. Do not touch the mouse
or keyboard while its two brief dialogs run. No application is needed.

Read in full before preparation: the production patch/page and public Wine
dialog.c property handler, SelectionTree and PathEdit setup/notification
paths, MSI action/message routing and relevant package-test schemas. The
SQL fixture, property comparisons and actor are original; standard schemas
and message/API identifiers follow public declarations. The initial
second-model advisory request hit a usage limit. The revision-3 advisory
and checks are recorded separately below. Native acceptance, Wine assertions
and L1 remain pending; raw output will not be committed.

## Initial Windows result and fixture correction (2026-11-09)

Both architectures exit 1 with identical observations: creating the Property
table succeeds, but the first INSERT returns 1615 (ERROR_BAD_QUERY_SYNTAX).
The second case fails at the same step. No dialog, selection, costing or
property comparison was reached, so no native directory-binding contract
can be inferred. This is our SQL fixture error, not evidence about the patch.

Revision 2 adds explicit column lists to all 23 INSERT statements, including
formatted Control rows. MSI SQL requires that syntax; the previous abbreviated
INSERT INTO table VALUES form was rejected by Windows. The program prints
FIXTURE revision=2 explicit_insert_columns=1 to distinguish the rerun. No
expected property value is added. Both strict architecture builds pass;
native setup and actual direct/indirect values still need a targeted rerun.
No production implementation, marker or series changes.

## Second Windows result and final fixture attempt (2026-11-09)

Revision 2 results match between x86_64/i686, both exit 2. All 32 SQL
operations, summary information, package opening, costing and initial
property reads succeed. Before the direct dialog, INSTALLDIR is a resolved
path and BROWSE remains the INSTALLDIR identifier. SHOWDIALOG then returns
-1. There is no UI record, after-property record, CASE_END or second case.
This is incomplete fixture execution, not the Windows selection contract.

The old actor's same-process GetWindowText could synchronously wait for the
main thread, which waited for that actor without pumping after the rejected
UI request. This is a source-derived explanation of the timeout, not a
measured stack trace. Revision 3 uses bounded WM_GETTEXT, a stop flag, and a
pumped join with an explicit timeout error. It requests the dialog directly
with MsiDoAction(package, WinumSelect), as supported by open Wine's action.c
dialog fallback and exercised by its existing package.c named-dialog test.
Those sources establish the API route, not the unmeasured property values.
No installation sequence or extra MSI action is introduced.

This is the third and final deliberate fixture iteration. If Windows still
does not produce both valid selection cases, stop this line of repair,
record the last result and move on under AGENTS.md's three-round rule. Do
not guess assertions or request another fixture retry. Use tree-final.list
for this targeted VM run; the already passed NULL-stream L1 and completed
mspatcha measurements do not need repetition.

A read-only second-model advisory checked revision 3 against the public
action routing and named-dialog test. It found the UI entry, state lifetime,
bounded actor and cleanup order sound, with no copied nontrivial block; this
is not an originality certificate or native acceptance. Its suggested guard
is included: the initial INSTALLDIR must be nonempty and must not itself be
either directory identifier, while BROWSE must still name INSTALLDIR. This
prevents an unresolved direct binding from supplying misleading coverage.
Both strict x86_64/i686 builds and bundle checksums pass. Windows dialog
acceptance and selection/property results remain pending.

Build only the final two-EXE measurement bundle:

```sh
bash scripts/build-measure-bundle.sh /path/to/NEW-output docs/wine-repro/batch3-measure/tree-final.list
```

Run run-measure.bat once in a fresh offline Windows 10 folder, without
compatibility mode or administrator relaunch, and return the two
result-msi-tree-property-*.txt files and exitcodes.txt even if incomplete.

## Third Windows result: automation coverage blocked (2026-11-09)

Revision 3 produces identical x86_64/i686 observations apart from bitness;
both exit 1. SQL, package setup, costing and all property reads succeed.
Both named-dialog actions return 0, and both CASE_END records and the final
SUMMARY are present. The maintainer saw and dismissed the dialogs manually.
However, every case reports UI found=0 selected_child=0, followed by
SelectionAutomation error 13; each process ends with api_errors=2.

The before/after properties are unchanged in both cases: INSTALLDIR remains
its initial non-identifier value, and BROWSE still names INSTALLDIR. These
are observations without a successful child-feature selection, not evidence
for either selection property's Windows contract. Manual dismissal cannot
replace the missing positive selection control. The screenshot confirms a
visible dialog, so found=0 describes our discovery failure, not absence of
native UI. It does not establish whether dismissal preceded discovery.

The best remaining hypothesis is that restricting discovery to the caller's
thread and process misses the windows used by native MSI UI. The returned
text does not include window owner thread/process measurements, so this is
not a confirmed cause. A future redesign would first measure only the
fixture dialog's owner identity through public APIs and then establish a
reliable selection/closure control; it must not infer values from this run.

Stop this fixture after the three native iterations, as required by
AGENTS.md. No fourth repair or same-fixture rerun is requested. Native
selection assertions and L1 remain blocked; no candidate or test marker is
promoted. The source and history are retained. Continue the other batch-3
patches independently; manual dismissal of these own UI dialogs does not
invalidate the earlier NULL-stream L1 or other successful measurements.
