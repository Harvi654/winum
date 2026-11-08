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

## State

Both schema-2 architectures compile with MinGW GCC 14.2 and `-Werror`.
Schema-2 native execution, reader semantics and boundary counts are pending.
The ordinary run can still return 2 for the recorded writer rejection;
return both result sets even when this happens. No Wine test, L1 pass, production patch revision,
`winum-test` marker, or series change is claimed by this scaffolding.
This initial bundle covers the two typedesc investigations only; the remaining
four batch-2 investigations are not claimed complete or included.
