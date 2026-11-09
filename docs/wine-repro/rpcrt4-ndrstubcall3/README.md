# NdrStubCall3: NDR32 Windows MEASURE

Original fixture, AI-assisted (Codex), LGPL-2.1-or-later. Initial native results are
recorded below; the control faults, so no functional oracle is established.
This prepares a functional oracle for rpcrt4-ndrstubcall3.patch;
it is not an export-only conformance test or an L1 pass. Production patches,
their markers and patches/series are unchanged.

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
