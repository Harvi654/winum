# Cross review: oleaut32 MSFT type description offsets v2 (2026-11-10)

Reviewer: Claude; candidate author: Codex. Candidate as on main `f64f3b9`
(file carries the v3 test revision; the folder stays `-v2`).

Read in full: the candidate patch (three `typelib.c` reader hunks and
`test_record_offsets` with its helpers) and its README. Read in Wine 11.20
with the full series applied: the type description reader loop in
`MSFT_ReadAllTypeDescs`' caller (`ITypeLib2_Constructor_MSFT`, both passes,
including the pointee-v2 simple-pointee branch next to the first hunk).
The implementation hunks were compared with production: identical. The
standalone L1 harness and builder were not re-read; the native results are
taken from the README.

## Findings

- **P1: none.** The three sites now build the 32-bit offset from both
  16-bit halves (`MAKELONG` takes them as WORDs, so the low half's sign no
  longer matters) and divide by the 8-byte entry size. The "simple type"
  tests that decide between a base type and an offset (`td[3] < 0`,
  `td[1] < 0`) are unchanged, and an offset can never reach 2 GB inside a
  type library, so the high half of a real offset is never negative. The
  pointee and array-element indices are bounds-checked against `cTD`; the
  CARRAY offset is stored whole for the second pass, as before.
- **P2: none.** The behaviour only changes for offsets from 0x8000 on,
  where vanilla read before the table or lost the high word, i.e. where
  vanilla was wrong. Libraries with small tables read exactly as before.
- **P3: two different fallbacks for out-of-range indices.** A bad pointee
  becomes `VT_VARIANT` (as the pointee-v2 branch does), a bad array
  element stays `VT_EMPTY` from `calloc`. Neither is measured, both only
  apply to corrupt files, and neither is asserted (README). Could be made
  the same; not a blocker.
- **P3: style.** The test is wrapped in `BEGIN/END winum` comment markers;
  other candidates avoid markers in Wine source. Harmless for us, would
  go in an upstream-style version.

## Test

The fixture is built with public `ICreateTypeLib2` calls only: 5000 I4
records and 5000 aliases `PTR -> CARRAY[i+2] -> PTR -> USERDEFINED`, with
distinct bounds and references so the writer cannot share descriptors.
Before loading, the test reads the saved file and requires that all three
reader paths actually contain offsets with bit 15 set and offsets of
0x10000 or more; otherwise it fails (no silent pass on a smaller writer
result). The copy to a fresh name avoids the loaded-library cache. Four
sampled aliases are walked with `ReadProcessMemory`, so a wrong pointer
from vanilla gives a failure, not a crash in the test itself (a fault
inside Wine's reader remains possible on vanilla, which is also a
failure). The only `todo_wine` is the record size 4: Wine's writer leaves
`cbSizeInstance` at 0, an independent writer gap explained in the README
and the v3 paragraph; every check the fix is about stays ordinary.
Cleanup releases the writer, type infos, attributes, var desc, name and
both files.

One open runtime point: 10000 type infos through Wine's writer
(`AddRefTypeInfo`, `SaveAllChanges`) may be slow; `oleaut32:typelib`
should stay well inside the unit timeout. `test-run/all-1` (group g3, run
42788810738) will show the time in its "Slowest units" step.

## Checks and verdict

All series entries apply at `--fuzz=0` with this candidate substituted,
also with every current candidate together (`test-run/all-1`, 64
entries). Native assertion L1 of the split-assertion body: 40095 checks,
0 failures, both architectures (README, 2026-11-09). No Wine runtime run
by this reviewer.

Verdict: **approve.** Promotion after `test-run/all-1` shows
`oleaut32:typelib` with the four expected size todos and no new failure,
and an acceptable run time.
