# COM stub activation context: Windows MEASURE

Original, AI-assisted (Codex), LGPL-2.1-or-later. Native context-selection
values are unknown until the maintainer returns Windows output. This measures
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
Native assertion L1, formal cross review, full-series build and paired Wine
runtime/consumer validation are not claimed by this MEASURE.


A second-model read-only advisory checked the design and full program against
open Wine's class-factory proxy/stub, marshaling and activation-context tests.
It identified a message-pump timeout gap, addressed with the independent
watchdog, and recommended the positive/exclusive marker controls now included.
Its follow-up found no blocking COM/handle ownership issue. Watchdog stdio
was then removed to avoid waiting on a stuck thread's output lock. No
conspicuous copied block was identified; this is not proof of originality,
formal cross review or native execution.
