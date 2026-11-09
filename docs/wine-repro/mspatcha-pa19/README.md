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
