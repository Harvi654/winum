# Cross review: msi transform NULL binary field v2 (2026-11-10)

Reviewer: Claude; candidate author: Codex. Candidate as on main `f64f3b9`.

Read in full: the candidate patch and README, the production patch, and
Wine 11.20's `get_transform_record` (the loop around both hunks) and
`MSI_RecordReadStream`. The candidate's `table.c` hunk is byte-identical
to production. The MEASURE README was read for the native rows; the
standalone L1 harness and builder were not re-read. The rest of `db.c`
was skimmed around the insertion point.

## Findings

- **P1: none.** For a binary column Wine opens the row's stream
  unconditionally; when it is missing, the patch leaves the field NULL
  and continues instead of dropping the whole record. The raw-data offset
  of the binary column has already been advanced before the open, so the
  following columns are read from the right place.
- **P2: none in the test.** It builds the transform with public IStorage
  calls (class, string pool and data, the table stream with two rows, the
  key-2 blob stream, no key-1 stream) and checks exactly the measured
  values: two rows, both notes, the NULL row NULL with
  `MsiRecordReadStream` = `ERROR_INVALID_DATA` and size 0 (Wine's
  `MSI_RecordReadStream` gives the same for a NULL field, so the patched
  build can pass), and the blob row's four bytes. On vanilla the NULL row
  is dropped, so `rows == 2` and the seen mask fail; the call order and
  cleanup are balanced, including `CoInitialize`/`CoUninitialize` (S_FALSE
  when START_TEST already initialised is still a success).
- **P3: the workaround is wider than the measurement.** Any failed
  `OpenStream` now becomes a NULL field. Windows was measured for an
  absent stream only; whether Windows looks at the row's raw value for the
  binary column (0 here) and fails for a non-zero value with a missing
  stream, or for a corrupt stream, is not known. The README and header
  state this limit; it should stay in the patch page's open points.
- **P3: the stream-name encoder** follows MSI's compressed name alphabet,
  which Wine's own `encode_streamname` also implements; the test version
  is short, original and separately written. No concern, noted for the
  originality question AGENTS.md asks about.

## Checks and verdict

All series entries apply at `--fuzz=0` with this candidate substituted,
also with every current candidate together (`test-run/all-1`, 64
entries). Native assertion L1: 43 checks, 0 failures, both architectures
(README, 2026-11-09). Paired L2: the `msi` group runs in every test run;
`test-run/all-1` (run 42788810738) is the first that includes this
candidate. No Wine runtime run by this reviewer.

Verdict: **approve.** Promotion after `test-run/all-1`'s paired L2 shows
`msi:db` without a new failure (the vanilla side runs vanilla's tests, so
the new test appears only as more checks on the series side).
