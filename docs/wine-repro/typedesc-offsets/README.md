# Typedesc offsets: native-writer record fixture (2026-11-09)

Original, AI-assisted (Codex), LGPL-2.1-or-later. The original program is a MEASURE;
the companion assertion test now uses its returned Windows values. Native
assertion L1 passed on both architectures (2026-11-09). Only public APIs and a read-only
audit of our own generated type library are used. No vendor binary, symbol,
source, registry export or native implementation is inspected.

## Evidence and current limit

Two preceding constructions did not yield an honest complete reader test:

1. The native 5000-alias nested-CARRAY construction returned the intended
   trees and boundary coverage on Windows, but triggers a separate open Wine
   writer lifetime issue before the reader is tested. Its draft is
   [preserved here](../../archive/typedesc-offsets-nested-writer-2026-11-09/README.md).
2. The modified serialized low/high fixtures returned the intended trees on
   Windows, then all four processes terminated before SUMMARY (x86_64
   0xc0000374, i686 0xc0000005). Partial successful getters are not conformance.
   That probe is [retired here](../../archive/typedesc-offsets-serialized-measure-2026-11-09/README.md).

This was the third construction attempt, with a stop required if it did not
bring the result closer. The complete native run below establishes a usable
oracle. No unearned l1/l3-only production marker is added.

## Construction

The program creates 5000 named records, each with one I4 field, and a matching
alias per record using ICreateTypeLib2/ICreateTypeInfo. Each alias is intended
to describe PTR -> CARRAY -> PTR -> USERDEFINED(record). Unlike the previous
native-writer design, the CARRAY element contains no nested CARRAY: recursive
serialization of the element cannot append another array-description record.
Unlike the retired serialized probe, no bytes or file metadata are modified.

After releasing the writer, the program copies the generated file to a fresh
temporary filename to avoid the writer's cache identity. A read-only audit
prints low-word bit-15 and high-word reference counts for all three reader
paths. The intended coverage requires `COVERAGE complete=1`; zero exit alone
does not establish it. It reloads that copy, reports the total type count,
samples aliases for records 0/1500/3000/4999, and reports returned descriptors,
bounds, referenced record attributes, field VARTYPE and generated names.
The returned native values are recorded below.

Before/after markers surround the public release and CoUninitialize calls.
They identify the API boundary if another abnormal exit occurs without
looking into native binaries. Both temporary files are deleted on normal
exit. A missing SUMMARY or nonzero/exception exit is incomplete measurement.

## VM bundle

The `tdoffset-records` entry in `docs/wine-repro/measure-bundle.list` is built
by **Build MEASURE bundle** and `scripts/build-measure-bundle.sh` for x86_64
and i686. Run run-measure.bat in a new directory in the offline Windows 10 VM.
Return result-tdoffset-records-x86_64.txt, result-tdoffset-records-i686.txt and
exitcodes.txt. Both EXEs compile with MinGW GCC 14.2, -Wall -Wextra -Werror;
no Windows runtime result is claimed from compilation.

No production implementation, patch page marker or series is changed.
The complete native observation and actual boundary coverage now support the
companion ok() reader conformance test. The other batch-2 patches remain
separate work.

A second-model read-only advisory checked the construction against open Wine
11.20. The CARRAY element follows only PTR/USERDEFINED writer branches and
does not reallocate the array-description segment recursively. No static
blocker was identified; this is not native execution or Claude's formal cross
review. Unexpected getter output can still fault during traversal, so the
process must reach SUMMARY and exit normally before its output is accepted
as a complete observation.


## Complete Windows MEASURE (2026-11-09)

Both x86_64 and i686 processes exited 0 with `SUMMARY api_errors=0
expectations=none`. All 40000 setup calls returned S_OK. SaveAllChanges and
LoadTypeLibEx returned S_OK; the library had 10000 types, a 160000-byte
descriptor table and an 80000-byte array-description table. The audit reported
`COVERAGE complete=1`, with identical counts on both architectures:

| Reader reference path | Low-word bit 15 set | Nonzero high word |
| --- | ---: | ---: |
| PTR/SAFEARRAY pointee | 4096 | 5904 |
| CARRAY array-description offset | 2048 | 904 |
| CARRAY complex element | 2048 | 2952 |

All four samples (0/1500/3000/4999, type indices 1/3001/6001/9999) returned
TKIND_ALIAS and PTR -> CARRAY -> PTR -> USERDEFINED. Every CARRAY had one
dimension, lower bound 0 and element count sample+2 (2/1502/3002/5001).
GetRefTypeInfo, GetTypeAttr, GetVarDesc and GetDocumentation all returned S_OK.
Each referenced record was TKIND_RECORD with one VAR_PERINSTANCE VT_I4 field,
size 4 and the generated name Record0/Record1500/Record3000/Record4999.
Numeric HREFTYPE handles differ by architecture; only their resolved record
identity is asserted. All release calls and CoUninitialize completed before
SUMMARY. The same batch's other six programs also exited 0. Raw uploaded
logs are not committed.

## Reader assertion candidate

The [v2 candidate](../../../patches/candidates/oleaut32-typedesc-z-offsets-v2/README.md)
adds the measured checks to oleaut32/tests/typelib.c without changing the
three production implementation hunks. `test-body.h` is the exact added block;
`l1.c` wraps it with a standalone assertion reporter. The dedicated builder
checks block identity before compiling:

```sh
bash scripts/build-l1-typedesc-offsets.sh /path/to/NEW-output
```

The generic bundle also builds `tdoffset-records-l1` for both architectures.
Return its two result files and exitcodes.txt after run-measure.bat finishes
in the offline Windows 10 VM. The successful MEASURE supplies the oracle; native assertion results are
recorded below.


## Windows assertion L1 (2026-11-09)

The dedicated bundle completed on offline Windows 10: x86_64 and i686 each
report `SUMMARY checks=40091 failures=0`, no FAIL lines and exit 0. This is
L1 of the same test body added to the v2 candidate. Cross review, Wine runtime
comparison, paired L2 and CAD checks remain pending; the production series
and its test marker are unchanged. Raw logs are not committed.

## Wine size-zero failure and narrow assertion correction (2026-11-09)

Run 42594173229 on candidates-4 was reported by the maintainer to fail the
combined record metadata assertion four times per architecture because
cbSizeInstance is zero. Source inspection identifies the writer: Wine's
record constructor sets size zero, AddVarDesc/LayOut do not compute a record
size, WMSFT_compile_typeinfo serializes that zero, and MSFT_DoTypeInfo copies
the stored value. The descriptor-offset reader changes do not set this size.

The fixture and all Windows values remain unchanged. Record kind/field
count are now a separate ordinary assertion; only the Windows size-4 check
is todo_wine for this independent writer gap. All offset/tree/bound/identity/
field assertions stay ordinary. The native wrapper expands todo_wine to an
ordinary assertion. A complete revised native run has 40095 checks; its
returned exact-body L1 result is recorded below. The original 40091-check
body's pass remains historical evidence.
The candidate README records the reasoning and remaining runtime gates.

## Corrected-body native L1 (2026-11-09)

Both architectures of the revised test pass on offline Windows 10:
SUMMARY checks=40095 failures=0, no FAIL lines and exit 0 each. The combined
bundle recipe at 8400e60 verifies this body against the candidate before
compilation; the assertion revision is 1ef60e0. The Wine-only size todo
expands to an ordinary assertion in the native wrapper, preserving the
measured size-4 contract along with every reader check. Revised native L1
is now passed, not inferred from the older 40091-check run.

Observed Wine behavior after the correction, paired L2 and formal review
remain open. No new Wine runtime pass, production promotion or test-marker
upgrade is claimed. Only summarized text evidence is committed.
