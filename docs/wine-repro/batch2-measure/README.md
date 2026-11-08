# Batch 2: Windows observations before regression-test expectations

Original, AI-assisted (Codex), LGPL-2.1-or-later. Text output only. No vendor
files, registry exports, proprietary data or network access are needed.
These programs have no hard-coded expected Windows return values.

## Type descriptions: pointee patch

`typedesc.c` builds its own type libraries using ICreateTypeLib2 and
ICreateTypeInfo, saves them to generated temporary files, releases the writer,
and reloads them through LoadTypeLibEx(REGKIND_NONE). It prints HRESULTs and
the returned public TYPEATTR/TYPEDESC/ARRAYDESC trees. Generated files are
deleted on normal exit. No Autodesk type library or binary is used.

The small cases distinguish a pointer to I4, a pointer to encoded
`VT_ARRAY|VT_I4`, and a pointer to a structured SAFEARRAY(I4). A native writer
may reject or normalize the encoded case. Such an outcome must be preserved
as evidence: it cannot be counted as coverage of the reader defect. A
synthetic writer/reader round trip is not assumed to preserve the encoding.
The program reports creation and load errors, rather than treating them as
successful observations. The unsupported-pointee fallback to VARIANT is not
asserted to be a Windows contract.

## Build and Windows run

```sh
bash scripts/build-measure-batch2.sh /path/to/NEW-bundle
```

Builds x86_64/i686 static MinGW EXEs with `-Wall -Wextra -Werror`, packages
source and SHA256SUMS. Copy the whole bundle into a new directory in the
maintainer's offline Windows 10 x64 VM. Run `run-measure.bat` once and return
its text results and `exitcodes.txt`. The script refuses to overwrite results.
No CAD installation is needed. A zero exit means the programmed API operations
completed; it does not establish any expected Windows type semantics.

## State

The two architectures compile with MinGW GCC 14.2. Native execution and
expectations are pending. No Wine test, L1 pass, production patch revision,
`winum-test` marker, or series change is claimed by this scaffolding.
This initial bundle covers the typedesc-pointee investigation only. The
remaining batch-2 investigations are not claimed complete or included.
