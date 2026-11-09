# COM stub activation context: Windows MEASURE

Original, AI-assisted (Codex), LGPL-2.1-or-later. Native context-selection
observations were returned on 2026-11-09; aggregate results are below. This measures
`combase-stub-activation-context.patch` before adding conformance assertions.
No production patch, series entry or test marker is changed.

## Public-API experiment

The program implements its own non-agile IClassFactory. It supports only
IUnknown and IClassFactory, not IMarshal, IAgileObject or a free-threaded
marshaler. It uses CoMarshalInterThreadInterfaceInStream and
CoGetInterfaceAndReleaseStream to obtain two standard proxies in another
apartment. Both marshaled streams exist before either is consumed, keeping
external marshal references to the same object outstanding while its second
marshal is made. The two proxies stay alive until all calls finish.

The measured CreateInstance method observes the current activation context
and deliberately returns E_NOTIMPL with a NULL output. That is our probe's
chosen response, not an expected Windows context-selection value. Its server
callback count verifies that each client call actually reached our method;
relative thread checks reject a direct call on the client thread. No COM
class is registered and the method does not instantiate another object.
No generated proxy DLL, type library, runtime compiler or Windows SDK import
library outside MinGW is required.

Two own manifests declare distinct window-class markers A and B. The program
activates each context and verifies masks A=1/B=2 and baseline=0 after
deactivation before using it. At construction, marshaling, client call boundaries and inside the server
method, FindActCtxSectionStringW queries those markers. OBS mask bit 0 means
A is visible and bit 1 means B is visible. GetCurrentActCtx reports only
whether a handle is present and releases its acquired reference; handles,
paths and pointers are never printed. CoGetApartmentType reports the actual
apartment and qualifier, so an MTA callback need not run on its creator's
thread to establish the correct apartment.

The synthetic named DLL is never loaded; querying a window-class redirection
record does not create an actual window. No vendor binary, symbol, registry
export, CAD installation or network access is involved. No external file or
arbitrary class can be supplied.

## Scenario matrix

Both server-STA/client-MTA and server-MTA/client-STA run this sequence:

| Variant | Context during own object's construction | First marshal | Second marshal |
| --- | --- | --- | --- |
| 0 | none | none | none |
| 1 | A | A | A |
| 2 | A | B | A |
| 0 repeated | none | none | none |

Each client unmarshals both streams without an explicit test context, then
calls both proxies with no test context, with A, and with B. Thus eight
scenarios make six calls each (48 callbacks). Variant 2 distinguishes the
first-marshaling context from construction, a later marshal and the caller's
context. The final no-context control detects persistent contamination across
object scenarios; it cannot prove that the same MTA worker thread was reused.
The client observes markers before and after every call. The server creator
thread also observes its state before and after the client's lifetime.

The server STA pumps messages until the client has released both proxies and
uninitialized COM. An independent watchdog thread covers each complete
scenario, including COM setup and cleanup, with a 30-second limit. On timeout
it terminates only this measurement process with exit 2, without stdio or DLL
shutdown that could wait on the stuck thread. It does not use TerminateThread
or free state underneath an RPC call while leaving the process running.
The last SCENARIO record identifies the incomplete scenario; a watchdog exit
need not produce an ERROR or SUMMARY line. On normal completion both
contexts and own manifest files are released/deleted. An abnormal process exit
can leave its two generated temporary manifest files; it is not a passed run.

## Build and offline Windows run

Both MinGW architectures compile with -Wall -Wextra -Werror, static CRT,
-lole32 -luuid. Compilation is not native execution. The program is in the
main docs/wine-repro/measure-bundle.list; to build this topic alone:

```sh
export STUB_MEASURE="$HOME/.cache/winum-com-stub-measure-$(date +%Y%m%d-%H%M%S)"
bash scripts/build-measure-bundle.sh "$STUB_MEASURE" docs/wine-repro/combase-stub-activation-context/measure.list
```

Copy the complete output directory into a new directory in the offline
Windows 10 VM and run run-measure.bat. No Inventor or interactive window is
needed. Return result-com-stub-actctx-x86_64.txt,
result-com-stub-actctx-i686.txt and exitcodes.txt, even if a process fails.

Acceptance as a complete observation requires both exits 0, successful marker
self-checks, all eight SCENARIO_END records, 48 SERVER callbacks, no ERROR
lines and `SUMMARY scenarios=8 calls=48 setup_errors=0 complete=1
expectations=none`. Every CALL must report one callback, the chosen E_NOTIMPL
response and no returned object. Inspect callback apartment/thread fields
before treating it as the intended cross-apartment path. No particular server
marker mask, context handle presence or selection policy is presumed. Here
"none" means no explicit test context; the process may still have a default
activation context.

The future Wine test must be based on these returned observations, including
unaffected controls and context restoration. The production patch captures
an activation context when a stub manager is created and activates it around
IRpcStubBuffer_Invoke. This experiment tests that claim through actual calls;
metadata lookup on the exporting thread alone would not cover it.
Native assertion L1, formal cross review and paired Wine runtime/consumer
validation are not established by this MEASURE. Focused candidate build
validation is recorded in the candidate README.


A second-model read-only advisory checked the design and full program against
open Wine's class-factory proxy/stub, marshaling and activation-context tests.
It identified a message-pump timeout gap, addressed with the independent
watchdog, and recommended the positive/exclusive marker controls now included.
Its follow-up found no blocking COM/handle ownership issue. Watchdog stdio
was then removed to avoid waiting on a stuck thread's output lock. No
conspicuous copied block was identified; this is not proof of originality,
formal cross review or native execution.


## Complete Windows observations (2026-11-09)

The x86_64/i686 outputs are identical apart from the bitness header. Both exit
0: scenarios=8, calls=48, setup_errors=0, complete=1. All marker self-checks
passed. Every scenario completed six callbacks; each client call reported one
callback, E_NOTIMPL (the response deliberately chosen by our own object), NULL
output, and unchanged before/after masks. All GetCurrentActCtx and
CoGetApartmentType queries succeeded.

For both streams in every construction/marshaling variant:

| Server apartment | Client phase none | Client phase A | Client phase B |
| --- | ---: | ---: | ---: |
| STA callback marker | 0 | 0 | 0 |
| MTA callback marker | 0 | 1 | 2 |

STA callbacks ran on the creator thread, not the client, with apartment=3
(MAINSTA), qualifier=0. MTA callbacks ran on neither creator nor client, with
apartment=1 (MTA), qualifier=0. Context A/B during construction or the first
and second marshaling did not select a persistent callback marker. The
client's context was restored after every call; the server creator thread's
before/after baseline and final client baseline were 0. The no-context
controls also completed after the context-bearing scenarios. Raw logs are
not committed.

This observation does **not** validate the patch's broad first-marshaling
capture explanation. It is evidence for standard-marshaled IClassFactory in
these in-process STA/MTA cases, not every COM interface or native internal
mechanism. The candidate documents the local STA shortcut separately from
the MTA dispatch_rpc path. A paired Wine comparison must establish actual
baseline/patch behavior before a scope correction is proposed.

## Windows-derived regression candidate

The [v2 candidate](../../../patches/candidates/combase-stub-activation-context-v2/README.md)
adds the measured ordinary-ok regression to combase/tests/stub_actctx.c,
keeping the production implementation unchanged for comparison. It is
**blocked**, since the current policy is not a conforming fix for the observed
MTA cases. No expected values or todos conceal that conflict. test-body.h is
the exact added block; l1.c supplies the native assertion reporter.

```sh
bash scripts/build-l1-com-stub-actctx.sh /path/to/NEW-output
```

Run run-measure.bat in a fresh Windows VM directory and return
result-com-stub-actctx-l1-x86_64.txt,
result-com-stub-actctx-l1-i686.txt and exitcodes.txt. The completed native L1
assertion results are recorded below; the successful MEASURE alone was not
counted as L1. Production patches and series remain
unchanged. The remaining batch patches are separate work.


## Windows assertion L1 (2026-11-09)

Both architectures completed `SUMMARY checks=291 failures=0`, no FAIL lines,
and exit 0 in the maintainer's offline Windows 10 VM. The same test body is
in the candidate Wine test. This confirms the assertions against Windows,
not a conforming Wine implementation; the candidate remains BLOCKED while
paired runtime comparison checks the production policy. Raw logs are not
committed.


## Paired Wine run launched (2026-11-09)

[Actions run 42567082861](https://github.com/Harvi654/winum/actions/runs/42567082861)
uses source commit 0ce44f0 and workflow Test COM stub capture (paired Wine).
Both variants contain identical final assertions and all other production
patches; only this capture implementation is omitted in without. The recipe
checks passed locally, including a complete source comparison showing only
the intended three implementation files differ. Six result-gate tests passed.

The run was in progress when recorded; no runtime compatibility verdict is
claimed. Resume by checking both matrix jobs and the compare report artifact.
A red compare result may be the expected conformance finding; distinguish it
from a failed build or incomplete test coverage before drawing conclusions.
The candidate stays blocked pending actual paired results and review. No
additional Windows VM run is needed for this stage.
