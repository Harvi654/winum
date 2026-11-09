# Activation-context COM threading model: Windows MEASURE

Original, AI-assisted (Codex), LGPL-2.1-or-later. Text output only; native
Windows values were returned on 2026-11-09. This measures the batch-2 patch,
`ntdll-actctx-threadingmodel-case.patch`, and supplies the candidate conformance assertions.
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

Both architectures compile with MinGW GCC 14.2, -Wall -Wextra -Werror.
Compilation alone did not establish native execution; the completed Windows
MEASURE is recorded below. The main
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
the revised v2 candidate now adds its matrix there beside the existing COM
redirection checks, using the same public API path. It
checks the changed case-folding behaviour and retains unaffected controls,
with independent discrepancies marked todo_wine. Its standalone native L1
EXEs are compiled for both architectures; the native assertion results are
recorded below. Full-series application and focused Wine build checks are recorded in the
candidate README. Formal cross review and paired runtime checks remain open. The remaining batch-2 patches are separate work.


A second-model read-only advisory checked the complete program against the
open Wine parser and activation-context tests. Its suggested runtimeVersion
control was added before native measurement. No other blocker or conspicuous
copied block was identified; this does not prove originality or constitute
formal cross review. The complete native observations and assertion-test state are recorded below.


## Complete Windows MEASURE (2026-11-09)

Both architectures exited 0 and returned identical output apart from the bitness
header: attempted=42, created=34, rejected=8, queried=34, query_failed=0,
setup_errors=0, complete=1. All 42 CASE_END records completed. For each accepted
manifest, FindActCtxSectionGuid succeeded and the copied prefix had format 1,
record size 120 and flags 0. Reported data lengths were 120 for comClass and
226 for clrClass; the test asserts a readable prefix, not that total length.

| Attribute value | comClass | clrClass |
| --- | --- | --- |
| Apartment (title/lower/upper/mixed) | accepted, model 1 | accepted, model 1 |
| Free (title/lower/upper/mixed) | accepted, model 2 | accepted, model 2 |
| Both (title/lower/upper/mixed) | accepted, model 4 | accepted, model 4 |
| Neutral (title/lower/upper/mixed) | accepted, model 5 | accepted, model 5 |
| Omitted | accepted, model 3 | accepted, model 4 |
| Empty | rejected, error 14001 | rejected, error 14001 |
| WinumInvalid | rejected, error 14001 | rejected, error 14001 |
| Leading/trailing space around Free | rejected, error 14001 | rejected, error 14001 |

14001 is ERROR_SXS_CANT_GEN_ACTCTX. These eight rejections were observations,
not failed setup. No COM server or CLR execution was needed. Raw uploaded
logs are not committed.

## Windows-derived assertion test

The [v2 candidate](../../../patches/candidates/ntdll-actctx-threadingmodel-case-v2/README.md)
keeps the production parser change and extends kernel32/tests/actctx.c.
The case-folding assertions use ordinary ok(); independent rejection/default
differences use todo_wine. test-body.h is the exact integrated test function, without delimiter markers.
l1.c supplies a native-only reporter which treats all assertions, including
todos, as ordinary checks. It is not a substitute for Wine's runtime harness.

```sh
bash scripts/build-l1-actctx-model.sh /path/to/NEW-output
```

Run run-measure.bat from the resulting directory in the offline Windows 10 VM
and return result-actctx-model-l1-x86_64.txt,
result-actctx-model-l1-i686.txt and exitcodes.txt. The native assertion results below record the L1 gate. The successful MEASURE alone does not
establish native execution of these new assertions. The main bundle also
contains actctx-model-l1; l1.list can build just that entry.


## Windows assertion L1 (2026-11-09)

Both offline Windows 10 runs (x86_64/i686) report
`SUMMARY checks=609 failures=0`, no FAIL lines, and exit 0. These are the
pre-review shared Wine test assertions, including the rejection and
omitted-model controls that are todos only under Wine. The revised direct
structure reader has not yet been rerun natively; these 609-check results
remain historical. Raw logs are not committed. The
production series and test marker are unchanged pending candidate review
and Wine runtime validation.


## Review placement/reader follow-up (2026-11-09)

The matrix now lives beside test_find_com_redirection in
kernel32/tests/actctx.c. It uses the existing comclassredirect_data structure
directly, with pointer/length checks, and removes the BEGIN/END delimiters.
The standalone builder locates the named function instead of markers and
checks its equality with test-body.h. The expected Windows values and cases
are unchanged. Both standalone architectures compile with -Wextra -Werror;
refreshed native assertion execution and kernel32.actctx L2 are pending.
The main bundle's actctx-model-l1 entry builds this revised reader.


## Revised reader Windows L1 (2026-11-09)

The maintainer returned the runtime bundle's x86_64 and i686 results for
the direct comclassredirect_data reader relocated to kernel32/tests/actctx.c.
Both report `SUMMARY checks=575 failures=0`, no FAIL lines, and exit 0.
This supersedes the pending native refresh above; the earlier 609-check
result remains evidence for the historical ReadProcessMemory version only.
All todo_wine checks execute as ordinary assertions in the native harness.
Kernel32 actctx paired L2 and consumer validation remain open. No production
patch, series entry or validation marker is changed. Raw logs are not committed.
