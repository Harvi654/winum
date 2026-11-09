# MSI NULL object in a transform: Windows MEASURE

Original, AI-assisted (Codex), LGPL-2.1-or-later. Expected native values are
unknown. This measures msi-transform-null-stream before writing assertions;
no production patch, marker or series entry changes.

The program creates only its own temporary database, with a three-column
WinumNull table (SHORT key, text note, nullable OBJECT). It constructs an
original compound-storage transform using public IStorage APIs and the MSI
stream-name/string-pool/row encoding described by open Wine's MSI code and
tests. It does not use an MSI from an application, copy an existing Wine
test fixture, call the unimplemented Wine transform generator, or require
Windows SDK tools or vendor data.

The transform adds two rows in one table stream. Key 1 has note null-row and
a NULL object marker; its binary stream is absent. Key 2 has note blob-row
and a four-byte W3M! binary control stream. Both contain all three fields,
so Wine must read the nullable binary field instead of simply omitting that
column from the transform record. The unchanged schema already exists in
the database. The CLSID and encoding are format identifiers, not values
guessed as native API results.

Output contains the apply result, row counts, keys, comparisons against our
fixed notes, MsiRecordIsNull, stream-read result/length and blob comparison.
Only fixed labels and numeric results are printed. No paths or general
database content is dumped. Successful setup does not prove the handmade
transform is valid: require an accepted transform and a populated binary
control before deriving the NULL-row contract. If native rejects the format,
correct the fixture first; do not convert rejection into a conformance test.
A rejected transform or failed populated-blob control makes the process exit
nonzero and CASES completed=0. Zero exit is completion of observation with
that control, not an assertion-test L1 pass.

All MSI/COM handles and own files are released, and an independent 30-second
watchdog terminates only this process if setup/apply/cleanup stalls. It does
not free state beneath a live worker. Abnormal process exit may leave the
three own temporary objects; such output is incomplete. Normal cleanup
errors are reported. The shared helper lives in batch3-measure.

The main measure-bundle.list and batch3-measure/measure.list include
msi-null-stream for x86_64/i686. Both build with -Wall -Wextra -Werror.
Run the final batch-3 bundle once in a fresh offline Windows 10 directory;
return result-msi-null-stream-*.txt and exitcodes.txt, including failures.
Only text returns from the VM. Native values, Wine test/L1, paired runtime
and cross review remain pending. Raw logs will not be committed.


A second-model read-only advisory checked both full original sources against
public Wine's MSI transform formats/tests. It found the format/control
plausible and handles balanced, and recommended reopening the committed DB
in direct mode and rejecting a failed transform/populated control as usable
coverage. Those changes are included. No conspicuous copied nontrivial
block was found; prescribed stream encoding is not proof of originality.
Native acceptance and expected NULL-field outcomes remain unmeasured.


## Windows observations (2026-11-09)

The x86_64/i686 results match apart from bitness; both exit 0 with complete
setup and CASES completed=1. The own transform applies with result 0. The
previously empty table contains two rows afterward:

| Key / note | Payload NULL | ReadStream result | Bytes | Content |
| --- | --- | --- | --- | --- |
| 1 / null-row | true | 13 (ERROR_INVALID_DATA) | 0 | no payload |
| 2 / blob-row | false | 0 | 4 | exact own W3M! blob |

Thus Windows retains the NULL-object row and its non-binary fields while
the populated binary control is valid. These are functional observations
of our original transform, not an assertion L1 pass. They provide the
regression oracle for a candidate test; production patch/series/marker
remain unchanged. Raw logs are summarized rather than committed.

## Windows-derived assertion candidate

The [v2 candidate](../../../patches/candidates/msi-transform-null-stream-v2/README.md)
keeps the production fix and adds the exact observed two-row contract to
the existing MSI db.c unit. Its shared helper/function block is compiled
unchanged by scripts/build-l1-msi-null-stream.sh for native assertion L1;
the builder verifies its equality to the patch. Assertions are ordinary
ok(), including the valid binary control and NULL stream's result 13.

The assertion fixture uses unique owned temporary files rather than the
MEASURE's directory and is therefore itself still subject to native L1.
Successful MEASURE is not promoted to an assertion pass. Cross review,
paired runtime and consumer gates also remain open; production page and
test marker stay unchanged until a native-tested candidate is promoted.
