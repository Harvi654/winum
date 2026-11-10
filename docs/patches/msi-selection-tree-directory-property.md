# msi-selection-tree-directory-property.patch

<!-- winum-test: l3-only: after three native fixture attempts, the own MSI dialog actor never established a positive feature-selection control, so the directory-binding Windows oracle is unavailable; retain the Wine direct/indirect dialog reproducer and SOLIDWORKS installation L4 pending a redesigned native UI measurement (temporary maintainer exception 2026-11-10) -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `msi`
- **Origin:** Solidworks-for-Linux (private for now), `patches/`
- **In winum:** `patches/series` from winum-11.20-1

## Accepted temporary exception (maintainer decision, 2026-11-10)

The maintainer accepts the proposed temporary l3-only exception below for
this release. The native selection oracle is unavailable because none of
the three fixture attempts established the positive selection control;
unchanged before/after properties do not become Windows expected values.
Retain the original Wine direct/indirect UI reproducer and recorded
SOLIDWORKS installer L4, with consumer work currently deferred as documented.

**Open follow-up after the release:** use another public-API automation
route, first identifying the own dialog's actual ownership and proving
selection/closure independently of the failed caller-thread actor. Measure
both directory bindings natively, then add an exact Windows-tested Wine
assertion and replace this temporary exception. Do not repeat the stopped
fixture unchanged or close this item before the new oracle exists.

## Problem as the user saw it

SOLIDWORKS installation: the feature tree overwrote INSTALLDIR.

## Cause and fix

The workaround preserves the resolved directory property while allowing
browse indirection to follow the selected feature. Its native selection
contract remains unmeasured; retention uses the dated temporary exception.

## Wine test

None in the patch.

## Validation

| Level | Result |
| --- | --- |
| L0 | full builds full-1/full-2 |
| L1 | no Wine test; native direct/indirect assertions missing |
| L2 | not recorded |
| L3 | synthetic direct/indirect bindings (x64) |
| L4 | installation; file set identical to Windows |
| L5 | in use since 2026-10-28 (maintainer to confirm) |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

The original native MEASURE reached both own dialogs in its third iteration
(2026-11-09), but its actor found neither dialog and selected neither child.
The maintainer dismissed the UI manually. Both architectures exit 1 with
two automation errors. Unchanged properties without selection are not a
native oracle. The fixture is stopped under AGENTS.md's three-round rule;
no assertion L1 or replacement candidate is claimed. A temporary release
exception is now accepted by the dated maintainer decision above; native
automation redesign remains open after the release.
See [measurement state](../wine-repro/msi-selection-tree-directory-property/README.md).

## Evidence

Solidworks-for-Linux `docs/wine-repro/msi-feature-map/README.md`; overview in `docs/patch-status.md`.


## Original proposal (2026-11-10), now accepted: native UI automation gap

**Accepted temporarily by the dated maintainer decision above.** The
proposal details are retained; this is not an assertion that public Windows
APIs cannot test SelectionTree.

What we measured: three iterations of our original temporary MSI fixture,
built for x86_64/i686. The final iteration successfully creates both own
packages, costs them and invokes both named dialogs with MsiDoAction.
It reads INSTALLDIR/BROWSE before and after and finishes both cases. The
maintainer saw and dismissed the dialogs, but the actor reports found=0,
selected_child=0 in each. Both architectures exit 1 with two automation
errors. The unchanged property values therefore say nothing about the
Windows result of selecting the second feature. No selection-value oracle
or conformance assertions may be derived from them.

Automation gap: the actor enumerates the caller's thread windows and then
restricts them to its process. A native MSI UI window may fall outside that
scope; its actual ownership was not measured. That is a hypothesis, not
a confirmed native implementation fact. Manual dismissal did not provide
the missing positive feature-selection control. Under the three-round
rule the current fixture is stopped; do not request a fourth retry or
convert its non-selection observations into expected values.

Accepted temporary reason (original proposal wording):

> l3-only: after three native fixture attempts, the own MSI dialog actor never established a positive feature-selection control, so the directory-binding Windows oracle is unavailable; retain the Wine direct/indirect dialog reproducer and SOLIDWORKS installation L4 pending a redesigned native UI measurement.

This is a temporary automation exception by explicit maintainer decision. It is weaker than proving that an open tool cannot build a valid
call, and must not be recorded as such. Windows supports the APIs; the
current actor is the limitation.

Existing reproducer:
[measure.c and its history](../wine-repro/msi-selection-tree-directory-property/README.md)
build two original packages with INSTALLDIR/ALTDIR features. In the direct
case SelectionTree and PathEdit bind to INSTALLDIR; in the indirect case
the tree binds to BROWSE (initially INSTALLDIR) and PathEdit dereferences
it. Only costing and the named dialog execute; no INSTALL/file-copy/product
registration action or vendor MSI is used. The actor selects root then
child through bounded tree-view messages, verifies the selected child,
and closes Done. The process has its own watchdog and outputs only fixed
identifiers, comparisons and API statuses, not real paths or user names.

For a meaningful Wine L3, require found=1 selected_child=1 in **both**
cases, successful named actions, both CASE_END rows and no timeout/API
error. Compare vanilla and patched Wine on the same original fixture.
The intended workaround preserves the resolved INSTALLDIR in the direct
case and lets BROWSE follow the selected ALTDIR in the indirect case.
These are proposed Wine-side workaround checks, not measured Windows
values. The earlier page records x64 direct/indirect synthetic coverage;
no new run of the current actor or both architectures is claimed here.
A build, manually closed dialog or exit code without positive selection
is insufficient. Existing SOLIDWORKS installer L4 is retained; its current
consumer work is deferred under AGENTS.md.

Follow-up replacing the exception: a redesigned public-API observer first
identifies only the fixture dialog's thread/process ownership, establishes
a reliable and positively verified feature selection, then measures both
bindings on Windows. Only after those outputs should an exact native-
tested Wine assertion replace this exception. No fourth iteration, new
candidate or guessed native value is introduced by the accepted exception.
