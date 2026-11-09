# Batch 2: Windows observations before regression-test expectations

Original, AI-assisted (Codex), LGPL-2.1-or-later. Text output only. No vendor
files, registry exports, proprietary data or network access are needed.
These programs have no hard-coded expected Windows return values.

## Type descriptions: first two patches

`typedesc.c` builds its own type libraries using ICreateTypeLib2 and
ICreateTypeInfo, saves them to generated temporary files, releases the writer,
and reloads them through LoadTypeLibEx(REGKIND_NONE). It prints HRESULTs and
the returned public TYPEATTR/TYPEDESC/ARRAYDESC trees. Generated files are
deleted on normal exit. No Autodesk type library or binary is used.

The small cases distinguish a pointer to I4, a pointer to encoded
`VT_ARRAY|VT_I4`, and a pointer to a structured SAFEARRAY(I4). A native writer
may reject or normalize the encoded case. Such an outcome must be preserved
as evidence: it cannot be counted as coverage of the reader defect. A
synthetic writer/reader round trip is not assumed to preserve the encoding.
The program reports creation and load errors, rather than treating them as
successful reader observations. The unsupported-pointee fallback to VARIANT
is not asserted to be a Windows contract.

Schema 2 adds a separate `--reader-only` process: it creates PTR(I4) through
the native API, releases the writer, validates the MSFT segment directory and
single pointer record in **its own generated file**, changes the low-word
pointee from `0003` to `2003` (one byte changes), then calls LoadTypeLibEx
on a new generated filename to avoid the original writer's cached identity.
The high-word marker and every other byte are preserved. This determines
whether the native reader accepts that encoding despite writer rejection,
and what public TYPEDESC tree it exposes. The format layout comes from open
Wine 11.20 source, not vendor binaries or disassembly. No external file can
be supplied to this program. The returned tree remains unknown until measured.
The separate process keeps rejection/crash of this synthetic encoding from
preventing the ordinary round trips and large-library measurement.

The large case constructs 5000 alias types, each containing a pointer to an
array whose element is a pointer to a uniquely sized inner array. Its intended
purpose is to grow both descriptor tables beyond the signed-low-word and
16-bit boundaries. Samples 0, 1500, 3000 and 4999 report the decoded trees.
Schema 2 audits the actual serialized offsets in this **self-generated**
library: type count/file size alone is not proof of boundary coverage.
`SERIAL` prints descriptor-table sizes and counts of pointer references,
array-description offsets and array-element references at/above 0x8000 and
0x10000. `SERIAL_SIGNED` separately counts low words with bit 15 set. Only
validated references wholly inside the intended segments are counted.
The large-library case is read-only; only the isolated reader fixture changes
one byte in its own small temporary file. Only text output leaves the VM.

## Build and Windows run

```sh
bash scripts/build-measure-batch2.sh /path/to/NEW-bundle
```

Builds x86_64/i686 static MinGW EXEs with `-Wall -Wextra -Werror`, packages
source and SHA256SUMS. Copy the whole bundle into a new directory in the
maintainer's offline Windows 10 x64 VM. Run `run-measure.bat` once and return
`result-typedesc-{x86_64,i686}.txt`, `result-reader-{x86_64,i686}.txt`, and
`exitcodes.txt`. The script refuses to overwrite results.
No CAD installation is needed. A zero exit means the programmed API operations
completed; it does not establish any expected Windows type semantics.

## Initial native observations (2026-11-08, schema 1)

Both Windows architectures produced identical observations apart from the
architecture header. Each process returned 2 because the program counts any
rejected API as an error; this does not invalidate the other completed cases.
No raw result files are committed.

| Case | Observed native result |
| --- | --- |
| Pointer to I4 | Load succeeds; alias kind 6; returned chain `001a -> 0003` |
| Pointer to encoded ARRAY\|I4 | SetTypeDescAlias returns `80020008` (DISP_E_BADVARTYPE); save/load never reached |
| Pointer to structured SAFEARRAY(I4) | Load succeeds; chain `001a -> 001b -> 0003` |
| Large nested array aliases | 5000 types reload; indices 0, 1500, 3000, 4999 return `001a -> 001c -> 001a -> 001c -> 0003`, each array has one dimension and lower bound 0; outer bound 2, inner bounds 2, 1502, 3002, 5001 |

The encoded-pointee rejection tests the writer, not the patched reader. A test
asserting only that rejection or the ordinary SAFEARRAY round trip would not
demonstrate the reader patch. Schema 2 is therefore required before choosing
its regression-test expectations. For offsets, the native returned trees
are established, but schema 1 did not prove serialized boundary coverage.

## Schema-2 native observations (2026-11-08)

Both architectures again match apart from the header. The isolated reader
processes exit 0: LoadTypeLibEx and GetTypeAttr return S_OK and the public
descriptor is **VT_PTR -> (VT_ARRAY|VT_I4)** (`001a -> 2003`). The native reader
preserves the encoded VARTYPE; it does not expose the structured
`001a -> 001b -> 0003` chain returned for the ordinary SAFEARRAY control.
This contradicts production typedesc-pointee v2's normalization. The
[v3 candidate](../../../patches/candidates/oleaut32-typedesc-pointee-v3/README.md)
preserves that VARTYPE and adds the corresponding Wine test, pending native
L1 and cross review. The production patch and its validation marker remain
unchanged.

The ordinary schema-2 processes exit 2 only for the same recorded writer
rejection. The large case has 160000 bytes in each descriptor segment and
still returns the previously measured trees and bounds. Boundary counts:

| Reference kind | At/above 0x8000 | At/above 0x10000 | Low-word bit 15 set |
| --- | ---: | ---: | ---: |
| PTR/SAFEARRAY pointee | 7952 | 5904 | 4096 |
| CARRAY array-description offset | 7952 | 5904 | 4096 |
| CARRAY element typedesc | 3976 | 2952 | 2048 |

Thus the large generated library really exercises all three serialized
reference paths across both boundaries. These measurements establish a
Windows oracle for the offsets test; they are not L1 of a Wine assertion
test, which still needs to be written and run. No raw logs were committed.

## Current validation state

Both schema-2 architectures compile with MinGW GCC 14.2 and `-Werror`.
Schema-2 native reader semantics and boundary counts are now measured.
The ordinary run can still return 2 for the recorded writer rejection;
return both result sets even when this happens. A pointee Wine test exists in
the separate v3 candidate; its L1 remains pending. No production patch revision,
`winum-test` marker, or series change is claimed by these observations.
This initial bundle covers the two typedesc investigations only; the remaining
four batch-2 investigations are not claimed complete or included.

## Remaining runtime observations (2026-11-09)

The first typedesc bundle above is historical. Its completed candidates and
follow-up fixtures have their own pages. The next unknown contracts are now
packaged sequentially, one patch per commit, for a single offline VM visit:

- [rpcrt4 NdrStubCall3](../rpcrt4-ndrstubcall3/README.md): original WIDL/SDK
  functional fixture, separate NdrStubCall2 control and syntax-entry probe.
  Native acceptance is unknown; the current Wine implementation is win64-only.
- [mshtml open optional result](../mshtml-document-open-optional-result/README.md):
  separate result-pointer/NULL processes with fresh, initialized and own-stream
  loaded documents. A successful native control is required.
- The reviewed actctx reader from the threading-model candidate is also built
  for a native refresh after moving it to kernel32 and using the typed record.
  Its earlier 609-check native result is historical.

```sh
export RUNTIME_MEASURE="$HOME/.cache/winum-runtime-measure-$(date +%Y%m%d-%H%M%S)"
bash scripts/build-measure-bundle.sh "$RUNTIME_MEASURE" docs/wine-repro/batch2-measure/runtime.list
```

Copy the full output directory to a fresh directory in offline Windows 10 and
run run-measure.bat once. Return all result-*.txt and exitcodes.txt; a ZIP
containing only those text files is fine. No additional compiler, SDK, CAD
program, generated proxy DLL or network is needed in the VM. The NDR metadata
is checked-in output generated from our own IDL by open Wine 11.20 WIDL.
Do not send the executables or any system DLL from Windows.

The actctx program is an assertion test. The NDR/MSHTML programs are MEASUREs:
zero exit alone cannot establish their expected semantics or native L1.
Unknown values must come from the returned native results before the Wine
conformance tests are written. Partial/crashed processes must also be returned.

## Runtime bundle returned and next native run (2026-11-09)

The reviewed actctx reader passed native L1 on x86_64/i686 (575 checks,
zero failures, exits 0). No repeat of that entry is needed. The mshtml
MEASURE established S_OK for supplied/NULL output pointers in all three
states on both architectures. Its new loaded-state Wine assertions need
native L1. The NDR controls faulted before the server; those results are
not an oracle for valid dispatch. The unchanged generated-stub diagnostic
is the next limited fixture check. Topic pages record the results and limits.

```sh
export RUNTIME_FOLLOWUP="$HOME/.cache/winum-runtime-followup-$(date +%Y%m%d-%H%M%S)"
bash scripts/build-l1-mshtml-open.sh "$RUNTIME_FOLLOWUP-mshtml-parity"
bash scripts/build-measure-bundle.sh "$RUNTIME_FOLLOWUP" docs/wine-repro/batch2-measure/followup.list
```

The first command verifies that the standalone mshtml function is identical
to the candidate patch; the second builds the single combined run directory.
Copy the complete RUNTIME_FOLLOWUP directory to a fresh directory in offline
Windows 10, run run-measure.bat, and return all result-*.txt and exitcodes.txt,
including failures. This is eight EXEs (four entries, both architectures),
not a repeat of all prior measurements. Do not copy any Windows DLL back.

The successful mshtml assertion result should be 34 checks, zero failures,
exit 0. The RPC entries remain diagnostic observations: a zero exit without
server dispatch is not functional coverage. If the generated control faults
too, do not infer native dispatch behavior from either wrapper result.


## Follow-up bundle returned (2026-11-09)

Both architectures passed the mshtml assertion L1 (34 checks, zero failures,
exit 0), also confirmed by the separately returned dedicated L1 outputs.
That entry does not need another native run. The unchanged generated NDR
control faulted on both architectures just like the dispatch-wrapper
controls; the x64 NdrStubCall3 probe also faulted. i686 NdrStubCall3 was absent.
The minimal-channel fixture has no working native control and is blocked.
Do not repeat followup.list unchanged or derive a conformance oracle from
those faults. The next RPC fixture must use a real COM marshaling path;
these completed mshtml results remain valid independently of the RPC failures.


## Third RPC bundle returned; native gate stopped (2026-11-09)

The real-COM generated and NdrStubCall2 controls now succeed on x86_64/i686:
S_OK, value=42, one server call on its STA creator and balanced observed
object references. The x64 NdrStubCall3 probe reaches its wrapper but not
our method or DISPATCH_DONE; the client receives RPC_E_SERVERFAULT. i686
has no export. All six processes exit 0 because they completed observations;
the probe's functional=0 is not a passed conformance test.

The third iteration provides working controls but no accepted native
NdrStubCall3 fixture. That gate remains blocked and this fixture direction
is stopped; do not request another guessed-metadata bundle. The topic
[conclusion and L3 proposal](../rpcrt4-ndrstubcall3/README.md#conclusion-and-stop-point)
records the exact limit and best hypothesis. No exception is converted into
a success-path assertion or l3-only waiver. Mshtml and the reviewed actctx
reader retain their completed native L1 gates. Production series is unchanged.
