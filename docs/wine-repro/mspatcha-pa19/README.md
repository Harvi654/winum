# mspatcha PA19: bounded native observations and decoder fixture gap

Original measurement, AI-assisted (Codex), LGPL-2.1-or-later. The production
patch is a third-party Wine MR 3870 port plus the project's large-old-file
change; this program does not copy that patch's implementation or fixtures.
No production patch, validation marker or series entry changes.

## What this MEASURE covers

GetFilePatchSignatureByBuffer and NormalizeFileForPatchSignature are loaded
at runtime from mspatcha.dll. Five observations use only original generated
data: a 1024-byte raw pattern with CRC/MD5 flags, a 9 MiB+17-byte raw pattern,
and a hand-constructed minimal PE32 image with default and normalization-
disabling flags. PE layout comes from public IMAGE_* declarations; the own
RET byte is never executed or loaded as a module. Each API receives a fresh
mutable copy. The program prints results, last-error values, changed-byte
counts and only bounded, validated hexadecimal signature strings.

Native normalization results and signatures are unknown. The last-error
sentinel can be preserved on success; that alone is not a failure. Native
acceptance of the minimal PE must be observed before deriving assertions
from it. Raw controls and completed API calls alone do not validate the
whole production patch. A 30-second process watchdog bounds execution.
Both architectures build with -Wall -Wextra -Werror; the final batch list
contains mspatcha-signature. Return both outputs and exitcodes.txt.

## What this does not cover

This is **not** a PA19 apply/decode fixture. In particular, a signature API
handling a large buffer does not exercise an old file exceeding the LZXD
reference window. It does not prove PE relocation/import/resource fixups,
interleave maps, decompression, reference-prefix choice or reconstructed
file correctness. CASES explicitly prints pa19_decode_covered=0.

The repository has no original reproducible PA19 encoder/fixture for those
paths. Wine's mspatcha tests explicitly note the missing mspatchc producer;
their existing opaque patch arrays are not newly original fixtures for this
task. Calling Microsoft's CreatePatchFile tools to manufacture new patch
files would violate this batch's requirement to create inputs without
Microsoft tools. No Autodesk update, third-party patch bytes or Windows DLL
is copied into this measurement or requested from the maintainer.

## Proposed decoder L3 and remaining native gate

Before a meaningful decoder L3, write an original reproducible PA19/LZXD
producer, with documented framing, checksums and a decoding reference match.
Its matrix should distinguish PE32 normalization enabled/disabled, old
length below/equal/above the 8 MiB reference window, and data that actually
references the old prefix. Literal-only output cannot prove prefix choice.
Compare complete reconstructed bytes to the producer's original new file,
include wrong-old/corrupt/truncated controls and bounded cancellation, and
pair unchanged Wine against the production patch. Do not call the current
signature observations that L3 or report it passed.

A valid original producer would also permit native ApplyPatchToFile* MEASURE
without copying vendor content; that is not established here. Therefore
full-patch L1 remains blocked, and no l3-only exception is claimed or test
marker changed. Native counterparts exist. The independently measurable
signature/normalization subset can gain a Windows-derived test after these
outputs return, with its smaller scope stated explicitly.

Read for preparation: patch/page, changed signature API implementations,
normalization entry, PA19 header/old-file matching and large-reference-window
branch, LZXD framing and the existing apply_patch test's producer limitation.
The broad third-party normalization/decoder port was inspected in relevant
sections, not audited line by line as a cross review. No native internals or
proprietary binary contents were inspected. Raw measurement logs will not
be committed.

## Returned x86_64 observations and incomplete i686 output (2026-11-09)

The supplied x86_64 output completes all five cases with api_errors=0 and
exit 0. Both exports are available. Every successful call retains the
last-error sentinel 0xdeadbeef. Signature results are terminated hex strings:

| Case / flags | Signature | Signature changed bytes | Normalize result / changed bytes |
| --- | --- | ---: | --- |
| raw-small / 0 | 6fea9368 | 0 | 1 / 0 |
| raw-small / SIGNATURE_MD5 | d4883a545d61557ce12ff4e95350e997 | 0 | 1 / 0 |
| raw-large / 0 | af841acf | 0 | 1 / 0 |
| pe32 / 0 | 864575c5 | 6 | 1 / 0 |
| pe32 / 00e50000 | e2fe48a8 | 0 | 1 / 0 |

Do not equate standalone normalization with the normalization performed by
signature generation: this fixture's PE32 signature call changed six bytes,
while the independent Normalize call did not. Fresh input copies separate
these observations. The buffer sizes, synthetic PE and flags are defined in
measure.c; none of these values establish PA19 decoding or window-reference
correctness.

The batch records i686 exit 0, but no i686 text was supplied; the screenshot
shows a zero-length output and Program Compatibility Assistant for that
EXE. There is no MEASURE header or completed i686 observation to accept.
A successful process exit or the assistant's installed-correctly response
does not replace API output. The response does not enable compatibility
mode, and already written text results remain usable. The precise cause of
missing i686 stdout has not been established.

The bundle builder now embeds an original asInvoker/Windows-10 manifest
in both architectures of our measurement EXEs. This declares execution
level and supported OS explicitly rather than relying on installer-name
heuristics. It is metadata for our own EXEs; no runtime code or native API
expectation is changed by it. Both resource tables and the embedded XML were
checked in the built own binaries. Use a fresh VM folder for a targeted
repeat of this entry; return both architectures so any manifest-dependent
observation is visible. If i686 remains empty, preserve its exit code and
last visible launch state rather than retrying with compatibility settings.
Native assertion L1 and the full decoder gate remain open.

## Completed two-architecture repeat (2026-11-09)

The targeted follow-up returns complete x86_64 and i686 text, identical apart
from bitness. Both exports are present, all five cases complete, api_errors=0
and both processes exit 0. All signatures, changed-byte counts and last-error
values match the x86_64 table above, including the six changed bytes in the
default PE32 signature call and zero in its independent Normalize call.
The manifest-bearing bundle therefore supplies the missing i686 observations.
This successful repeat does not establish why the earlier stdout was empty.

These are native MEASURE results, not an assertion L1 pass. PA19 apply/decode
coverage remains explicitly zero, and the full-patch producer/fixture gap
above remains open. No production marker, implementation or series change.

## Signature-subset assertion candidate

The [v2 candidate](../../../patches/candidates/mspatcha-signature-v2/README.md)
adds an original Wine signature.c test with all five native-derived cases.
The production implementation diff is retained unchanged. Its entire test
helper/function block is checked against the standalone source by
scripts/build-l1-mspatcha-signature.sh. Expected successful native execution
is 50 checks, zero failures and exit 0 on each architecture; that assertion
run is still pending. The summary explicitly prints pa19_decode_covered=0.

This makes the independently measured API subset reviewable without
pretending it covers the decoder or solves the original large-reference
fixture gap. No full-patch L1, l3-only exception or production marker upgrade
is claimed. The native test requires only our generated buffers, not PA19
data, Microsoft tools, vendor files or an application.
