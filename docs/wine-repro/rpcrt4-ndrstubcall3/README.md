# NdrStubCall3: NDR32 Windows MEASURE

Original fixture, AI-assisted (Codex), LGPL-2.1-or-later. All three native
fixture iterations are recorded below. Real COM transport now gives working
NdrStubCall2 controls, but the x64 NdrStubCall3 probe does not reach the server.
No functional native NdrStubCall3 oracle or L1 is established. **Stopped after
the third iteration; do not rerun the historical bundles unchanged.**
Production patches, their markers and patches/series are unchanged.

## Own SDK fixture

probe.idl declares IWinumNdrProbe.Calculate(LONG, LONG*). Wine 11.20 WIDL
(commit 7298a818b79446bae153b8d5aa27235ab3703bf9) generated the checked-in
x86_64/i686 proxy, header and IID metadata from that original IDL. No Microsoft
MIDL compiler, vendor stub, native binary internals or private symbols were
used. The generated NDR format bytes describe our own method, not an observed
vendor interface. Root LICENSE applies to this fixture and generated output.

WIDL's method-offset table normally uses a pointer biased by -3. These own
outputs instead have three leading IUnknown slots and an unbiased pointer,
preserving procedure 3 while avoiding an out-of-bounds pointer under -Werror.
regenerate.py reproduces the metadata and that documented transformation:

```sh
python3 docs/wine-repro/rpcrt4-ndrstubcall3/regenerate.py /path/to/widl-11.20 /path/to/wine-11.20/include
```

The normal bundle build uses the checked-in outputs and needs only MinGW;
there is no WIDL or Windows SDK dependency at bundle-build time and no download.

NdrDllGetClassObject creates a real IPSFactoryBuffer. Its CreateStub and
CreateProxy build standard SDK objects for our IID. A local IRpcChannelBuffer
forwards the generated proxy's request to IRpcStubBuffer_Invoke, exercising
actual parameter marshaling and server dispatch rather than a direct call
to our Calculate implementation. Our server increments its call counter,
computes input+7 and returns S_OK. No class or proxy is registered globally,
no extra DLL is installed, and there is no network transport or CAD dependency.

The control dispatches through dynamically loaded NdrStubCall2 with WIDL's
ordinary server metadata. The probe dispatches through NdrStubCall3 with a
writable copy of that metadata: nCount=1 and one genuine NDR32 syntax entry
(UUID 8a885d04-1ceb-11c9-9fe8-08002b104860, version 2.0) containing the generated
procedure/type strings. The top-level procedure pointers are NULL, requiring
the syntax-entry path. The generated client proxy and request remain the
same. There is no invented NDR64 entry or opaque vendor data.

This single NDR32 entry is a candidate native fixture, not proof of acceptance
by Windows. If Windows rejects it, validate the fixture before turning that
rejection into an expected Windows contract. It does not cover real NDR64
negotiation, NDR64 wire data or the patch's legacy top-level fallback branch.

## Architectures, ownership and limits

Both control/probe compile for x86_64 and i686 with -Wall -Wextra -Werror.
The current Wine patch exports NdrStubCall3 only on win64: an i686 availability
observation cannot validate that implementation. Native export availability
is printed independently on both architectures, without assuming a Windows
result. Missing export exits 0 as an observation; it is not functional coverage.

Each mode runs in its own process so a fault in the new interpreter does not
lose the ordinary control. An independent 30-second watchdog terminates only
the measurement process if necessary. Proxy/stub, server-info and syntax
storage outlive their calls; object/channel references and request/reply
buffers are balanced and reported. Unknown native HRESULT/value/phase results
are printed without conformance assertions. Fixture setup or control failures
produce a nonzero exit. No addresses, private strings or paths are printed.

## Build and offline Windows run

```sh
export NDR_MEASURE="$HOME/.cache/winum-ndr-measure-$(date +%Y%m%d-%H%M%S)"
bash scripts/build-measure-bundle.sh "$NDR_MEASURE" docs/wine-repro/rpcrt4-ndrstubcall3/measure.list
```

Copy the directory to a fresh directory in offline Windows 10 and run
run-measure.bat. Return both architectures' result-ndrstub-control-*.txt and
result-ndrstub-call3-*.txt, plus exitcodes.txt, even after a nonzero exit.
No Inventor or interactive window is needed.

A usable control must report available=1, one DISPATCH/server callback and
our server's value 42 from input 35, with complete SUMMARY and api_errors=0.
The probe must actually reach DISPATCH_DONE and SERVER to establish its NDR32
path; missing export, rejected metadata, missing SUMMARY or a timeout cannot
count as functional acceptance or native L1. Compare both replies and cleanup
before deriving ok()/todo_wine expectations. Raw logs will not be committed.

The main measure-bundle.list contains both entries. The same fixture can also
serve as a bounded L3 NDR32 reproducer on Wine, but no Windows conformance
marker or claim of NDR64 support follows from it. A real native subset test
is feasible; it is not classified L3-only merely because results are pending.

## Preparation review

A second-model read-only advisory read the original IDL, SDK metadata path,
full harness and relevant open Wine cstub/interpreter code. It found no
blocking lifetime/buffer issue and confirmed that the unbiased table retains
method-3 indexing. Reference/buffer cleanup is checked and the independent
watchdog was added. This is an advisory and limited resemblance check, not
proof of originality, formal cross review or native execution. All four
EXEs compile strictly and bundle hashes verify. Expected native values and
any subsequent Wine conformance test remain pending Windows output.


## Initial Windows results (2026-11-09): fixture blocked

The maintainer ran the runtime bundle in offline Windows 10 and returned
all four text outputs and exit codes. No vendor files or native internals
were inspected. The observations are:

| Mode | Architecture | Export | Last completed observation | Exit |
| --- | --- | --- | --- | --- |
| NdrStubCall2 control | x86_64 | present | DISPATCH, before_phase=0 | -1073741819 (0xc0000005) |
| NdrStubCall2 control | i686 | present | DISPATCH, before_phase=0 | -1073741819 (0xc0000005) |
| NdrStubCall3 probe | x86_64 | present | DISPATCH, before_phase=0 | -1073741819 (0xc0000005) |
| NdrStubCall3 probe | i686 | absent | Complete availability-only SUMMARY | 0 |

All three dispatching processes first reported S_OK for CoInitializeEx,
NdrDllGetClassObject, CreateStub, CreateProxy and Connect. Their own channel
reported method=3, bytes=4, representation=00000010. None reached SERVER,
DISPATCH_DONE, CALL, CLEANUP or SUMMARY. The access violation is therefore
inside the invocation interval, but its precise source is not established
by this output. It is not evidence that a valid native NdrStubCall3 call
should fault: the ordinary control fails too.

The i686 absence is an availability observation only, consistent with this
patch's win64 export scope; it is not functional coverage or an L1 pass.
The control must be corrected and remeasured before writing conformance
assertions from this fixture. No arbitrary exception value is turned into
an expected result, no export-only test substitutes for dispatch, and no
L3-only classification is claimed. The production patch, page marker and
series remain unchanged. Raw outputs are summarized rather than committed.


## Schema-2 diagnostic: unchanged generated stub

A read-only second-model advisory rechecked our own metadata and public
Wine/MinGW declarations after the native faults. It found no justified
offset-table or calling-convention correction. Reaching DISPATCH confirms
slot 3 dispatch, not acceptance by the native interpreter. The minimal fake
channel may lack context normally supplied by COM, but that is a hypothesis;
no reserved RPCOLEMESSAGE fields are populated by guess.

`generated-control.c` (mode 0) now keeps WIDL's original interpreted stub,
server metadata and NULL dispatch table. The same factory/proxy/channel
round trip runs without our dispatch wrapper or copied stub-table entries.
Its successful control would report input=35, value=42, server_calls=1 and
stub_calls=0, with balanced cleanup and complete SUMMARY. Mode 2 still
expects stub_calls=1. Schema 2 adds CALL_BEGIN to locate a fault even when
the generated control has no wrapper log. The functionally unknown mode-3
result remains an observation. All six EXEs build with -Wextra -Werror.

```sh
export NDR_DIAGNOSTIC="$HOME/.cache/winum-ndr-diagnostic-$(date +%Y%m%d-%H%M%S)"
bash scripts/build-measure-bundle.sh "$NDR_DIAGNOSTIC" docs/wine-repro/rpcrt4-ndrstubcall3/diagnostic.list
```

Run run-measure.bat in a fresh offline Windows directory and return all
result-*.txt and exitcodes.txt. If the unchanged generated control faults
too, the fake-channel path is not an accepted native control: move to a
documented real COM transport before interpreting NdrStubCall3. If it works,
investigate the wrapper/copied metadata difference first. This is the
second fixture iteration, not a Wine fix or native L1; do not keep guessing
metadata across repeated unsuccessful iterations.


## Schema-2 Windows results (2026-11-09): generated control also faults

Both unchanged generated-stub controls reached CALL_BEGIN and WIRE
(method=3, bytes=4, representation=00000010), then exited -1073741819
(0xc0000005) before SERVER, CALL, CLEANUP or SUMMARY. The mode-2 controls
on both architectures and the x64 mode-3 probe again stopped after DISPATCH
with the same exit. The i686 mode-3 export was absent and its availability
SUMMARY completed with exit 0. Every preceding factory/proxy/stub/connection
API reported S_OK.

Removing the wrapper and copied server metadata did not restore a usable
control. This rules out that wrapper being necessary for the fault; it
does not identify the precise failing instruction or prove that generated
format strings are accepted. The minimal channel remains a leading harness
hypothesis, not an established native RPC defect. Do not repeat these
minimal-channel programs unchanged or turn the access violation into an
expected conformance result. The next fixture uses public COM marshaling
to provide a real channel, without filling reserved message fields. This
is the second unsuccessful fixture iteration; one further deliberate
transport change is permitted before applying AGENTS.md's three-round stop.
No production patch, marker, series entry or L1 claim changes here.


## Schema-3 MEASURE: standard COM transport (third fixture iteration)

`com-measure.c` replaces the fake IRpcChannelBuffer with public COM
marshaling. The main STA owns the non-agile object. CoMarshalInterThreadInterfaceInStream
and CoGetInterfaceAndReleaseStream pass its custom interface to an MTA
thread; the STA pumps messages until the client has released its proxy and
uninitialized COM. NdrDllGetClassObject supplies the SDK proxy factory.
CoRegisterClassObject (CLSCTX_INPROC_SERVER) and CoRegisterPSClsid register
only our own factory/IID inside the process in each apartment. These are
public registration APIs, also exercised by open Wine's own tests; their
success in this new fixture still needs native measurement. No registry
keys, native DLLs, proxy DLL installation or reserved message fields are
written. COM supplies the channel and message state.

Three independent programs keep the control/probe distinction:

| Program | Stub path | What must complete before interpretation |
| --- | --- | --- |
| ndrstub-com-generated | Original WIDL interpreted stub, NULL dispatch table | Real COM call reaches our server once, value=42, no explicit wrapper calls |
| ndrstub-com-control | Ordinary metadata, explicit NdrStubCall2 wrapper | Same successful call plus one DISPATCH/DISPATCH_DONE |
| ndrstub-com-call3 | One NDR32 syntax entry, explicit NdrStubCall3 wrapper | Observe native export, dispatch, server and reply before choosing expectations |

All programs use the same original IDL and generated formats. Modes 0/2
check the response chosen by our own method (input=35 -> output=42, S_OK),
with one callback on its STA creator rather than the client thread. Mode 3
prints unknown native HRESULT/phase/value observations; it is not an
assertion test. `functional=1` requires completed S_OK/value=42 dispatch,
one server callback on the creator, and the appropriate wrapper count.
Missing export exits 0 with functional=0; that is not conformance coverage.
Successful setup alone does not establish interpreter acceptance.

The proxy/stub metadata remains alive until both apartments uninitialize.
The client consumes its marshaled stream on unmarshal, including failed
unmarshal. A stream not handed to that call is rewound and its marshal data
released in the owning STA. Client proxies are released before revoking its
factory; the server disconnects its own object, revokes its factory and
uninitializes COM before reporting the remaining reference observation.
No native reference count or stub phase is guessed as an expected value.
A separate 30-second watchdog covers setup, pumping, calls and cleanup and
terminates only this dedicated process on timeout. No live worker state is
freed on a timeout. Raw paths, addresses and native binary internals are
not inspected or printed.

```sh
export NDR_COM_MEASURE="$HOME/.cache/winum-ndr-com-$(date +%Y%m%d-%H%M%S)"
bash scripts/build-measure-bundle.sh "$NDR_COM_MEASURE" docs/wine-repro/rpcrt4-ndrstubcall3/transport.list
```

Copy the full folder into a fresh directory in offline Windows 10, run
run-measure.bat and return all six result-*.txt plus exitcodes.txt, including
failures. Only this list is needed; mshtml and actctx native L1 are already
complete. Do not repeat the historical minimal-channel lists. All six
transport EXEs compile with -Wall -Wextra -Werror and hashes verify.
Compilation does not establish native runtime success.

This is the third deliberate fixture iteration. If its native controls fail
to establish a working dispatch, stop under AGENTS.md's three-round rule,
record the exact last phase and best hypothesis, and leave this patch's
conformance gate blocked. Do not issue another guessed-metadata VM bundle
or call an export-only probe L1. No production implementation or series
change and no native L1 claim are made by preparing this measurement.


A second-model read-only advisory read the complete schema-3 source,
wrappers, original IDL and public Wine/MinGW marshaling and registration
paths. It found no concrete ABI, metadata-lifetime or synchronization defect
in the new transport. The public registration pattern also appears in Wine
tests; the original method, control variants and STA/MTA measurement are
our own fixture. No conspicuous nontrivial copied block was identified.
This limited advisory is not formal cross review or proof of originality.

One failure-path limit remains: if unmarshaling consumes the stream and
then fails, that stream is no longer available for explicit marshal-data
release. Server disconnection and apartment shutdown still run, and the
remaining object reference observation is printed; an elevated count on
that failed path is not a successful cleanup/conformance claim. Native
acceptance and all resulting expected values remain unknown. Even if the
controls work, a rejected mode-3 metadata shape alone must not be called a
functional native NdrStubCall3 oracle. Do not keep modifying it beyond this
third round without stopping and recording the evidence limit.


## Schema-3 Windows results (2026-11-09): controls work, probe blocked

The maintainer returned all six outputs and exitcodes.txt from the real COM
transport bundle. Every process exited 0. That means completed measurement
execution, not six successful functional tests. All registration, marshaling,
unmarshaling, revocation and disconnection APIs printed S_OK where invoked.

| Program | Architecture | Client result | Server / explicit wrapper calls | functional | Cleanup |
| --- | --- | --- | --- | --- | --- |
| Generated control | x86_64, i686 | S_OK, value=42 | 1 / 0 | 1 | object_refs=1 |
| NdrStubCall2 control | x86_64, i686 | S_OK, value=42 | 1 / 1 | 1 | object_refs=1 |
| NdrStubCall3 probe | x86_64 | RPC_E_SERVERFAULT (0x80010105), value=0 | 0 / 1 | 0 | object_refs=1 |
| NdrStubCall3 probe | i686 | Export absent; no call | 0 / 0 | 0 | Not initialized |

The controls' callbacks reported input=35, output_present=1, on_creator=1
and on_client=0. Both explicit NdrStubCall2 wrappers received method=3,
bytes=8, before_phase=0 and returned normally with result=0, after_phase=2.
Thus the real COM path supplies a working NDR32 control on both architectures.
Its request size differs from the minimal channel's four bytes; this does
not isolate which transport detail caused the earlier faults.

The x64 NdrStubCall3 probe reached DISPATCH with method=3, bytes=8 and
before_phase=0, but produced neither DISPATCH_DONE nor SERVER. COM returned
RPC_E_SERVERFAULT to the client and the process completed normal cleanup
with api_errors=0 and functional=0. This is the HRESULT observed at the COM
boundary, not a normal return from NdrStubCall3 and not the underlying
exception code. It does not establish a valid native NdrStubCall3 contract.
The i686 process reported available=0, completed=0 and functional=0; its
zero exit is only an availability observation. No raw logs are committed.

### Conclusion and stop point

Standard COM transport resolves the control failure, so these results are
closer than the first two iterations. However, the one-NDR32-entry metadata
shape is still not demonstrated as accepted by native NdrStubCall3. The
best remaining hypothesis is a mismatch between this synthetic server
metadata/syntax-selection setup and what that native entry point requires;
the underlying exception and precise requirement are unknown. Acceptance by
NdrStubCall2 does not prove acceptance by NdrStubCall3. The measurements do
not show a Windows defect or justify treating RPC_E_SERVERFAULT as the
expected result of a valid call.

Stop this fixture direction after its third iteration, as planned under
AGENTS.md's three-round rule. No fourth guessed metadata variant or VM run
is requested. Before reopening it, an independently justified public-API
metadata design with a successful native NdrStubCall3 control is needed.
Only open-source generators/declarations and our original IDL may supply
that design; native binary internals remain outside scope.

An honest successful-dispatch conformance test for this patch cannot yet be
written from these outputs. An export-only test would miss the implemented
interpreter, and encoding the observed server fault would validate this
unaccepted fixture rather than the intended functionality. No candidate
patch is invented and no production test marker or series entry changes.
This is a blocked native gate, not proof that Windows testing is impossible.

### Proposed L3 while the native gate is blocked

The existing real-COM programs can serve as a Wine-only synthetic subset
reproducer: run identical binaries in isolated prefixes against paired Wine
11.20 builds without and with rpcrt4-ndrstubcall3.patch, keeping other inputs
identical. First require successful generated/NdrStubCall2 controls on both
architectures. Then record whether the patched win64 probe reaches our
method once, returns the chosen value 42, completes DISPATCH_DONE and normal
cleanup; record baseline export availability separately. A skipped/missing
export, incomplete process or failed control is not a passing L3 run.

This would exercise the patch's claimed NDR32 syntax-entry subset and its
transport, not establish native compatibility, NDR64 support, alternate
syntax selection, fallback behavior or general safety of generated stubs.
It has not been run as L3 here. It cannot replace missing L1 or justify
`winum-test: l3-only`, because the native API exists and its valid-call
contract remains unresolved. Review the supported scope before deciding
how to handle that release gate. Existing CAD workaround behavior is
retained; no implementation removal or scope change is made by this record.

## L3 reproducer (2026-11-10)

See [L3.md](L3.md): the maintainer's L3-only decision for this patch and
three Wine-only scenarios built from `com-measure.c` (which gained a hook,
`CONFIGURE_SERVER_INFO`, unused by the MEASURE itself).
