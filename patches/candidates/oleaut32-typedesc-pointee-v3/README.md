# Typedesc pointee v3: native representation correction

Original, AI-assisted (Codex), LGPL-2.1-or-later. Candidate only; not submitted
upstream. Replaces the production `oleaut32-typedesc-pointee.patch` in a private
recipe, **not** an incremental patch to apply after it. No series or L1 marker
is changed. Cross review, native L1 of the assertions, paired L2 and CAD remain
pending.

## Native evidence and bounded correction

On 2026-11-08 the maintainer ran schema-2 batch-2 MEASURE in offline Windows
10 x86_64/i686. Both reader processes exited 0 and returned identical results
apart from architecture. On a fresh-path copy of our own PTR(I4) type library,
one byte changes the simple pointee encoding to ARRAY|I4. LoadTypeLibEx and
GetTypeAttr return S_OK; the returned alias is **VT_PTR -> 0x2003**. It is
not VT_PTR -> VT_SAFEARRAY -> VT_I4. Both native writers reject construction
of the encoded case with DISP_E_BADVARTYPE, so a writer-only test cannot
exercise the reader fix. No vendor library, disassembly or private symbols
were inspected. Raw logs are not committed.

Production v2 avoids the invalid std_typedesc index but normalizes the type
into a different public descriptor tree. V3 preserves the native encoded
VARTYPE in the already owned extra descriptor. Allocation/lifetime, ordinary
pointees and the unmeasured fallback for unsupported values remain unchanged.
This correction needs CAD validation: the old representation worked around
an application failure, and descriptor fidelity alone does not prove the
complete Invoke path works with the new representation.

## Test and L1

The patch includes a type library compatibility test in
oleaut32/tests/typelib.c that compares the stored type description with
Windows. It creates its
own alias library with ICreateTypeLib2, checks the ordinary PTR(I4) control,
then validates the eight-byte descriptor record in its own generated file,
changes one byte and loads a fresh-path copy. ReadProcessMemory bounds the test-side pointer read; vanilla can still fault
inside GetTypeAttr before that check.
The regression asserts the native returned VARTYPE with ordinary ok(), so
both vanilla's bad index and production v2's normalization should fail;
candidate runtime execution is not claimed until tested.

```sh
bash scripts/build-l1-typedesc-pointee.sh /path/to/NEW-bundle
```

The builder downloads only a pinned Wine 11.20 source archive (SHA-256 checked
before extraction) for test.h/debug.h, extracts the exact test block from the
candidate, and builds static x86_64/i686 EXEs with -Wall -Wextra -Werror. The
bundle contains the source and headers too. For an existing verified archive,
set WINE_L1_SOURCE_ARCHIVE on a separate export line before running it.

Copy the complete folder into a new directory in the offline Windows 10 VM
and run run-tests.bat. Return result-x86_64.txt, result-i686.txt and
exitcodes.txt. Acceptance requires both exits 0, zero failures/skips and a
completed summary. These are assertion tests, unlike the preceding MEASURE;
their expected values come from the returned native observations.

Promotion requires Claude's cross review, full-series applicability/build,
paired oleaut32 typelib/tmarshal L2, and the consumers' CAD smoke tests.

## Preparation validation (2026-11-08)

- All 62 production patches apply with `--fuzz=0` when the pointee entry is
  replaced by this candidate; the following z-offsets patch still applies.
- Both oleaut32 DLL targets and complete oleaut32_test.exe targets linked
  successfully (x86_64/i386), including the added typelib test. Configuration:
  `--enable-archs=x86_64,i386 --without-x --without-wayland --without-freetype`.
  This is compile validation from the upstream tag, not an official-archive,
  full-feature L0 runtime build or test execution.
- Both standalone native L1 EXEs compiled with MinGW GCC 14.2 and
  `-Wall -Wextra -Werror`, using the pinned Wine 11.20 headers. Bundle hashes
  verified. Native execution of these assertions is pending; the earlier
  MEASURE results are their oracle, not an L1 pass.
