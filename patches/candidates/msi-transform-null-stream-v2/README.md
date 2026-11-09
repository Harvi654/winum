# MSI transform NULL binary field: v2 candidate

Original, AI-assisted (Codex), LGPL-2.1-or-later. Candidate only; production
implementation and patches/series are unchanged.

## Native basis

The own compound-storage MEASURE applied successfully on offline Windows 10
x86_64/i686 on 2026-11-09. An absent binary stream represents key 1's NULL
OBJECT field; Windows retains its key and null-row note, reports NULL and
ReadStream ERROR_INVALID_DATA/zero bytes. The populated key-2 control has
blob-row note and exact four-byte W3M! contents. Both rows survive.
[Observation details](../../../docs/wine-repro/msi-transform-null-stream/README.md).

The new ordinary-ok test is added beside test_try_transform in
dlls/msi/tests/db.c, with private prefixed helpers and one call after that
existing test. Its transform schema/encoding/strings/rows/blob are identical
to MEASURE. It uses unique owned temporary files, including placeholders
overwritten by MSI/IStorage, instead of the original private directory.
The native L1 must validate that complete assertion fixture too; a measured
format alone does not count as assertion L1.

The test checks both rows, notes, NULL/read semantics and the positive binary
control. Vanilla's missing-stream branch drops the NULL row, so the count and
seen-mask assertions detect the target defect. This source-level prediction
still needs observed vanilla/patched runtime confirmation. No todos hide
the row-retention contract. The production implementation hunks are copied
unchanged, with an original test and a dated v2 header paragraph.

This regression does not prove that every failed OpenStream is a valid NULL
field: corrupt or inaccessible non-NULL stream handling remains unmeasured.
It does not change that broader production workaround's scope or grant a
release exception. Native assertion L1, cross review, paired MSI L2 and
consumer validation remain prerequisites for promotion.

## L1 bundle

```sh
bash scripts/build-l1-msi-null-stream.sh /path/to/NEW-output
```

The builder verifies the entire helper/function block byte-for-byte against
test-body.h without BEGIN/END markers, then compiles it unchanged with the
standalone reporter for x86_64/i686 using -Wall -Wextra -Werror. The wrapper
has an independent 30-second watchdog covering COM setup and cleanup.
Normal errors are assertions, not skipped passes; all MSI/COM/file resources
are released. A timeout may leave its own temporary files and is not a pass.

Run run-measure.bat in a fresh offline Windows directory and return both
result-msi-null-stream-l1-*.txt plus exitcodes.txt, even if incomplete.
Complete successful execution should report 43 checks and zero failures.
The main MEASURE list also contains the L1 entry; use the dedicated builder
to check body parity before accepting results. No vendor MSI or SDK tool is
needed. Native assertion execution is pending; no production marker upgrade
is claimed.

## Application and checks

Substitute this patch for msi-transform-null-stream.patch at its existing
series position, keeping every other production entry. Do not apply both
versions or change patches/series. All 62 entries apply with --fuzz=0 in a
fresh pinned upstream Wine 11.20 tree. Standalone builds on both architectures
and body parity pass. Focused DLL/test build results are recorded below;
no Wine runtime execution is claimed by compilation.

## Focused build and advisory (2026-11-09)

The full 62-entry series applies with --fuzz=0 using this candidate. Complete
msi.dll and msi_test.exe PE targets link for x86_64/i386. The built db.c is
byte-identical to the fresh final-candidate application. Source: upstream
Wine 11.20 commit 7298a818b79446bae153b8d5aa27235ab3703bf9, archive SHA-256
9f516d63e9f814f4dad70ce0a1943211b8a6236008ac3025cf58004091c661db. Configure
used --enable-archs=x86_64,i386 --without-x --without-wayland --without-freetype;
this is focused PE compilation, not a full graphical Wine build or runtime.
Both strict standalone builds, body-parity check and bundle hashes pass.

A second-model read-only advisory read the complete original test/harness
and candidate against the native observations and db.c insertion context.
It found the assertions aligned, ownership balanced and no conspicuous
copied nontrivial block. It highlighted the temp-placeholder setup difference,
which the pending exact-body L1 must validate. This limited advisory is not
proof of originality, formal cross review or native execution.

The builder optionally accepts a BUNDLE_LIST containing the exact MSI L1
entry, allowing the targeted batch-3 follow-up to share one VM visit with
the two incomplete measurements. It still verifies the full candidate test
block before compiling. The common builder now embeds an explicit original
asInvoker/Windows-10 manifest; the pending native assertion run validates
these actual EXEs, not an earlier build's metadata.
