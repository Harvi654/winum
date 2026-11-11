# mspatcha v2: Windows-derived signature subset test

> **Promoted 2026-11-11** into `patches/series` for winum-11.20-1. This folder stays as the record of the candidate.

Candidate only. Production patch, validation marker and patches/series are
unchanged. The implementation retains its Wine MR 3870 LGPL provenance;
only the new test/harness is original, AI-assisted (Codex), LGPL-2.1-or-later.
This is not a cross review of the third-party decoder port.

## Native basis and test scope

Our original [MEASURE](../../../docs/wine-repro/mspatcha-pa19/README.md) completed
on offline Windows 10 x86_64/i686 on 2026-11-09 with matching output. The new
Wine unit dlls/mspatcha/tests/signature.c uses the same generated raw pattern,
1024-byte minimal PE32 input, five flag/size cases and API arguments. It
asserts the measured CRC/MD5 strings, bounded string termination, mutation
counts, return values and unchanged last-error sentinel. Normalize receives
a fresh input copy and its own measured base/time arguments; it is not
presumed equivalent to signature generation's internal request.

The test uses ordinary ok(), so vanilla's unimplemented signature and
normalization exports fail it. No todo hides those contracts. The expected
patched result still needs runtime observation. A complete native assertion
run should execute 50 checks; the previous MEASURE is not assertion L1.
The original implementation diff is byte-for-byte unchanged. A dated v2
paragraph adds the measured basis, test and explicit decoder limitation.

**No PA19 decode coverage:** a large raw-buffer signature is not a reference-
window test. No patch file is produced or applied. PE relocation/import/
resource repair, interleave tables, malformed/truncated streams and the
large-old-file LZXD path remain untested. The original producer/fixture gap
and proposed decoder L3 are documented on the measurement page. Even a
native pass of this subset cannot close full-patch L1, grant an l3-only
exception or justify changing the production marker to l1.

## L1 build and execution

```sh
bash scripts/build-l1-mspatcha-signature.sh /path/to/NEW-output
```

The builder extracts the entire named helper/function block from the new
Wine test unit and checks byte equality with test-body.h before compiling
the unchanged body in the standalone wrapper. No BEGIN/END marker is added
to Wine source. Both architectures use -Wall -Wextra -Werror and the common
original asInvoker/Windows-10 manifest. The wrapper has an independent
30-second watchdog; a timeout exits 2 and is not a pass. Both raw buffers
and the dynamically loaded DLL are released; no files or windows are used.

Run run-measure.bat in a fresh offline Windows 10 folder, with normal caller
permissions. Return result-mspatcha-signature-l1-x86_64.txt,
result-mspatcha-signature-l1-i686.txt and exitcodes.txt, including failures.
The summary explicitly retains pa19_decode_covered=0. The entry is also in
the main measurement list; use the dedicated builder to verify test parity.

## Application and validation

Substitute this patch for mspatcha-pa19-32bit-and-large-files.patch at its
existing full-series position, preserving every other entry. Do not apply
both versions or edit patches/series. All 62 entries apply with --fuzz=0
on fresh upstream Wine 11.20 commit
7298a818b79446bae153b8d5aa27235ab3703bf9 (source archive SHA-256
9f516d63e9f814f4dad70ce0a1943211b8a6236008ac3025cf58004091c661db).
Applied Wine test-body parity, both strict standalone builds and all bundle
checksums pass. Complete mspatcha.dll and mspatcha_test.exe PE targets link
for x86_64 and i386, with the new signature unit included. Configure used
--enable-archs=x86_64,i386 --without-x --without-wayland --without-freetype.
This is a focused PE build, not a full graphical Wine L0 build. No Wine
runtime or native assertion execution is inferred from compilation.

A second-model read-only advisory compared the complete original test body,
harness and builder with both native outputs and MEASURE. It confirmed all
five fixtures/arguments/oracles match, 50 successful-path checks, balanced
ownership, bounded string access and explicit decoder exclusion. It found
no suspicious copied nontrivial block; public PE layouts are prescribed
format declarations. This is a limited advisory, not proof of originality,
formal candidate cross review or full third-party implementation review.

Observed vanilla/patched runtime, paired mspatcha L2, formal cross review and
full decoder validation remain open. Only summarized text results may be
recorded from Windows.

## Native assertion L1 for the subset (2026-11-09)

The maintainer's offline Windows 10 results for x86_64 and i686 both report
SUMMARY checks=50 failures=0 pa19_decode_covered=0, with exit 0. These are the
exact-body assertion executables prepared at c50734d, not the earlier MEASURE
observations. Native assertion L1 therefore passes for the signature and
normalization subset only. No raw logs or Windows binaries are committed.

PA19 apply/decode and the large-old-file reference-window regression remain
uncovered. This subset pass does not close the full-patch validation gate;
the production marker stays missing, and patches/series is unchanged.

## L2: test-run/all-1 (2026-11-10)

[docs/wine-repro/l2-all-1-2026-11-10](../../../docs/wine-repro/l2-all-1-2026-11-10/README.md), run 42788810738, with every current candidate: `mspatcha:signature` 50 checks, 0 failures, both architectures; `apply_patch` unchanged. Decode coverage still open.
