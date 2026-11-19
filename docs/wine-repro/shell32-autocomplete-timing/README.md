# Shell autocomplete callback timing

AutoCAD 2027's Save As file-name edit can stop responding while the
application enumerates autocomplete suggestions. Wine 11.22's
`dlls/shell32/autocomplete.c` calls `IEnumString::Next` in the
edit window's message handler and gathers the full list. This
program observes the Windows implementation with its own edit
window, without launching AutoCAD or enumerating the filesystem.

The [patch page](../../patches/shell32-autocomplete-responsive.md) records
the approved v2 deviation and where to resume compatibility work.

Each autocomplete case creates a fresh edit and initializes `IAutoComplete2`
with autosuggest and autoappend. The program starts with one synthetic
character, pumps the UI, and summarizes callbacks. The source is
finite, reports the number of strings produced and returns
`S_FALSE` when it runs out.

- `empty`: empty control case.
- `fast-long`: 1,024 in-memory suggestions, no added delay.
- `slow-standard`: 128 suggestions, 12 ms added delay per returned
  item, an object created on the edit's STA without a custom marshaler.
- `slow-agile`: the same 128 suggestions and delay with a
  free-threaded marshaler, to distinguish callback threading from
  delivery through the standard STA proxy.

`input_dispatch_ms` is the time from sending `WM_CHAR` to its
return. The program then pumps messages for another three seconds.
`in_input` counts callbacks entered before the message returned.
`on_ui` and `off_ui` refer to the source object's callback thread.
They do not identify the thread that made a marshaled call.
`returned` counts suggestions produced (resets can start another
pass). `s_false` counts calls that reached the end before filling
the request. A second thread sends bounded probe messages to the
window and reports how many the UI handled during an active
callback. Timing samples are observations, not exact expectations
for a Wine regression test.

The executable watchdog bounds the whole measurement at 45 seconds.
A case that never invokes the source after the synthetic character
reports `no_next_after_input`. It is inconclusive for enumeration
timing. This fixture covers bounded valid enumerators. It does not
measure malformed enumeration results or infer the exact behavior
of AutoCAD's source from the number of calls in a trace.

## Offline Windows 10

Extract the bundle on Linux and verify it with
`sha256sum -c SHA256SUMS` from the bundle directory. Copy the
verified folder into a new directory in the offline VM and
double-click `run-measure.bat`. The measurement windows appear and
close on their own. Leave the keyboard and mouse idle until both
architectures finish. Return only these files:

- `result-shell32-autocomplete-timing-x86_64.txt`
- `result-shell32-autocomplete-timing-i686.txt`
- `exitcodes.txt`

The program uses its own sample suggestions. It prints no actual
filenames, homes, registry values or account information.

## Build

From this winum checkout with both MinGW compilers installed:

```bash
export AUTOCOMPLETE_MEASURE_OUT="$PWD/measure-shell32-autocomplete-timing"
bash scripts/build-measure-bundle.sh "$AUTOCOMPLETE_MEASURE_OUT" \
    docs/wine-repro/shell32-autocomplete-timing/measure.list
(cd "$AUTOCOMPLETE_MEASURE_OUT" && sha256sum -c SHA256SUMS)
```

## Windows 10 results

Measured on 2026-11-19 in the maintainer's offline VM using the
[MEASURE bundle](https://github.com/Harvi654/winum/actions/runs/44548426145)
from `d297048`. Both executables exited with code 0.
`WM_CHAR` returned within the tick counter's resolution (reported
0 ms), with no `Next` callbacks before input or inside the
input message. All the observed `Next` callbacks ran off the thread
that owned the edit window.

| Source | `Next` calls (x86_64 / i686) | End of source (x86_64 / i686) | UI replies during `Next` (x86_64 / i686) |
| --- | ---: | --- | ---: |
| Empty | 1 / 1 | `S_FALSE` / `S_FALSE` | 0 / 0 |
| Fast, 1,024 items | 1,001 / 1,001 | No `S_FALSE` observed in either run | 0 / 0 |
| Slow, plain STA object | 123 / 122 | Call still active at sampling in both runs | 60 / 62 |
| Slow, free-threaded marshaling | 123 / 123 | Call still active at sampling in both runs | 61 / 58 |

Both architectures requested one item per call, reset the source once
per case and did not call `Clone`. In the fast case both returned 1,001
items, with no call still active at sampling. This is an observation
for that source, prefix and option set. The measurement does not
establish a general enumeration limit. The slow cases establish that
the edit window could respond before the source finished.

Wine 11.22 calls `enumerate_strings()` inline from `autocomplete_text()`
on the edit thread when it needs to refresh its suggestion list. A
slow source can therefore hold up the input handler. This explains
how the dialog could stop responding during the repeated
`IsNetDrive` calls in the AutoCAD 2027 trace. Whether that source
would finish and whether moving or rescheduling callbacks fixes the
dialog still need separate confirmation in AutoCAD.

## Follow-up on COM apartments

The original `slow-standard` result shows where `Next` ran. It does
not establish that Windows autocomplete used a standard COM proxy.
The updated program first measures two controls without autocomplete.
It marshals the same kinds of provider objects from the edit's STA to
an MTA worker with `CoMarshalInterThreadInterfaceInStream`. The worker
unmarshals and calls `Next` once. The STA pumps messages. The two
controls show where the callee runs through an explicit COM transfer,
for a plain object and for an object that aggregates the free-threaded
marshaler.

The program then repeats the four autocomplete cases. It adds the
`CoGetApartmentType` HRESULT, type and qualifier from the first `Next`,
and the number of provider queries for `IMarshal` and `IAgileObject`.
An apartment lookup failure is itself a result. Interface query counts
and apartment type show what the provider can observe. They do not
identify the implementation of Windows autocomplete.

In a Linux x86_64 check with winum-11.22-1 and the v2 runtime candidate,
the explicit marshaling controls both returned `S_FALSE`. The plain
provider received `Next` on the original STA, and the free-threaded
provider received it on the MTA worker.

On 2026-11-19, both Windows 10 VM executables from
[`33ee610`](https://github.com/Harvi654/winum/actions/runs/44619793847)
exited with code 0. The explicit controls agreed on both architectures.
The plain provider received `Next` on the UI main STA (type 3).
The free-threaded provider received it on the MTA worker (type 1).
For autocomplete, all `Next` calls in the four cases ran off the UI.
The first callback in every case reported a separate STA (type 0,
qualifier 0). The providers observed zero `IMarshal` and `IAgileObject`
queries. A normal COM transfer therefore gave different callback
placement for the plain provider. The counters do not reveal how
autocomplete obtained that behavior.

### Separate provider STA and in-flight calls

`separate-sta` creates the plain provider on its own STA, marshals
`IEnumString` from there into the edit STA and passes the resulting
proxy to `IAutoComplete2::Init`. The owner pumps messages throughout
the case. `on_provider_owner` counts actual calls on the owner and
distinguishes it from both the edit thread and an autocomplete worker.

The three `*-during-next` cases hold the first input-triggered `Next`
until the edit action returns or the 1.5 second wait expires. They use
a local plain provider with alternating `a` and `b` suggestions.
The edit handles exactly one action: replace the selected input with
`b`, disable autocomplete, or destroy the edit window. The summary
records whether the held call was active when the action started and
returned, how many calls had started at those points, the hold's wait
result, and the final edit prefix. `hold_wait=0` is event signaled and
`hold_wait=258` is timeout. Timings locate a possible synchronous wait.
They are not exact expectations for a regression test.

On 2026-11-19, both Windows 10 VM executables from
[`93e5b98`](https://github.com/Harvi654/winum/actions/runs/44621177032)
reported the same outcomes:

| Case | Result on x86_64 and i686 |
| --- | --- |
| Separate STA proxy | COM unmarshal and autocomplete setup succeeded. The owner received no `Reset` or `Next` during the three-second observation. The probe returned exit code 1 for this case on both architectures. |
| Replace input during `Next` | The action returned while the first callback was held. The source was reset twice in total, and the edit showed a completion with the replacement `b` prefix. |
| Disable during `Next` | The action returned while the first callback was held. It was the only `Next` observed, and the edit kept the input `a`. |
| Destroy during `Next` | The action returned while the first callback was held. It was the only `Next` observed, and the edit was destroyed. |

The three held calls received the release event in all six trials. The
proxy case establishes that the provider saw no enumeration in this
window. It does not expose the HRESULT from a possible call on the
autocomplete worker or establish how Windows handles other proxy types.

For comparison, the x86_64 Linux replay with the v2 candidate called the
proxy provider on its owner STA. The three edit actions were delivered
after the held `Next` had timed out on the edit thread. This gives a
concrete responsiveness difference even when the UI can process messages
between calls.

## Wine regression test

`patches/tests-first/shell32-tests-autocomplete-thread.patch` adds the
`shell32:autocomplete_thread` test to Wine 11.22. The empty-source case
checks that autocomplete calls `Next` after input on a thread other than
the one owning the edit. The two lifecycle cases use an event to hold
the first callback and check that disabling autocomplete or destroying
the edit returns before the callback and abandons its results. They
check callback order and the resulting edit state. Time limits bound
the test run and the observation window.

To build the same Wine test for both architectures with MinGW:

```bash
export AUTOCOMPLETE_L1_OUT="$PWD/l1-shell32-autocomplete-thread"
bash scripts/build-l1-shell32-autocomplete-thread.sh "$AUTOCOMPLETE_L1_OUT"
(cd "$AUTOCOMPLETE_L1_OUT" && sha256sum -c SHA256SUMS)
```

Copy the verified folder to a new directory in the offline Windows 10
VM and double-click `run-tests.bat`. Leave the mouse and keyboard idle
while the test windows are open. Return `result-shell32-autocomplete-thread-x86_64.txt`,
`result-shell32-autocomplete-thread-i686.txt` and `exitcodes.txt`.

### L1 result for the original callback-thread test

Passed on 2026-11-19 in the maintainer's offline Windows 10 VM using the
[test bundle](https://github.com/Harvi654/winum/actions/runs/44554709236)
from `5d308e9`. Both x86_64 and i686 ran 10 checks with zero failures,
zero skips and process exit code 0. This covers the Wine test itself
as well as the earlier MEASURE result for the empty source.

### L1 result for the extended lifecycle test

The [extended test bundle](https://github.com/Harvi654/winum/actions/runs/44623001615)
from `d02aa0b` passed in the offline Windows 10 VM on 2026-11-19.
Both x86_64 and i686 ran 34 checks with zero failures, zero skips,
zero todos and zero flaky checks. Both executables exited with code 0.

In the x86_64 Linux replay on the v2 runtime, the extended test ran
34 checks with three expected todos, zero failures and zero skips.
Both edit actions ran after the held callback returned.

The lifecycle assertions are ready to check a new runtime. Standard COM
proxy delivery can route `Next` back to the edit STA. Calling an
arbitrary STA provider directly from a worker would instead bypass its
COM apartment boundary. The measured lifecycle behavior defines when
the edit must stop using a result. The callback transport for a
Windows-compatible implementation remains open.

### Wine baseline

On 2026-11-19, the x86_64 L1 executable reported the expected
`todo_wine` on the SHA-256-verified [winum-11.22-1 release](https://github.com/Harvi654/winum/releases/tag/winum-11.22-1)
(`ba856f8`, fresh prefix with Xvfb). Its only `Next` call ran on the
edit thread before `WM_CHAR` returned. The test recorded 10 checks,
one expected todo, zero failures and zero skips.

In the x86_64 MEASURE replay, winum read all 1,024 suggestions from
the fast source and made a final call that returned `S_FALSE`.
Both slow sources made 129 calls on the edit thread before
`WM_CHAR` returned, including the call that reached the end.
Input dispatch took about 1.57 seconds. The window answered none
of the probe messages while `Next` was active. The same two sources
on Windows 10 left the edit responsive during enumeration.

The local winum baseline is x86_64 only. The i686 test and MEASURE
executables failed to start in that Linux environment and yielded no
assertion results. The built-in 32-bit `cmd.exe` also failed to start
in a fresh prefix there. Both i686 runs on Windows 10 completed.

The [runtime candidate](../../../patches/candidates/shell32-autocomplete-responsive/README.md)
records the paired comparison and the AutoCAD test procedure.

## AutoCAD 2027 follow-up

In the paired Save As dialog test with FILEDIA set to 1, the
baseline froze during filename editing and required termination.
With the autocomplete v2 candidate, Cancel returned to the drawing,
editing continued and AutoCAD exited normally. The
[paired shell32 test run](https://github.com/Harvi654/winum/actions/runs/44592013573)
uses the same runtime and test executables for both variants.

On 2026-11-19 the maintainer reported that editing the filename worked
with the v2 candidate. AutoCAD still crashed on saving and when opening
an existing DWG.

All four observed runs used FILEDIA set to 0 and the same MSXML3 build
(`aeb7a59`). Each exited with code 9 and raised unhandled exception
0xc0000409 with flags 0x1.

| Variant and operation | Fault location | Output |
| --- | --- | --- |
| Autocomplete v2, Save As | `coreclr.dll+0x14922d`, recovered from the original trace's load address | DWG exists, 30,927 bytes. Reopening is unverified |
| Baseline, Save As | `coreclr.dll+0x14922d`, resolved from the loaded image range | Unchecked |
| Autocomplete v2, Open | `coreclr.dll+0x14922d`, resolved from the loaded image range | Existing DWG failed to open |
| Baseline, Open | `coreclr.dll+0x14922d`, resolved from the loaded image range | Same failure. No Event Log entries were captured |

The baseline reproduces both crashes without the autocomplete
candidate. All four failures reached the same CoreCLR address relative
to the image base. The triggering condition is still unknown.

A second baseline Open run with the debug-event observer reached the
same CoreCLR offset. The fail-fast reason was 2
(`FAST_FAIL_STACK_COOKIE_CHECK_FAILURE`). The preceding process-wide
events included CLR and native C++ exception codes. The first observer
did not record their thread IDs. No Event Log entries were captured.

The next baseline Save As run also reported reason 2 at
`coreclr.dll+0x14922d`. The observer captured one first-chance CLR
exception (0xe0434352) on the failing thread. The event contained no
parameters, and the run produced no Event Log entries.

Keep the candidate and baseline Winum runtimes on the same MSXML3
build and reuse the two prepared test prefixes. Run one operation per
launch with FILEDIA set to 0. Save a scratch drawing under a new name
and test Open separately with a known-good DWG. If an operation only
crashes with the dialog enabled, run it again with FILEDIA set to 1.

From the winum checkout, use the shared launcher with an existing
`ACAD_AUTOCOMPLETE_WORK`:

~~~bash
bash scripts/trace-autocad-dwg.sh candidate save-0
~~~

Use `baseline` for the matching control and `open-0`, `save-1` or
`open-1` for those operations. Set FILEDIA in AutoCAD as printed at
startup. After each launch return the script's summary and, for Save
As, whether the new file appeared before the crash.

The trace uses Wine SEH, DLL load, module mapping and event log
channels in acad.exe. Raw logs stay in the private run directory.
The summary includes managed exception types if the runtime reports
them through the event log or debug output. For an
unhandled exception, the summary reports the image and offset
when the fault address falls within a recorded loaded image range.
The original report used a D3DCompiler-relative offset. The newer
image-range traces place this failure inside CoreCLR.

### Compare startup DWG loading with the OPEN command

[Inventor PR #28](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/pull/28)
handles first startup of `dwgviewr.exe`, the TrueView OLE server used by
Inventor's sketch command Insert AutoCAD file. It seeds the viewer's
per-user registry and supplies the native `d3dcompiler_47.dll` required by
the prefix override. The PR also uses the winum PR #4 MSXML3 fix for
the viewer's profile loop. Inventor's File > Open uses its own DWG/DXF
wizard. The AutoCAD failure above occurred in `acad.exe`, with a mapped
CoreCLR fail-fast address and the MSXML3 fix already present. The
comparison counts TrueView loads and `RegLoadAppKey` stub calls to check
the possible startup overlap. It prints only process counts and public
module names. The two shader compiler filenames are distinct DLLs.

Use the existing baseline prefix copy and the same test runtime for
both runs. Choose a small DWG that opens normally in Windows. The helper
stages a new scratch copy in the test prefix's C: drive for each run.
Disable the earlier optional collectors for this comparison.

~~~bash
export ACAD_DWG_FIXTURE="$HOME/path/to/known-good.dwg"
unset ACAD_EVENTPIPE ACAD_FAILFAST_OBSERVER
bash scripts/trace-autocad-dwg.sh baseline startup-open "$ACAD_DWG_FIXTURE"
~~~

The launcher passes the scratch DWG as a startup argument. Record whether
the drawing itself opens. If it opens, make a small edit and run `._QSAVE`
once. Record whether AutoCAD survives that operation, then close it.
The summary says whether the scratch DWG changed. A changed file alone
does not establish a successful save or reopen.

Then start a fresh AutoCAD process on the same baseline with:

~~~bash
bash scripts/trace-autocad-dwg.sh baseline open-0 "$ACAD_DWG_FIXTURE"
~~~

Set `FILEDIA` to 0, run `._OPEN` with the Windows path printed by the
helper and record whether it opens. Return only the two summaries and
the visible result of each action. If startup opens the drawing but
the command fails, compare command dispatch and document activation.
If both opening paths fail with the same signature, investigate the
shared DWG loading path. If startup opens the drawing and QSAVE succeeds,
compare that write path with Save As to isolate the operation that fails.
If AutoCAD stays on its start screen and never opens the startup DWG,
the startup argument trial is inconclusive.

In the first startup trial, the maintainer used a DWG already known to
open on Windows. AutoCAD exited during loading with `0xc0000409` at
`coreclr.dll+0x14922d`, the same address as the earlier command trials.
The summary counted zero TrueView loads and zero `RegLoadAppKey` stub
calls. The paired command trial with this exact DWG is still open.

### Measure reads of the startup DWG

Trace the same startup trial on the same baseline prefix with `strace`
installed. The helper prints only the file's six-byte DWG format code
when it matches `AC` followed by four digits, the file size and counts
of file operations. `AC1032` is the AutoCAD 2018 DWG format, and
`AC1027` is the 2013 format. The header identifies the format generation.
It gives no indication of damage, drawing classes or external references.
[Autodesk format codes](https://www.autodesk.com/support/technical/article/caas/sfdcarticles/sfdcarticles/drawing-version-codes-for-autocad.html)

~~~bash
unset ACAD_EVENTPIPE ACAD_FAILFAST_OBSERVER
export ACAD_DWG_IO=1
bash scripts/trace-autocad-dwg.sh baseline startup-open "$ACAD_DWG_FIXTURE"
~~~

The system-call filter follows the launcher and its children on Linux
and selects the scratch DWG by its exact path. A positive read count
establishes that bytes were read. A successful mapping establishes
that the file was mapped. Neither result establishes that the drawing
parsed or opened. If opening fails before bytes are read or a mapping
is recorded, check path handling and the startup sequence first. Compare
the fail-fast signature with the untraced trial. Raw traces are private.

### Capture the fail-fast reason

Wine's x86_64 fast-fail dispatch preserves the numeric reason in the
first exception parameter. The terminal SEH message omits it. The
observer reads Win32 debug events to recover that parameter and the
preceding exception codes on the failing thread. For CLR exception
events it also reports the first numeric argument. The observer does
not read process memory or use vendor symbols. Attaching sets the
process's debugger flag, so compare the resulting fault module and
offset with the previous run.

Build with `scripts/build-measure-bundle.sh` and the
`docs/wine-repro/shell32-autocomplete-timing/failfast.list` manifest.
With no arguments, each executable runs a self-test that observes a
numeric fail-fast reason from its own child process. On winum-11.22-1,
the x86_64 self-test and a separate attach to a synthetic process both
reported reason 7 and the expected exit status. A two-thread synthetic
check of the updated observer reported only the argument from the
thread that issued fail-fast. In the offline
Windows VM, run the two self-tests with `run-measure.bat`. For AutoCAD,
use the x86_64 executable with the existing trace script:

~~~bash
export ACAD_FAILFAST_OBSERVER="$ACAD_OBSERVER_BUNDLE/autocad-failfast-observer-x86_64.exe"
bash scripts/trace-autocad-dwg.sh baseline open-0
~~~

Wait for AutoCAD to reach the start screen before pressing Enter in
the terminal. After `observer_ready=1`, set FILEDIA to 0 and open the
same test DWG. The observer detaches after two minutes if the process
is still running.

### Collect runtime exception events

The startup EventPipe session can stream the CLR exception provider
to a private file when enabled with the documented
[EventPipe environment settings](https://learn.microsoft.com/en-us/dotnet/core/diagnostics/eventpipe).
Run the baseline Open trial with the existing launcher and without
the optional debugger observer:

~~~bash
unset ACAD_FAILFAST_OBSERVER
export ACAD_EVENTPIPE=1
bash scripts/trace-autocad-dwg.sh baseline open-0
~~~

Use FILEDIA 0 and the same test DWG. Files are written below a new
private `winum-eventpipe.*` directory inside the test prefix's C:
drive. The summary selects the file matching the crash process ID.
It reports possible exception type names from the trace's string
payloads. These hints do not correlate a type to a specific event
or thread. Return only the summary. The trace can contain exception
messages and paths and stays local.

On 2026-11-19, a synthetic program using Windows .NET 10.0.0
under winum-11.22-1 threw two caught exceptions and then called
FailFast. Its streamed file retained both expected type names.

The AutoCAD baseline Open trial reached the same CoreCLR offset and
produced a trace for the crash process. The string scan found
`System.IO.FileNotFoundException`,
`System.ServiceModel.EndpointNotFoundException` and
`Autodesk.DesktopConnector.API.DesktopConnectorNotInstalledException`.
Their occurrence on the failing thread was not established by the scan.

The structured decoder recovered 663 exception events before the
incomplete end of the trace. It found zero events on the failing
thread (0x0ae8). The last decoded EndpointNotFound and Desktop
Connector exceptions occurred on thread 0x0b3c at about 38.4 seconds.
The last decoded exception was FileNotFound on that same thread at
about 41.4 seconds. The previous Win32 observer still recorded a CLR
exception on the failing thread during Save As. The decoded Open trace
does not account for that last phase of the crash.

To check for the same exception names during startup, run an idle
control with EventPipe in the same baseline prefix:

~~~bash
unset ACAD_FAILFAST_OBSERVER
export ACAD_EVENTPIPE=1
bash scripts/trace-autocad-dwg.sh baseline idle
~~~

Stay on the AutoCAD start screen for 60 seconds and close the program
normally. The script selects the AutoCAD process trace even on normal
exit and reports whether each of the three string hints above appears.

The control run on 2026-11-19 showed all three names in the selected
AutoCAD trace without an Open or Save operation. The launcher returned
96, and the run recorded no unhandled exception or stack-dispatch
failure. Those names also appear during startup. The result weakens
the Desktop Connector hypothesis for the shared CoreCLR failure.
The triggering condition at the CoreCLR fail-fast site is still unknown.

### Decode an existing EventPipe trace

The EventPipe summary utility uses TraceEvent to read CLR exception
events and report elapsed time, the native thread ID and the type.
When the stream is incomplete after a crash, the utility keeps the
events parsed before the format error. This was tested with a streamed
Windows .NET 10 trace from winum whose process terminated by fail-fast.

Download and verify the `autocad-eventpipe-summary` artifact. With
`ACAD_AUTOCOMPLETE_WORK` set, run its shell script with the hexadecimal
process ID and thread ID from the existing baseline crash summary.
For the first baseline EventPipe run:

~~~bash
bash summarize-autocad-eventpipe.sh 0400 0ae8
~~~

The script locates the existing trace and uses a verified portable
.NET runtime and TraceEvent package in a temporary directory. The
trace stays in the prefix. Return only the summary lines with counts,
times, thread IDs and types.
