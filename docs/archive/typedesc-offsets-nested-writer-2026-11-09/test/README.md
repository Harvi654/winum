# Large MSFT typedesc offsets: Windows-derived test

Original, AI-assisted (Codex), LGPL-2.1-or-later, 2026-11-09.

`test-body.h` is the exact test block added to the replacement candidate
`patches/candidates/oleaut32-typedesc-z-offsets-v2/`. It uses Wine ok()
assertions in oleaut32/tests/typelib.c. `l1.c` supplies a small standalone
assertion counter for the same body, so the generic MEASURE bundle can build
it for x86_64/i686 without Wine headers or a Wine installation in the VM.
Zero exit and zero failures mean its assertions completed successfully;
native execution is still required before recording L1.

## Native oracle

The 2026-11-08 schema-2 [batch-2 measurement](../batch2-measure/README.md)
created and reloaded 5000 aliases in both architectures. Each sampled alias
has PTR -> CARRAY -> PTR -> CARRAY -> I4, dimensions 1, lower bounds 0,
outer count 2, and inner count index+2. Native samples 0, 1500, 3000, 4999
all returned S_OK with these values. Both descriptor segments were 160000
bytes. Pointer, CARRAY-descriptor and CARRAY-element references each included
values with low-word bit 15 set and values at/above 0x10000.

The test repeats the same construction with ICreateTypeLib2. Before loading,
it audits only its own generated file using the open Wine MSFT layout and
requires both boundary conditions for all three reference paths. It then
checks the native descriptor trees and bounds at all four sampled indices.
No vendor file, registry data or proprietary implementation is inspected.
There is no file modification in this test. Temporary files are deleted.

## Running and validation

The program is listed in `docs/wine-repro/measure-bundle.list` and built by
**Build MEASURE bundle**. Run the bundle's `run-measure.bat` in a fresh folder
in the offline Windows 10 x64 VM. Return the two
`result-typedesc-offsets-l1-<arch>.txt` files and `exitcodes.txt` with the
other bundle results. No CAD software is needed. The test body uses ordinary
ok() assertions; no unmeasured invalid-file fallback is asserted.

Both standalone EXEs compile with MinGW GCC 14.2, -Wall -Wextra -Werror.
The candidate is not in production; native L1 and runtime paired L2 are
pending. A vanilla reader can fault while loading its invalid descriptor
pointers: baseline execution must be isolated and an abnormal exit is a
failure, never an L1 pass. Do not replace the native expectations to make
such a reader pass.
