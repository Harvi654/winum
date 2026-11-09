# Reader-only typedesc offset measurement (2026-11-09)

Original, AI-assisted (Codex), LGPL-2.1-or-later. Windows expectations for
this new serialized fixture are pending; it is a MEASURE, not a conformance
test or an L1 pass. No production patch, page marker or series is changed.

## Why the native 5000-alias construction is not reused in the Wine test

The [schema-2 native results](../batch2-measure/README.md) established the
5000 nested aliases' descriptor trees and offset-boundary coverage. Open Wine
11.20's WMSFT_append_arraydesc, however, keeps an address into arraydesc_seg
across a recursive append of a nested CARRAY. That append can realloc the
same segment, invalidating the saved address before the outer writes. The
original draft can therefore fail in the writer before reaching the reader
under test. Its source and compile checks are preserved in the
[blocked draft](../../archive/typedesc-offsets-nested-writer-2026-11-09/README.md).
No separate writer fix is introduced as part of this reader investigation.

## Fixture and scope

`measure.c` creates one small PTR(I4) alias with ICreateTypeLib2, which avoids
the nested-array writer path. `fixture.h` validates only that generated seed
against the open Wine MSFT layout. It leaves the original seed untouched and
builds a separate file with appended typedesc and array-description segments.
The alias root stays at typedesc index zero; original names, syskind and
pointer-sized type metadata remain unchanged.

The intended tree is PTR -> CARRAY -> PTR -> CARRAY -> I4, with both arrays
one-dimensional, lower bound zero, count two. Unreferenced typedesc slots
repeat the seed's valid PTR(I4) record; unreferenced array records describe
one-dimensional I4 arrays of two elements, with lower bound zero.
The low fixture places references at 0x8000/0x8008/0x8010; the high fixture
uses 0x10000/0x10008/0x10010. Pointer, CARRAY-description and CARRAY-element
references thus exercise the three reader paths without requiring large
alias counts or recursive writer calls. The program prints the references
it constructed and the HRESULTs/public descriptor tree Windows returns.
An accepted file and its returned tree are not presumed from this layout.

Only original synthetic data is serialized. No arbitrary input path or vendor
type library is accepted. Only text results leave the maintainer's VM.

## Build, run and checks

Both variants are in `docs/wine-repro/measure-bundle.list`:

```sh
bash scripts/build-measure-bundle.sh /path/to/NEW-bundle
```

The **Build MEASURE bundle** workflow builds them too. In a fresh directory
in the offline Windows 10 x64 VM run run-measure.bat and return the four
`result-tdoffset-{low,high}-{x86_64,i686}.txt` files plus exitcodes.txt with
the other bundle results. Each boundary/architecture is a separate process,
so a rejected or crashing fixture does not suppress the other observations.
Exit zero means API operations completed, not agreement with an assumed type.

All four EXEs compile with MinGW GCC 14.2 and -Wall -Wextra -Werror.
`test-fixture.c` checks boundary references, segment separation, seed
immutability and rejection of truncated/invalid seeds on the host. Those
checks validate serializer mechanics, not acceptance by Windows or Wine.

A read-only second-model advisory checked the serializer against the open
Wine layout and found no blocker to using it as a MEASURE. It cannot prove
originality or native compatibility. Native loading/GetTypeAttr or traversal
of a malformed returned descriptor can still fault; separate processes keep
that observation distinct from the other boundary/architecture cases. Such
an exit is incomplete measurement, never a conformance pass.

After the native output is returned, build the Wine assertion test from the
measured fixture behaviour, then run L1 and paired typelib/tmarshal L2 before
changing any production validation marker. Claude's cross review remains
required for a candidate. The other batch-2 patches remain separate work.
