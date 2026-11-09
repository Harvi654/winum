# Activation-context COM threading model: Windows MEASURE

Original, AI-assisted (Codex), LGPL-2.1-or-later. Text output only; native
Windows values are not yet known. This measures the next batch-2 patch,
`ntdll-actctx-threadingmodel-case.patch`, before adding conformance assertions.
No production patch, series entry or test marker is changed.

## What is observed

For each case the program writes its own temporary assembly manifest,
CreateActCtxW creates a context, ActivateActCtx makes it current and
FindActCtxSectionGuid queries COM_SERVER_REDIRECTION for our synthetic CLSID.
It prints the returned record's size, flags and threading-model field.
Each context is deactivated and released before the next case; its manifest
is deleted. Every case uses a fresh generated filename and distinct assembly
identity. No context-handle return flag is requested from the query, so no
extra query reference needs releasing.

The 42 cases cover both native `comClass` (inside file) and managed `clrClass`
(at assembly scope), the two callers of Wine's changed parser. For each:

- Apartment, Free, Both, Neutral, each in title/lower/upper/mixed case;
- missing attribute, empty value and an unknown value;
- leading/trailing space around Free, to distinguish case folding from trimming.

Only manifest metadata is queried. The named synthetic DLL does not need to
be loaded, no COM instance is constructed and no CLR/.NET runtime is started.
No registry change, CAD installation or network access is needed in the VM.
All clrClass cases carry the same runtimeVersion metadata (v4.0.30319),
so rejection cannot be confounded by an omitted runtimeVersion attribute.

The three-DWORD record prefix layout is taken from open Wine 11.20
`dlls/ntdll/actctx.c` and `dlls/kernel32/tests/actctx.c`. It is not a MinGW
SDK declaration or an inferred native implementation. ReadProcessMemory
copies only that bounded prefix of the public API's returned record. Enum
values are printed without assigning expected model numbers from Wine.
No vendor binary, symbol or disassembly is inspected, and no external input
file or arbitrary CLSID can be supplied.

## Build and offline Windows run

Both architectures compile with MinGW GCC 14.2, -Wall -Wextra -Werror. This
is compile validation, not execution on Windows or Wine. The main
`docs/wine-repro/measure-bundle.list` includes actctx-model; for this topic
alone use its single-entry list:

```sh
export ACTCTX_MEASURE="$HOME/.cache/winum-actctx-measure-$(date +%Y%m%d-%H%M%S)"
bash scripts/build-measure-bundle.sh "$ACTCTX_MEASURE" docs/wine-repro/ntdll-actctx-threadingmodel-case/measure.list
```

Copy the whole output directory to a new directory in the offline Windows
10 VM, run run-measure.bat and return:

- result-actctx-model-x86_64.txt;
- result-actctx-model-i686.txt;
- exitcodes.txt.

The normal complete summary has attempted=42, setup_errors=0 and complete=1.
CreateActCtx rejections and query failures are observations, not assumed
assertion failures; their exact returned errors belong in the future oracle.
A zero exit alone therefore does not establish usable coverage. Inspect the
canonical controls and every accepted context's RECORD before interpreting
case variants. An ERROR line, missing SUMMARY, incomplete case or abnormal
exit is incomplete measurement. Return both files even if a process fails.
No native assertion L1 is claimed until a Wine test built from this output
has itself run on Windows.

## Test placement and remaining work

The existing open Wine activation-context tests live in kernel32/tests/actctx.c;
there is no ntdll/tests/actctx.c in Wine 11.20. After native observations, add
an isolated ntdll test for this parser, using the same public API path. A new test
must distinguish the changed parser from the case-sensitive vanilla result,
keep unaffected controls, and be compiled for both architectures for native
L1. Formal cross review, full-series application/build and paired runtime
checks then remain necessary. The remaining batch-2 patches are separate work.


A second-model read-only advisory checked the complete program against the
open Wine parser and activation-context tests. Its suggested runtimeVersion
control was added before native measurement. No other blocker or conspicuous
copied block was identified; this does not prove originality or constitute
formal cross review. Native execution and all expected model values remain
pending.
