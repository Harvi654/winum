# Typedesc offsets: native-writer record fixture (2026-11-09)

Original, AI-assisted (Codex), LGPL-2.1-or-later. This is a MEASURE, with no
asserted Windows descriptor values or L1 claim. Only public APIs and a read-only
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

This is one further construction attempt. If it does not bring the result
closer, stop this direction under the three-attempt rule, record the evidence
and proceed to the other batch tasks. No unearned l1/l3-only production marker
will be added.

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
These return values are unknown until the native output arrives.

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
Only after a complete native observation and actual boundary coverage should
this construction become an ok()/todo_wine reader conformance test. The
other batch-2 patches remain separate work.

A second-model read-only advisory checked the construction against open Wine
11.20. The CARRAY element follows only PTR/USERDEFINED writer branches and
does not reallocate the array-description segment recursively. No static
blocker was identified; this is not native execution or Claude's formal cross
review. Unexpected getter output can still fault during traversal, so the
process must reach SUMMARY and exit normally before its output is accepted
as a complete observation.
