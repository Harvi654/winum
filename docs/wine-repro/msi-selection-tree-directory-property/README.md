# MSI SelectionTree directory property: Windows MEASURE

Original, AI-assisted (Codex), LGPL-2.1-or-later. Native expected property
values are unknown. No production patch, validation marker or series changes.

Two original temporary MSI packages define our own directory tree, two
features with directory-key-path components, and one small modal dialog.
The direct case binds SelectionTree and PathEdit to INSTALLDIR. The indirect
case binds SelectionTree to BROWSE (initially INSTALLDIR), with an indirect
PathEdit. Features point at INSTALLDIR and ALTDIR. This selects the actual
directory-bound versus browse-property branch changed by the patch.

Only CostInitialize, FileCost and CostFinalize execute. The public
MsiProcessMessage INSTALLMESSAGE_SHOWDIALOG request displays the own dialog;
there is no InstallExecute sequence, file copy, product registration or
vendor MSI. MsiOpenPackageEx ignores existing machine state for this own
product. MsiSetInternalUI's previous process setting is restored afterward.

An actor thread enumerates only the main thread's own-process windows,
finds the fixed fixture title and tree/button, selects the first and second
feature via bounded standard treeview messages and closes Done. No input is
injected into other programs, and no system-wide hook is used. The main
thread owns all MSI handles and reads properties before/after the dialog;
the actor never accesses them. Its state stays alive until the thread joins.
A 60-second process watchdog covers modal UI, automation and cleanup.

Outputs give API return codes, UI discovery/selection success and comparisons
against the before values and our two fixed directory identifiers. No real
path, machine/user name or general window text is printed. Both property
outcomes remain observations. Missing dialog/tree/selection, API errors or
timeouts invalidate coverage; zero exit alone is not assertion L1. Require
two CASE_END rows, two successful selected_child controls and complete
before/after properties. The native dialog return is printed rather than
assuming every nonzero dialog result is an MSI error.

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
and message/API identifiers follow public declarations. A second-model
advisory was requested under AGENTS.md, but its session hit a usage limit
before returning a review; no completed advisory or originality certification
is claimed for this fixture. Native acceptance, Wine assertions and L1
remain pending; raw output will not be committed.

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
