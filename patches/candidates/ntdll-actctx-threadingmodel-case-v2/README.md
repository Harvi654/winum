# Activation-context threading-model case v2: conformance test

Original, AI-assisted (Codex), LGPL-2.1-or-later. Candidate only, not submitted
upstream. Replace `ntdll-actctx-threadingmodel-case.patch` in a private recipe
with this file; do not apply both. The parser implementation is unchanged from
production. V2 adds `dlls/ntdll/tests/actctx.c` and its Makefile entry.
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

The isolated ntdll test recreates the same 42 manifests, with distinct
assembly identities and fresh temporary paths. It creates, activates and
queries each context, verifies the returned prefix and model, deactivates
and releases it, and deletes its file. Neither the synthetic COM DLL nor
the CLR is loaded. The three-DWORD query-result prefix layout comes from
open Wine's parser and kernel32 activation-context tests. Native observations
supply the expected values; ReadProcessMemory bounds the test-side copy.

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

The builder checks that the candidate test block equals test-body.h byte for
byte, then builds static x86_64/i686 EXEs with MinGW, -Wall -Wextra -Werror.
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
The earlier MEASURE supplies the oracle; the native assertion L1 pass is
recorded below.

actctx-model-l1 is also in the main VM bundle list and the topic's l1.list.
The generic builder does not perform candidate/body parity checking itself;
that equality was checked for this revision. Recheck it after future changes
or use the dedicated builder before accepting new L1 evidence.

## Validation and remaining gates

- Complete native MEASURE: both architectures, exit 0, 2026-11-09.
- Native assertion EXEs compiled for both architectures with -Wextra -Werror;
  dedicated bundle checksums verified. Native assertion results are below.
- All 62 patches apply with `--fuzz=0` when this replaces the production
  entry. The resulting ntdll/actctx.c is byte-identical to production.
- Both ntdll DLLs and complete ntdll_test.exe targets linked (x86_64/i386),
  including the new actctx test. Source: upstream wine-11.20 tag, configuration
  `--enable-archs=x86_64,i386 --without-x --without-wayland --without-freetype`.
  This is focused compile validation, not an official-archive, full-feature
  L0 build or runtime execution.
- Formal cross review, vanilla/patched runtime comparison, paired ntdll and
  neighbouring kernel32 activation-context L2, and consumer L4 remain pending.

The existing broad activation-context suite is in kernel32/tests/actctx.c.
This new isolated test belongs to ntdll/tests because it validates the ntdll
manifest parser changed here, through the same public API path. It requires
no extra runtime imports beyond ntdll tests' standard kernel32 API support.


A second-model read-only advisory read the whole candidate, shared body,
native harness, builder, both native texts and the relevant open Wine parser
and tests. It confirmed oracle/body parity and the source basis for the
independent todos, with no P1/P2 blocker or conspicuous copied block. This
cannot prove originality and does not replace Claude's formal cross review.
One failure-path limit remains: an unexpected CloseHandle failure can retain
a file handle until process exit; both close and subsequent cleanup failures
are reported as failed assertions, never a pass.


## Windows assertion L1 (2026-11-09)

The maintainer ran the dedicated bundle in offline Windows 10. Both x86_64
and i686 completed **609 checks, 0 failures**, no FAIL lines, and exit 0.
All todo_wine assertions ran as ordinary native assertions. This establishes
L1 for the same test body included in the v2 candidate, not just its preceding
MEASURE. Raw uploaded logs are not committed. Formal cross review,
vanilla/patched runtime comparison, paired L2 and consumer L4 remain open;
the production patch still has no integrated test and is unchanged.
