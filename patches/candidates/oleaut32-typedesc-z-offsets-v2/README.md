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
