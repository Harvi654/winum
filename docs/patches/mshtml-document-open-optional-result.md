# mshtml-document-open-optional-result.patch

<!-- winum-test: missing -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `mshtml`
- **Origin:** Solidworks-for-Linux (private for now), `patches/`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Simulation: Create Study failed.

## Cause and fix

`IHTMLDocument2::open` accepts a NULL result pointer.

## Wine test

None in the production patch. The
[v2 candidate](../../patches/candidates/mshtml-document-open-optional-result-v2/README.md)
adds test_doc_open_optional_result beside dom.c's existing open tests.
Its ordinary ok() assertions compare supplied/NULL result pointers on
separate loaded copies of our own HTML. Values come from our Windows
MEASURE; the exact standalone body then passed native assertion L1.
The production marker stays missing because that test is not promoted.

## Validation

| Level | Result |
| --- | --- |
| L0 | full builds full-1/full-2 |
| L1 | v2 candidate: Windows 10 x86_64/i686, 34 checks each, 0 failures, exit 0 in both returned result sets (2026-11-09). Production has no integrated test yet |
| L2 | not recorded |
| L3 | compiled; Gecko-dependent probes not run |
| L4 | Simulation study, mesh, solver, results |
| L5 | in use since 2026-10-28 (maintainer to confirm) |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

Formal cross review, Gecko-enabled paired mshtml.dom L2 and consumer validation
remain open before candidate promotion. Do not count a disabled mshtml backend
or native MEASURE alone as runtime validation.

## Evidence

Solidworks-for-Linux `docs/wine-repro/mshtml-open/README.md`; overview in `docs/patch-status.md`.


## History

- 2026-11-09: Recorded native public-API MEASURE and exact-body assertion L1
  for the v2 candidate. Both architectures passed 34 checks in two returned
  result sets. Full-series fuzz-zero apply and complete PE DLL/test links
  passed; implementation hunks and production series remain unchanged.

## Draft for maintainer decision (2026-11-10): missing Gecko-enabled L2

**Proposal only; the active test marker is unchanged.** This is not a
request for l3-only: Windows supports the API and the v2 assertion already
passed native L1. The missing level is a usable paired mshtml runtime.

Why all-1 did not run it: no matrix group names mshtml, and
scripts/test-series.sh disables mshtml globally with
WINEDLLOVERRIDES="mscoree,mshtml=". Unlike Build Wine, the L2 dependency
step does not prepare wine-gecko. Thus enabling the name alone either
leaves the backend disabled or risks an installer prompt/no-Gecko exit;
none of those outcomes validates document open. There is no recorded
Gecko-enabled full dom run or observed hang in this environment. Do not
turn that missing evidence into a claim that the unit cannot run.

Preferred route: provision Gecko, enable builtin mshtml only for its test
processes, prove that both PE dom units complete, then put mshtml in g5
(or a separate bounded group if its measured duration warrants one).
Keep the existing external unit timeout and treat a timeout, missing
Gecko, failed enumeration or backend-wide skip as incomplete L2. The
candidate's five-second document-ready bound does not bound every old
test in the full dom unit. Pair existing dom tests against vanilla and
retain the native-tested optional-output assertion on the candidate side.

For a dedicated download, use the Gecko version/checksums pinned in Wine
11.20's open-source dlls/appwiz.cpl/addons.c, not a floating latest build:

| MSI, version 2.47.4 | SHA-256 |
| --- | --- |
| wine-gecko-2.47.4-x86.msi | 26cecc47706b091908f7f814bddb074c61beb8063318e9efc5a7f789857793d6 |
| wine-gecko-2.47.4-x86_64.msi | e590b7d988a32d6aa4cf1d8aa3aa3d33766fdd4cf4c89c2dcc2095ecb28d066f |

Official source: https://dl.winehq.org/wine/wine-gecko/2.47.4/ . These are
open-source Wine Gecko packages, not native Microsoft components. Check
both hashes before installing into disposable test prefixes. This draft
records the setup needed; no packages were downloaded or installed and
no runtime success or non-hanging full unit is claimed here.

Alternative decision: accept a maintainer-run paired local L2 using the
Gecko-bearing all-1 runtime and matching Wine test EXEs, returning filtered
dom summaries, timeouts and failure locations for both architectures.
That would be an explicit infrastructure exception for the L2 venue,
not a replacement of the existing Windows L1 or a fabricated l3-only
reason. Until one route runs, keep the L2 gate open and do not add a group
that only records disabled-backend skips as success.
