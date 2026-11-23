# ucrtbase-cxx-exception-size.patch

<!-- winum-test: l1 -->

- **DLL:** `ucrtbase` (code in `dlls/msvcrt/except.c`, shared by the CRTs; also reached through `vcruntime140`)
- **Status:** In the winum-11.22-2 series. [Candidate history](../../patches/candidates/ucrtbase-cxx-exception-size/README.md)
- **Target:** winum-11.22-2

## Symptom

In AutoCAD 2027, Save As ended the whole program: the process stopped
with a fail fast (`0xc0000409`) inside CoreCLR, without an error dialog.
`RASTER` with some inputs ended the same way.

## Cause and fix

A caller asks `__CxxQueryExceptionSize` how much room it must reserve for
the frame record it then hands to `__CxxRegisterExceptionObject`. Wine's
x86-64 UCRT answered 16, the size of the exception type descriptor, but
`__CxxRegisterExceptionObject` writes a record of four pointers (32 bytes).
The caller's 16-byte buffer on the stack was overrun by 16 bytes, and the
stack cookie check (/GS) turned that into the fail fast.

Windows 10 x86-64 returns 32 from `ucrtbase` and `vcruntime140` (measured;
legacy `msvcrt.dll` has no such export). The patch returns the size of the
record that registration writes, for x86-64 UCRT builds only. i386 (where
both structures are 16 bytes), legacy `msvcrt` and ARM64EC keep the old
value.

In plain words: Wine told the program a box was half as big as what it
then put into it; the program's own safety check saw the spill and
stopped.

## How it was found

AutoCAD's crash address resolved to `coreclr.dll`; a trace of CRT calls
during Save As showed the size query right before the fail fast. An A/B
run with only `ucrtbase.dll` changed, in the same prefix and .NET, turned
the crash into a normal Save As
([record](https://github.com/Harvi654/winum/commit/d108da9)). Measured on
Windows 10 with a MEASURE program
([commit](https://github.com/Harvi654/winum/commit/3a3dda4)).

## Wine test

`dlls/ucrtbase/tests/cpp.c` checks the returned size on x86-64 without
constructing an exception (the test was first kept in
`patches/tests-first/ucrtbase-tests-cxx-exception-size.patch` and moved
into this patch in v3).

## Validation

| Level | Result |
| --- | --- |
| L0 | Applies with `--fuzz=0` after the full series on Wine 11.22 (2026-11-23); built x86_64 and i386 in the paired run [44905557665](https://github.com/Harvi654/winum/actions/runs/44905557665) and in the combined 11.22-2 build [45064416572](https://github.com/Harvi654/winum/actions/runs/45064416572) |
| L1 | Passed on offline Windows 10: `ucrtbase:cpp` x86_64 43 checks, i686 36 checks, 0 failures (maintainer, 2026-11-22) |
| L2 | [Run 44905557665](https://github.com/Harvi654/winum/actions/runs/44905557665): 8 of 8 UCRT units on both architectures, paired with the baseline; the new assertion fails on the baseline (16) and passes with the patch, zero new failures |
| L4 | AutoCAD 2027: Save As, Open, QSAVE and the file dialog with the candidate (2026-11-21/22); combined 11.22-2 build: AutoCAD passed (maintainer, 2026-11-23). Inventor 2027 with the combined build: start, part, assembly, drawing, save/reopen, restart passed (maintainer, 2026-11-22). SOLIDWORKS deferred under AGENTS.md |
| L5 | Pending |
| Cross review | Written by Codex; read in full and approved by Claude Opus 5.5 (2026-11-22), see the candidate README |

## Notes for a Wine bug report

AutoCAD 2027 (CoreCLR 10) calls `__CxxQueryExceptionSize` and passes a
buffer of that size to `__CxxRegisterExceptionObject`. Wine x86-64 UCRT
returns 16, Windows 10 returns 32; the registration writes 32 bytes.
Public synthetic test: the `cpp.c` check above.

## History

- v1 2026-11-21: candidate, AutoCAD A/B.
- v2 2026-11-21: ARM64EC excluded explicitly after review.
- v3 2026-11-23: test moved into the patch; promoted to the series.
