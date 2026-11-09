# MSFT type description offsets v2: reader compatibility test

Original, AI-assisted (Codex), LGPL-2.1-or-later. Candidate only, not submitted
upstream. Replace the production `oleaut32-typedesc-z-offsets.patch` in a
private recipe with this file; do not apply both. All three implementation
changes are identical to production. V2 adds a Wine type library compatibility
test that compares the stored type description with Windows.

## Windows oracle (2026-11-09)

The maintainer's offline Windows 10 x86_64/i686 MEASURE completed with exit 0,
`SUMMARY api_errors=0 expectations=none`, and complete release/CoUninitialize
markers. Each native writer completed 40000 setup calls with S_OK. The
read-only audit of our own generated library found low-word bit 15 and
nonzero high words in all three reader paths. Both readers returned the same
sample trees, bounds, record names and field types. Numeric HREFTYPE values
differ by architecture; they are not an oracle. See the
[measurement and aggregate evidence](../../../docs/wine-repro/typedesc-offsets/README.md).
No vendor type library, implementation, symbol or binary was inspected.

The test creates 5000 records with I4 fields and interleaved aliases:
PTR -> CARRAY -> PTR -> USERDEFINED(record). Distinct references and array
bounds prevent descriptor deduplication from keeping offsets small. There
is only one CARRAY per alias: element recursion does not append another
array-description record, avoiding the separate recursive writer lifetime
issue found in the archived first draft. No serialized bytes are edited.

After releasing the writer, it copies the file unchanged to a fresh name to
avoid the writer cache, verifies actual bit-15/high-word coverage read-only,
and loads the copy. It checks aliases for records 0/1500/3000/4999, their
array bounds, referenced record identity, size and I4 field. No invalid-input
fallback result is asserted. ReadProcessMemory limits test-side traversal;
it cannot prevent a fault inside a Wine reader API. A crash, missing summary,
failed setup or incomplete coverage is a failed run, never a skip or pass.

## Native assertion L1

```sh
bash scripts/build-l1-typedesc-offsets.sh /path/to/NEW-output
```

The script checks that the candidate's added test block is byte-for-byte
identical to `docs/wine-repro/typedesc-offsets/test-body.h`, then builds both
architectures with our standalone assertion harness. It needs MinGW, Bash,
Python 3 and ordinary build utilities; it downloads nothing and uses no Wine
runtime or Microsoft headers/binaries beyond MinGW. Sources, license and
SHA256SUMS accompany the dedicated bundle. `tdoffset-records-l1` is also in
`docs/wine-repro/measure-bundle.list` for the generic workflow/bundle builder.

Copy a fresh bundle into the offline Windows 10 VM and run run-measure.bat.
Return result-tdoffset-records-l1-x86_64.txt,
result-tdoffset-records-l1-i686.txt and exitcodes.txt. Acceptance requires
both exits 0, a completed `SUMMARY checks=... failures=0` and no FAIL lines.
The preceding successful MEASURE supplies expected values; it does not count
as execution of these new assertions.

## Validation and remaining gates

- Native MEASURE: complete on Windows 10, both architectures, 2026-11-09.
- Standalone L1 compile: MinGW GCC 14.2, `-Wall -Wextra -Werror`, both
  architectures; native assertion results are recorded below.
- All 62 patches apply with `--fuzz=0` when this replaces the z-offsets
  entry, both with production pointee v2 and the pointee-v3 candidate.
- With production pointee v2 and this candidate, both oleaut32 DLL targets
  and complete oleaut32_test.exe targets linked successfully (x86_64/i386).
  Source: upstream wine-11.20 tag, configuration
  `--enable-archs=x86_64,i386 --without-x --without-wayland --without-freetype`.
  This is focused compile validation, not an official-archive, full-feature
  L0 build or runtime test.
- Formal cross review, vanilla/patched runtime comparison, paired oleaut32
  typelib/tmarshal L2 and the consumers' L4 remain pending.

The standalone harness and integrated Wine suite call the same test body.
It uses ordinary ok() for the corrected paths, so the signed/truncated
reader offsets should fail on vanilla; no runtime result is claimed from
source inspection or compilation. Keep the production patch page marker and
`patches/series` unchanged until the applicable validation/review gates pass.


A second-model read-only advisory checked the complete candidate, shared body,
standalone harness, builder and MEASURE against open Wine and both returned
native texts. It found no blocker or conspicuous copied block; resemblance
inspection cannot prove originality and does not replace Claude's formal
cross review. One workflow limit remains: the generic bundle compiles l1.c
without the dedicated builder's parity check. The bodies were checked equal
for this revision; recheck parity before accepting evidence after any later
candidate/body change, or use the dedicated builder.


## Windows assertion L1 (2026-11-09)

The maintainer ran the dedicated standalone bundle in offline Windows 10.
Both x86_64 and i686 completed **40091 checks, 0 failures**, with exit 0
and no FAIL lines. This establishes native L1 for this candidate's shared
Wine test body; the successful MEASURE alone was not counted as L1. Raw
uploaded logs are not committed. Formal cross review, vanilla/patched runtime
comparison, paired L2 and consumer L4 remain open. The production patch still
has no integrated test and is not promoted by this record.

## candidates-4 failure analysis and test correction (2026-11-09)

The maintainer reported four typelib.c:8953 size failures per architecture in
Actions run 42594173229 on test-run/candidates-4: the referenced record has
the correct kind and one field, but cbSizeInstance is zero. The test/patch
copied onto that branch were read; no new runtime run is claimed here.

This is an independent Wine writer gap exposed by an over-broad combined
test assertion, not a defect in the three offset-reader changes. In public
Wine 11.20, ICreateTypeLib2_fnCreateTypeInfo initializes TKIND_RECORD size
to zero. AddVarDesc increments cVars and marks layout needed; LayOut handles
member identifiers/vtables but does not compute a record's instance size.
WMSFT_compile_typeinfo writes that unchanged cbSizeInstance to base->size,
and MSFT_DoTypeInfo reads the stored size back. None of the candidate's
offset hunks changes those paths. The Windows writer instead returns the
measured size 4 for the same original I4 record.

The corrected test keeps record kind/field count ordinary and separates
only the Windows size-4 check into todo_wine. Every pointer/array boundary,
tree, bounds, referenced record name and field-type assertion remains
ordinary. The generated inputs and API sequence are unchanged. Native
Windows treats the size check as ordinary too, including in the standalone
wrapper. Expected size is still 4, never relaxed to Wine's zero.

The header carries a dated v3 test-revision paragraph; the v2 directory is
retained so existing candidate recipes still find the file. The three
implementation hunks are unchanged. A complete revised native run should
report 40095 checks, zero failures (four more checks after splitting the
combined assertions). The previous 40091-check native L1 is historical
evidence for the same expectations; the revised-body L1 is recorded below,
and paired Wine runtime remains open. Wine is expected to report four size todos
with no new ordinary failure, but a rerun is pending. Neither the production
patch nor patches/series nor the candidates-4 branch is changed.

Correction validation: both strict standalone MinGW builds and body parity
pass. A fresh final 62-entry application passes --fuzz=0 and matches the
compiled files. Complete oleaut32.dll and oleaut32_test.exe PE targets link
for x86_64/i386 with the same focused configure flags documented above.
Legacy mutable OLECHAR name buffers remove const-qualification warnings;
their text and the fixture's API sequence are unchanged. No Wine runtime
or revised exact-body native assertion pass is inferred from these builds.

## Corrected-body Windows assertion L1 (2026-11-09)

The maintainer returned both results from the combined offsets/selection
bundle available at 8400e60. The corrected offsets body from 1ef60e0 passes
on offline Windows 10: x86_64 and i686 each report SUMMARY checks=40095
failures=0 and exit 0, with no FAIL lines. The native wrapper treats the
Wine-only size todo as an ordinary assertion, so the expected Windows
record size remains 4 and is checked for all four sampled records.

This establishes L1 for the actual split-assertion revision, superseding
the pending rerun noted during its preparation. The earlier 40091-check
pass remains historical evidence for the original combined assertion.
No raw logs or Windows files are committed. This native pass does not
verify the predicted four writer-size todos in Wine; the corrected Wine
runtime and paired oleaut32 L2 still need to be recorded. Formal cross
review and remaining consumer validation stay open. No production patch,
test marker, series entry or candidates-4 branch is changed.
