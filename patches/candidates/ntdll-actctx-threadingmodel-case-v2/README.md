# Activation-context threading-model case v2: conformance test

Original, AI-assisted (Codex), LGPL-2.1-or-later. Candidate only, not submitted
upstream. Replace `ntdll-actctx-threadingmodel-case.patch` in a private recipe
with this file; do not apply both. The parser implementation is unchanged from
production. After review, the current test follow-up adds the matrix beside
`test_find_com_redirection` in `dlls/kernel32/tests/actctx.c`. It adds no
ntdll test file or Makefile entry.
No production patch, series entry or test marker is changed.

## Native oracle and scope

The maintainer ran our 42-case MEASURE in offline Windows 10 on x86_64 and
i686 (2026-11-09). Outputs match apart from the bitness header. Both exited
0: 34 contexts were created and queried, eight inputs were rejected, and
there were no query or setup failures. Complete aggregate observations are
[recorded here](../../../docs/wine-repro/ntdll-actctx-threadingmodel-case/README.md).
Only our own generated manifests and public activation-context APIs were
used; no vendor binary, symbol or native implementation was inspected.

Both comClass and clrClass accept Apartment/Free/Both/Neutral in title,
lower, upper and mixed case, with models 1/2/4/5 respectively. Windows rejects
empty, unknown and space-padded model values with ERROR_SXS_CANT_GEN_ACTCTX
(14001). Without the attribute it returns model 3 for comClass and 4 for
clrClass. The latter two behaviours differ from the existing Wine code
independently of case folding; they are retained as todo_wine controls.
The production fix's scope is not expanded by this test.

## Test

The kernel32 activation-context test recreates the same 42 manifests, with distinct
assembly identities and fresh temporary paths. It creates, activates and
queries each context, verifies the returned prefix and model, deactivates
and releases it, and deletes its file. Neither the synthetic COM DLL nor
the CLR is loaded. It reads size, flags and model directly through the existing
`struct comclassredirect_data`, after checking the returned pointer and length.
The Wine file already declares that structure; only the standalone harness
needs a matching declaration. Native observations supply the expected values.

The 24 noncanonical specified-model checks use ordinary ok(), so a vanilla
case-sensitive parser should fail them. Eight canonical cases remain controls.
There are eight todo_wine rejection checks and two todo_wine omitted-model
checks, for independent unfixed Wine differences. The expected rejection error
is checked when creation really rejects the input. No unmeasured parser
normalization, apartment activation or COM server behaviour is asserted.

## Native assertion L1

```sh
bash scripts/build-l1-actctx-model.sh /path/to/NEW-output
```

The builder finds the named function and its closing brace in the patch
additions and checks that it equals test-body.h byte for byte, then builds static x86_64/i686 EXEs with MinGW, -Wall -Wextra -Werror.
It downloads nothing. The dedicated bundle includes the shared body, native
harness, license and SHA256SUMS. Copy the complete directory to a new directory
in the offline Windows 10 VM and run run-measure.bat. Return:

- result-actctx-model-l1-x86_64.txt;
- result-actctx-model-l1-i686.txt;
- exitcodes.txt.

Both exits must be 0, with a completed `SUMMARY checks=... failures=0` and no
FAIL lines. The standalone harness treats all todo_wine assertions as ordinary
assertions, as required on Windows. The integrated Wine test uses Wine's own
todo handling. Do not use the native harness to score a Wine runtime comparison.
The earlier MEASURE supplies the oracle; historical native L1 is recorded
below and the revised reader still needs a native refresh.

actctx-model-l1 is also in the main VM bundle list and the topic's l1.list.
The generic builder does not perform candidate/body parity checking itself;
that equality was checked for this revision. Recheck it after future changes
or use the dedicated builder before accepting new L1 evidence.

## Validation and remaining gates

- Complete native MEASURE: both architectures, exit 0, 2026-11-09.
- Native assertion EXEs compiled for both architectures with -Wextra -Werror;
  dedicated bundle checksums verified. Historical results and refresh status are below.
- All 62 patches apply with `--fuzz=0` when this replaces the production
  entry. The resulting ntdll/actctx.c is byte-identical to production.
- After relocation, both ntdll DLLs and complete kernel32_test.exe targets
  linked (x86_64/i386), including the revised actctx matrix. Source: upstream wine-11.20 tag, configuration
  `--enable-archs=x86_64,i386 --without-x --without-wayland --without-freetype`.
  This is focused compile validation, not an official-archive, full-feature
  L0 build or runtime execution.
- Formal cross review, vanilla/patched runtime comparison, paired ntdll and
  neighbouring kernel32 activation-context L2, and consumer L4 remain pending.

The test now extends the existing kernel32 activation-context suite as
requested in cross review. The next L2 run must select kernel32.actctx;
ntdll.actctx is no longer a new test unit. Keep ntdll coverage for the changed
parser too. The general test-series g3 list does not automatically select
kernel32, so the candidate validation recipe must request this unit explicitly.


A second-model read-only advisory read the whole candidate, shared body,
native harness, builder, both native texts and the relevant open Wine parser
and tests. It confirmed oracle/body parity and the source basis for the
independent todos, with no P1/P2 blocker or conspicuous copied block. This
cannot prove originality and does not replace Claude's formal cross review.
One failure-path limit remains: an unexpected CloseHandle failure can retain
a file handle until process exit; both close and subsequent cleanup failures
are reported as failed assertions, never a pass.


## Windows assertion L1 (2026-11-09)

Before the test-placement follow-up, the maintainer ran the dedicated bundle
in offline Windows 10. Both x86_64
and i686 completed **609 checks, 0 failures**, no FAIL lines, and exit 0.
All todo_wine assertions ran as ordinary native assertions. This establishes
historical L1 for the pre-review test body, not just its preceding MEASURE.
The relocated/direct-reader body keeps the expected values but has not yet
been rerun natively; the old 609-check result is not claimed for that revision. Raw uploaded logs are not committed. Formal cross review,
vanilla/patched runtime comparison, paired L2 and consumer L4 remain open;
the production patch still has no integrated test and is unchanged.


## Cross-review follow-up (2026-11-09)

The dated v3 paragraph records this test-only follow-up in the same candidate
folder. The test is beside the existing COM redirection checks in
kernel32/tests/actctx.c, uses struct comclassredirect_data directly and has
no BEGIN/END markers or inline attribution. Attribution stays in the patch
header. All 42 input cases, measured model/error values, todo_wine scope and
production parser changes are unchanged. The L1 builder compares the named
function without markers and the standalone harness supplies the full record
layout from the existing Wine test.

The complete series applies at fuzz 0 and standalone EXEs compile on both
architectures with -Wall -Wextra -Werror. Refreshed native assertion L1 and
kernel32 actctx L2 remain open; the 609-check result above is historical.
