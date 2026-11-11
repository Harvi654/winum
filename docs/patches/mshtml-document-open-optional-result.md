# mshtml-document-open-optional-result.patch

<!-- winum-test: l1 -->

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
| L0 | full builds full-1/full-2; winum-11.20-1 series (promoted 2026-11-11): every entry applies with `--fuzz=0` on the official 11.20 archive; the tree is identical to that of `test-run/all-6` (run 42997560598) |
| L1 | 34/0, both arch (promoted file: `mshtml-document-open-optional-result-v2`) |
| L2 | not recorded; winum-11.20-1: ok with pinned Gecko since all-2 (`mshtml:dom` 44107 -> 44140) |
| L3 | compiled; Gecko-dependent probes not run |
| L4 | Simulation study, mesh, solver, results; winum-11.20-1: SOLIDWORKS Simulation, deferred |
| L5 | in use since 2026-10-28 (maintainer to confirm) |
| Cross review | Claude, approve |

## Open points

The bounded candidate implementation/test has Claude's cross review.
Gecko-enabled paired mshtml.dom L2 and consumer validation remain open
before candidate promotion. Do not count a disabled mshtml backend
or native MEASURE alone as runtime validation.

## Evidence

Solidworks-for-Linux `docs/wine-repro/mshtml-open/README.md`; overview in `docs/patch-status.md`.


## History

- **2026-11-11, promoted** into `patches/series` for winum-11.20-1 from `mshtml-document-open-optional-result-v2` (maintainer approval of `docs/release/winum-11.20-1.md` at `ef64451`). Open: loaded state only in the test.

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


## Maintainer decision and CI setup (2026-11-10)

**No exception accepted.** The maintainer chose Gecko-enabled paired CI;
the old draft above is historical, and the active marker remains missing
until the native-tested candidate is promoted through the normal gates.

Mshtml is now in workflow group g5 and in the script's default test list.
Both vanilla and series provision Wine Gecko 2.47.4 x86/x86_64 from the
fixed official MSI URLs. wine/gecko.env contains the two SHA-256 values
listed above; scripts/lib/wine-gecko.sh first verifies that version and
both architecture hashes match the selected, verified Wine source's
addons.c. It verifies every download/cache file, and rechecks each MSI
before its bounded silent installation. A changed source pin or corrupt
MSI stops setup; neither can become a disabled-backend success.

Only jobs requesting mshtml prepare Gecko. It has a separate disposable
mshtml-prefix, preserving the previous prefix of other suites. Wineboot
and MSI setup keep the default mshtml-disabled override. Each architecture's
own backend-probe process enables builtin mshtml, loads/registers that
builtin in its prefix's registry view (wineboot skipped it), loads an
original in-memory HTML stream, waits at most five seconds for complete,
and requires its controlled title and parent window. External timeouts
also bound boot, installation, the probe and enumeration. COM registration
is prefix-wide, not process-local; it is isolated from other suites by the
dedicated prefix. No persistent DLL override is written.

Unit enumeration and every mshtml test process alone receive the dedicated
prefix and WINEDLLOVERRIDES='mscoree=;mshtml=b'. Other suite processes retain
their original prefix and disabled mshtml override. All enumerated mshtml
units run on both PE architectures with the existing UNIT_TIMEOUT (300 s
by default, followed by the existing ten-second kill bound). An EXIT trap
also kills both test wineservers and Xvfb after setup/coverage failures.

scripts/check-mshtml-l2.py rejects absent/mismatched backend probes, missing
or invalid enumeration, missing/unfinished/timed-out units, an all-zero-
assertion backend-wide skip, missing dom coverage or dom's required-
interface skip. Optional per-feature skips remain reported; individual
zero-check units are not automatically backend-wide skips. Ordinary test
failures stay in the paired comparison rather than being misclassified
as a missing backend. mshtml-coverage-<variant>.json records the scope;
MSHTML_COVERAGE/MSHTML_INCOMPLETE are the filtered summary lines.

Checks performed here: both probe PE builds pass -Wall -Wextra -Werror;
12 coverage/download/cleanup tests pass using only own temporary fixtures
and a mock transport, without executing Wine or downloading Gecko. YAML,
shell syntax and source pin parity pass. A second-model read-only advisory
found no concrete backend/prefix/override fail-open issue and no identifiable
borrowed implementation in the bounded original delta (not an originality
proof); its failure-cleanup suggestion is included. Runtime installation,
full dom completion and paired L2 remain to be observed in Claude's
upcoming test-run/all-2. No runtime pass, Gecko download or local Wine
execution is claimed by these build/static checks.
