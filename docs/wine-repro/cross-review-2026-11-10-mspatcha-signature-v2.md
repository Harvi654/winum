# Cross review: mspatcha signature test v2 (2026-11-10)

Reviewer: Claude; candidate author of the test: Codex. Candidate as on
main `f64f3b9`.

Scope: this reviews **the v2 additions** - the patch header, the new
`dlls/mspatcha/tests/signature.c` and its `Makefile.in` entry - and checks
that the implementation part is unchanged. It is not a review of the
decoder port from Wine MR 3870 (third-party LGPL code, about 3700 lines
of `pa19.c`, `lzxd_dec.c`, `md5.c`, `mspatcha_main.c`), which was only
skimmed for the two functions the test calls.

Read in full: the new test unit, the v2 header, the README, and in the
patched tree `GetFilePatchSignatureByBuffer` and
`NormalizeFileForPatchSignature`. Compared with production: the only
differences are the header and the two test files; every implementation
hunk is byte-identical. The standalone L1 harness and builder were not
re-read; the native results are taken from the README.

## Findings

- **P1: none.** The test checks five measured cases (raw 1 KB CRC32 and
  MD5, raw 9 MB + 17 bytes, a minimal PE32 with and without the fix-up
  options): the signature string, its termination inside the buffer, the
  number of bytes the call changed in the caller's buffer (6 for the PE32
  with fix-ups - Windows normalises in place, and the port does the same),
  an untouched last error, and `NormalizeFileForPatchSignature` returning
  1 without changing a fresh copy for the given base and time. All are
  ordinary `ok()`; no `todo_wine`.
- **P2: the test does not cover the reason the patch exists.** PA19
  apply/decode with 32-bit images and the old-file-larger-than-window
  path are untested (README, header). This subset cannot give the patch
  its `l1` marker; the production page needs either a decode test with
  a patch file built by open tools or a maintainer-dated `l3-only` reason.
  This is the open item for release, not a defect of v2.
- **P3: the decoder port itself has no second-model review** in this
  repository. Its provenance (Wine MR 3870, author named, LGPL) is stated,
  which is what AGENTS.md asks for third-party code; a full review is out
  of scope here.

## Runtime check by this reviewer (2026-11-10)

Local Wine 11.20 x86_64 build (Ubuntu, not the CI container), the test
unit and `include/patchapi.h` from the full series applied:

| | `mspatcha:signature` | `mspatcha:apply_patch` |
| --- | --- | --- |
| vanilla mspatcha | 50 tests, 31 failures (`ERROR_CALL_NOT_IMPLEMENTED` stubs) | - |
| with the patch | 50 tests, 0 failures | 2145 tests, 0 failures |

So the test fails on vanilla and passes with the patch, as required. Not
L2 (no i386, not paired in CI); `test-run/all-1` (group g5, run
42788810738) is the paired run.

## Checks and verdict

All series entries apply at `--fuzz=0` with this candidate substituted,
also with every current candidate together (`test-run/all-1`, 64
entries). Native assertion L1 for the subset: 50 checks, 0 failures, both
architectures (README, 2026-11-09).

Verdict: **approve the v2 test** as replacement of the production file.
It records L1 for signatures and normalisation only; the patch's own
marker stays open until the decode path has a test or a recorded
`l3-only` decision (P2).
