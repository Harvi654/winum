# Cross review: kernelbase-hkcr-class-reads (2026-11-24)

Reviewer: Claude Opus 5.5 (the patch and its test were written by Codex).
Reviewed: `topic/kernelbase-hkcr-read-measure` at 094a0d4.

Read in full: `patches/candidates/kernelbase-hkcr-class-reads/kernelbase-hkcr-class-reads.patch`,
`patches/tests-first/advapi32-tests-hkcr-class-reads.patch`, the candidate
README and `docs/wine-repro/kernelbase-hkcr-read/README.md`, against Wine
11.22's `dlls/kernelbase/registry.c` with `patches/series` applied (both
patches apply with `--fuzz=0`). Skimmed: `measure.c` (fixture and cleanup
logic only), the three workflows, `scripts/build-l1-kernelbase-hkcr-read.sh`.

## Verdict

Approved as a candidate for L0/L2, with two items to do before the
series (1, 2) and notes.

What the patch does matches the measured Windows 10 results in the
README (non-elevated, matching view): an HKCU-only CLSID key opens
through HKCR and relative to an HKCR\CLSID handle; in a collision the
user value wins and a machine-only value stays readable. It is limited to
read opens (`KEY_QUERY_VALUE` within `KEY_READ`, or exactly
`MAXIMUM_ALLOWED`) of `CLSID\` and `Wow6432Node\CLSID\` paths, with no
options; everything else goes through the old `open_key` path.

Checked:

- Handle tag: read handles get bit 1 set (`| 2`). Wine ignores the two low
  bits of a handle in the server (`handle_to_index` is `(h >> 2) - 1`)
  and in ntdll's fd cache, so `NtClose`, `NtQueryKey`, `NtEnumerateKey`
  and `NtDuplicateObject` work on the tagged value. No other kernelbase
  code tests `& 3 == 2` on handles (console handles use `== 1`).
  `RegCloseKey` closes it normally.
- Remapped predefined HKCR (`RegOverridePredefKey`): detected by the
  key name check and left on the old path.
- Relative opens from a tagged handle rebuild the physical path from
  `KeyNameInformation`, so a handle in the 32-bit view keeps its
  `Wow6432Node` component; explicit HKLM or HKCU handles are untagged
  and keep their provenance.
- `query_registry_value`: the first query on the backing handle keeps the
  caller's access check; the user value wins (also its buffer-size
  answer); a user `STATUS_OBJECT_NAME_NOT_FOUND` falls back to the
  machine value. Callers that loop on `STATUS_BUFFER_OVERFLOW` get a
  consistent size from the same hive on the second call.
- `RegOpenKeyExA`/`W` still return `ERROR_INVALID_HANDLE` for a bad root
  (`STATUS_INVALID_HANDLE` from `open_registry_key`).

## Before the series

1. **todo_wine in the test.** The tests-first patch marks the user-only
   opens and the collision values `todo_wine_if`. The fix does not
   touch the test, so with the fix applied these checks become
   "Test succeeded inside todo block" (failures) in L2. When the test
   moves into the fix's patch (tests-first README), remove those
   `todo_wine_if`; record which ones in the patch's v2 paragraph.
2. **L2 on a fresh Wine prefix.** The 32-bit view cases depend on the
   HKCU WOW64 layout (candidate README). Wine's test prefix is created by
   wineboot without the CI modelling; check in the paired L2 that the
   `KEY_WOW64_32KEY` cases do not fail on the series side for that
   reason, or note why they are skipped.

## Notes (no change needed for the candidate)

- Cost: every value read through a tagged machine-backed handle does
  `NtQueryKey(KeyNameInformation)`, `RtlOpenCurrentUser` and an
  `NtOpenKeyEx` on HKCU before answering. COM activation reads a handful
  of values per class, so this is small, but worth a look in the
  Inventor start-time number of the L4.
- Writes through a tagged handle opened with `MAXIMUM_ALLOWED` on a
  machine-backed key still go to HKLM (Windows writes to HKCU when the
  user key exists); writes, enumeration, `RegQueryInfoKey` counts and
  notifications are not merged. All documented as outside the
  measurement.
- A user-only key opened with `MAXIMUM_ALLOWED` (legacy `RegOpenKey`)
  is given `KEY_READ`; a caller that writes through it gets
  `ERROR_ACCESS_DENIED` where it used to get `ERROR_FILE_NOT_FOUND` at
  the open. Acceptable, documented.
- `root == HKEY_CLASSES_ROOT` compares the full pointer; a 64-bit caller
  passing a zero-extended `0x80000000` takes the old path (the rest of
  kernelbase compares with `HandleToUlong`). Harmless, but
  `HandleToUlong(root) == HandleToUlong(HKEY_CLASSES_ROOT)` would match
  the surrounding style.
- The collision test runs only when the L1 runner sets
  `WINUM_HKCR_COLLISION`; in Wine's own test run (L2) it is skipped.
  Under Wine the test process could create the HKLM fixture itself,
  which would let L2 cover the merge path too.
- Style nit: a blank line is missing before `test_hkcr_class_collision`.
- Integrity level: on Windows these reads were measured at Medium. At
  High, .NET's COM shim skips HKCU classes itself (see
  `server-token-integrity/`), so with the token integrity candidate the
  Nastran path needs both.
