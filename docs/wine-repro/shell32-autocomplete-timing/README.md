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

The maintainer's traced startup trial on 2026-11-19 used an `AC1018`
DWG of 4,145,388 bytes. The traced processes completed seven reads of
the exact scratch file, totaling 4,853 bytes. There were no matching
open or mapping calls in the filtered trace. The reads establish that
the startup reached the drawing data. Their total can include repeated
reads of the same offsets. The Wine log recorded `0xc0000409` at
`coreclr.dll+0x14922d` before the trace collector was stopped manually.
The launcher exit code 137 describes the collector stop. The scratch
DWG hash remained unchanged.

### Compare small DWGs in two format generations

Use LibreDWG's public `Line.dwg` fixtures in the 2004 (`AC1018`) and
2018 (`AC1032`) formats. The accompanying `Line.txt` records show the
same line endpoints. This controls for the sample's visible geometry
while comparing two format generations and removes the original
4 MB drawing's complexity from the test. Their full drawing databases
can differ. The original drawing is the sample confirmed to open in
the maintainer's Windows installation.

~~~bash
export ACAD_DWG_CONTROLS="${ACAD_AUTOCOMPLETE_WORK:?Set the test work directory}/dwg-controls"
export ACAD_DWG_CONTROLS_REF=47b67c0c5e0b4b33a65c6acea8f26b3e21ac5386
unset ACAD_DWG_IO ACAD_EVENTPIPE ACAD_FAILFAST_OBSERVER

(
    set -euo pipefail
    mkdir -p -- "$ACAD_DWG_CONTROLS"
    cd "$ACAD_DWG_CONTROLS"
    for version in 2004 2018; do
        curl -fsSL "https://raw.githubusercontent.com/LibreDWG/libredwg/$ACAD_DWG_CONTROLS_REF/test/test-data/$version/Line.dwg" \
            -o "line-$version.dwg"
    done
    sha256sum -c <<'SUMS'
60c86e1d4f038a3f1755d15bd22710c34608900957af61282432e4719b730c24  line-2004.dwg
1f4edd4c11aaf28b3295fd69fa59376f9cb911c637aa47a6073408ecac4797a3  line-2018.dwg
SUMS
)
~~~

Run `startup-open` once for each control with the same baseline prefix.
Check whether the line drawing appears, then close AutoCAD. Compare
the header and fault site for each run. If the small 2004 drawing opens,
the legacy generation can load in this configuration. If both controls
fail at the same site, prioritize their common loading or document
activation path. An outcome that splits by version suggests comparing
the legacy and modern paths with additional matched fixtures.

Both line drawings failed in the maintainer's startup trials on
2026-11-19. The 62,118-byte `AC1018` and 27,982-byte `AC1032` files
each exited with code 9 and raised `0xc0000409` at
`coreclr.dll+0x14922d`. Both staged files were unchanged. The fault
therefore also occurs with small drawings and in both tested format
generations.

### Compare desktop and Core Console drawing loading

Run the 2018 line fixture with the installation's `accoreconsole.exe`
on the same baseline runtime and prefix. The helper stages a new copy
of the DWG and a short AutoLISP script. It makes a private copy of the
AutoCAD launcher and substitutes the final executable while preserving
the ordinary prefix setup and service startup. Core Console opens the
scratch DWG and the script checks the active drawing name and counts
LINE entities before quitting. The desktop launcher remains available
for a matching control. [Autodesk Core Console usage](https://help.autodesk.com/cloudhelp/2026/ENU/AutoCAD-Core-Console/files/AUTOCAD_CORE_CONSOLE.html)

~~~bash
unset ACAD_DWG_IO ACAD_EVENTPIPE ACAD_FAILFAST_OBSERVER
bash scripts/trace-autocad-dwg.sh baseline core-open "$ACAD_DWG_CONTROLS/line-2018.dwg"
~~~

Return the summary lines `variant=`, `core_console_dwg=` and
`fault_module=` if present. `core_console_dwg=1` with a positive line
count demonstrates usable drawing data on the console path. A matching
fail-fast in the console would extend the failure beyond the desktop
path. If the executable is missing, it fails its own startup, or the
script marker is absent, treat the draw-open result as unconfirmed.

On 2026-11-20, the maintainer's Core Console run opened the staged
27,982-byte `AC1032` file and found one LINE. It exited with code 0 and
left the DWG unchanged. The filtered module trace recorded builtin
`msxml3.dll` and `shell32.dll`. It recorded no load of CoreCLR or
`wpfgfx_cor3.dll` for the console process. The same fixture failed in
desktop AutoCAD. This shows the file can load on this Wine installation
through the console path. The process comparison leaves the failing
desktop component open.

### Repeat desktop loading with AutoCAD safe mode

Use the same 2018 fixture, prefix and Wine runtime. The `startup-safe`
trial adds only AutoCAD's `/safemode` startup switch to the desktop
invocation. This temporarily restricts the executable code and add-ins
AutoCAD can load for the session. A successful open would justify
isolating the blocked components one at a time. The same fault site
would keep the shared desktop initialization and its runtime in scope.
[Autodesk safe-mode switch](https://help.autodesk.com/view/ACD/2027/ENU/?caas=caas%2Fdocumentation%2FACD%2F2014%2FENU%2Ffiles%2FGUID-8E54B6EC-5B52-4F62-B7FC-0D4E1EDF093A-htm.html)

~~~bash
unset ACAD_DWG_IO ACAD_EVENTPIPE ACAD_FAILFAST_OBSERVER
bash scripts/trace-autocad-dwg.sh baseline startup-safe "$ACAD_DWG_CONTROLS/line-2018.dwg"
~~~

Record whether the line drawing appears. If AutoCAD opens it, check the
read-only `SAFEMODE` system variable and close the program. Return the
filtered trial and fault lines. A run that reaches only the start screen
does not establish whether safe mode can open the drawing.

On 2026-11-20, the maintainer's safe-mode trial failed at the same
site, `coreclr.dll+0x14922d`, with exception `0xc0000409` and launcher
exit 9. The staged DWG was unchanged. The trace still recorded CoreCLR
and `wpfgfx_cor3.dll` loading. Restricting the session's executable
code by this switch did not resolve the failure.

### Test WPF software rendering in desktop AutoCAD

Use the same 2018 line drawing in a normal desktop startup. The
`startup-wpf-software` trial sets the copied prefix's
`HKCU\Software\Microsoft\Avalon.Graphics\DisableHWAcceleration`
DWORD to `1` and verifies that the setting survives a wineserver
restart. This tests WPF's renderer preference separately from AutoCAD's
own graphics settings. The helper restores the previous value or its
absence when it exits. It requires a default value or no value to make
the comparison meaningful. Close other apps in the test prefix first.
[Microsoft's WPF rendering setting](https://learn.microsoft.com/en-us/dotnet/desktop/wpf/graphics-multimedia/graphics-rendering-registry-settings#disable-hardware-acceleration-option)

~~~bash
unset ACAD_DWG_IO ACAD_EVENTPIPE ACAD_FAILFAST_OBSERVER
bash scripts/trace-autocad-dwg.sh baseline startup-wpf-software "$ACAD_DWG_CONTROLS/line-2018.dwg"
~~~

Record whether the line appears, plus `wpf_registry=`, `variant=`,
`fault_module=` and `wpf_registry_restored=`. If the same fault recurs,
the WPF software-rendering preference did not change the outcome. A
successful open warrants repeating the normal startup after restoration
to test whether the difference follows the setting.

On 2026-11-20, the maintainer confirmed `wpf_registry=software_only`
and reproduced `0xc0000409` at `coreclr.dll+0x14922d` with launcher
exit 9. The scratch DWG stayed unchanged and the helper restored the
value's absence. The rendering preference did not change the failure.

### Compare standalone .NET file I/O with a WPF document window

The managed probe workflow builds two small assemblies from our own
`managed-open-probe.cs`. Both require the x86_64 Windows .NET 10.0.0
runtime already in the AutoCAD test prefix. Both catch an expected
`FileNotFoundException` and read every byte of the same staged
`AC1032` file. The console control starts with `Microsoft.NETCore.App`.
The WPF probe starts with `Microsoft.WindowsDesktop.App`, waits until
its first window renders, reads the file, then renders a second window
on the same UI thread. Both exit on completion or after a time limit.

Use the `autocad-managed-open-probes` artifact from the
[managed probes workflow](../../../.github/workflows/build-autocad-managed-probes.yml).
Close AutoCAD and run this with the existing test work directory:

~~~bash
export ACAD_MANAGED_BUNDLE="$ACAD_AUTOCOMPLETE_WORK/managed-probes"
(cd "$ACAD_MANAGED_BUNDLE" && sha256sum -c SHA256SUMS)
bash "$ACAD_MANAGED_BUNDLE/run-autocad-managed-probes.sh" "$ACAD_DWG_CONTROLS/line-2018.dwg"
~~~

The runner uses the installed `Program Files\dotnet\dotnet.exe` and
reports missing host or framework directories without changing them.
It prints the runtime version, stages reached, file size, process exit
and the faulting image when the Wine trace contains it. Raw traces stay
in the private test directory. A shared fault in the console fixture
would support a smaller CLR or Wine reproducer. A WPF-only failure
would focus that reproducer on the window sequence. If both pass, the
basic managed file and window paths have a passing control and the
AutoCAD-specific desktop document activation remains open.

On 2026-11-20, both probes from
[build 44637686908](https://github.com/Harvi654/winum/actions/runs/44637686908)
ran on the maintainer's test prefix with .NET 10.0.0. Each caught the
controlled missing-file exception, read all 27,982 bytes of the DWG,
reported completion and exited with code 0. The WPF probe rendered
both windows. The passing controls cover basic exception handling,
managed file I/O and a WPF window sequence in that prefix.

### Record desktop OPEN events

Use the same baseline prefix and the 2018 line drawing. The helper
stages `dwg-open-events.lsp` in the scratch directory. It registers
transient command, DWG and document-manager reactors from the active
AutoLISP namespace. Each callback writes one known name to the scratch
log and closes it. It reads no document properties. Logging is limited
to callbacks delivered to that namespace. An absent event after a
document switch cannot establish that the application skipped that step.
[DWG reactors](https://help.autodesk.com/cloudhelp/2025/DEU/AutoCAD-MAC-AutoLISP-Reference/files/GUID-82B7F0D1-02AD-4463-A8A3-BF87EFCC83A4.htm)
· [Document manager reactors](https://help.autodesk.com/cloudhelp/2018/PTB/AutoCAD-AutoLISP-Reference/files/GUID-6C371938-A51E-43C8-B4A7-365A595DAB11.htm)

~~~bash
export ACAD_OPEN_EVENTS_LISP="$PWD/docs/wine-repro/shell32-autocomplete-timing/dwg-open-events.lsp"
unset ACAD_DWG_IO ACAD_EVENTPIPE ACAD_FAILFAST_OBSERVER
bash scripts/trace-autocad-dwg.sh baseline open-events "$ACAD_DWG_CONTROLS/line-2018.dwg"
~~~

Create a blank drawing and set `FILEDIA` to 0. Paste the AutoLISP line
printed by the helper. If AutoCAD blocks the load, report that result.
Run `._REGEN` to confirm command callbacks in the starting document,
then run `._OPEN` with the printed path. Return `variant=`, the
`open_event=` lines and `fault_module=`. Compare the fault signature with
the previous `OPEN` runs before interpreting the event sequence.

On 2026-11-20, the command trial reported `reactors_ready`,
`control_regen`, and `command_open`, then the known fail-fast at
`coreclr.dll+0x14922d`. The DWG and document-manager reactors reported
no further events in the starting namespace. The control proves the
command reactor was working. It cannot establish the timing of events
that were not delivered to that namespace.

### Compare the ActiveX document open path

Run `open-activex` with the same baseline, staged file and AutoLISP
probe. Paste the setup line in a blank drawing and run `._REGEN` as
before. Paste the second line from the helper in place of typing
`OPEN`. The probe calls `Documents.Open` directly with the staged path
and read-write access. It records entry and return and checks the name
of the returned document. The caller and document reactors remain
scoped to the active starting namespace.
[Autodesk ActiveX Open method](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-ActiveX-Reference/files/GUID-9ED7A548-7978-4BFD-8462-E2FA747E8880.htm)

~~~bash
unset ACAD_DWG_IO ACAD_EVENTPIPE ACAD_FAILFAST_OBSERVER
bash scripts/trace-autocad-dwg.sh baseline open-activex "$ACAD_DWG_CONTROLS/line-2018.dwg"
~~~

On 2026-11-20, the baseline ActiveX trial reported `reactors_ready`,
`control_regen`, `activex_begin`, then `activex_open_enter`. Desktop
AutoCAD exited with code 9 after `0xc0000409` at
`coreclr.dll+0x14922d`. The scratch DWG was unchanged. No return or
caught error was recorded. The command and ActiveX entry points both
reproduce the fail-fast with this fixture.

### Read a side database inside desktop AutoCAD

The `open-dbx` trial loads the same scratch DWG through the AutoCAD
2027 `ObjectDBX.AxDbDocument.26` interface in the existing desktop
process. It calls `Open` on a side database, counts `AcDbLine` objects
in model space, and checks the active drawing's name before and after.
Only stage markers, the count and the name comparison are logged. The
probe does not save the drawing or open it in a document tab.
[Autodesk 2027 ObjectDBX example](https://help.autodesk.com/cloudhelp/2027/ENU/AutoCAD-ActiveX-Reference/files/GUID-D9E0A89C-2D81-4141-8B88-B9AC6EAABD62.htm)

~~~bash
export ACAD_OPEN_EVENTS_LISP="$PWD/docs/wine-repro/shell32-autocomplete-timing/dwg-open-events.lsp"
unset ACAD_DWG_IO ACAD_EVENTPIPE ACAD_FAILFAST_OBSERVER
bash scripts/trace-autocad-dwg.sh baseline open-dbx "$ACAD_DWG_CONTROLS/line-2018.dwg"
~~~

Start a blank drawing, paste the setup line, run `._REGEN` and paste
the `winum-open-dbx` line. Close AutoCAD after the trial and return
`variant=`, `open_event=` and `fault_module=` from the summary.

On 2026-11-20, the maintainer ran the ObjectDBX call three times in one
baseline session. Each attempt delivered `:vlr-dwgfileopened` between
`dbx_open_enter` and `dbx_open_return`, found one line, and reported the
same active drawing name. The scratch DWG was unchanged and AutoCAD
exited with code 0. The desktop process can read the fixture and
enumerate its geometry through ObjectDBX. The comparison narrows the
failure to the path used to open an existing DWG as a desktop document.
It does not identify the point of failure within that path.

### Check creation and activation of a second desktop document

`new-document` calls `Documents.Add` from an already open blank
drawing, using AutoCAD's default template selection. It records the
change in document count and whether the new drawing became active.
It then calls `Activate` on the original drawing and on the new one
and compares the active drawing name after each call. Only markers
and the count difference are logged. Use an empty test session and
close both drawings without saving.
[Add method](https://help.autodesk.com/cloudhelp/2027/ENU/AutoCAD-ActiveX-Reference/files/GUID-6DB6EFDF-C393-4BD0-9E59-A57BCDA3456F.htm)
· [Activate method](https://help.autodesk.com/cloudhelp/2027/ENU/AutoCAD-ActiveX-Reference/files/GUID-8131A6DF-ED9F-4281-9207-C17BBDB187BF.htm)

~~~bash
export ACAD_OPEN_EVENTS_LISP="$PWD/docs/wine-repro/shell32-autocomplete-timing/dwg-open-events.lsp"
unset ACAD_DWG_IO ACAD_EVENTPIPE ACAD_FAILFAST_OBSERVER
bash scripts/trace-autocad-dwg.sh baseline new-document
~~~

Start with one blank drawing. Load the probe with the printed setup
line, run `._REGEN` and paste `winum-new-document` once. Report the
filtered summary and whether a second tab appeared. `new_count_delta=1`,
`new_activate_original_active`, and `new_activate_created_active`
show that general document creation and activation work in the same
desktop process. The matching fail-fast during Add or Activate would
give a smaller trigger. If markers stop at a document switch and the
process survives, check whether both tabs are visible and can be selected
manually. The probe starts in the original drawing's AutoLISP namespace.

On 2026-11-20, the maintainer confirmed that the second drawing
appeared and that both tabs could be selected manually. `Documents.Add`
returned with `new_count_delta=1`. The immediate ActiveX checks
reported `new_add_active=other`, `new_activate_original_active` and
`new_activate_created_other`. Document activation callbacks continued
after the last immediate check. The callbacks did not identify which
document became current. AutoCAD exited with code 0. The trial shows
that this baseline supports creating and switching desktop documents.

### Record SAVEAS command and file completion

`save-events` runs the baseline in the existing test prefix. It stages
a reactor in the starting drawing and supplies a fresh `saved.dwg`
path below `C:`. It observes `SAVEAS` command start, end, failure or
cancellation, and the drawing's `beginSave` and `saveComplete` events.
The probe reports whether the completion path matched the scratch
target, comparing case and slash variants locally. After the launcher
exits, it reports whether the DWG exists and, if so, its size and
six-byte format signature. No paths or drawing properties from the
callbacks are included in the summary.
[AutoLISP DWG reactors](https://help.autodesk.com/view/ACD/2027/ENU/?caas=caas%2Fdocumentation%2FACD%2F2014%2FENU%2Ffiles%2FGUID-82B7F0D1-02AD-4463-A8A3-BF87EFCC83A4-htm.html)

~~~bash
export ACAD_OPEN_EVENTS_LISP="$PWD/docs/wine-repro/shell32-autocomplete-timing/dwg-open-events.lsp"
unset ACAD_DWG_IO ACAD_EVENTPIPE ACAD_FAILFAST_OBSERVER
bash scripts/trace-autocad-dwg.sh baseline save-events
~~~

Start one blank drawing, set `FILEDIA` to 0 and load the printed setup
line. Draw a line, run `._REGEN` and use `._SAVEAS` in the default DWG
format with the printed path. Close AutoCAD if it remains open. Return
`variant=`, `save_event=`, `dwg_saved=`, `dwg_header=`, and any
`fault_module=` line. If `save_target_complete` precedes the matching
fail-fast, AutoCAD reported that it had saved the target before the
fault. Use the existing `core-open` trial on the written DWG to
verify that it can be reopened and still contains the line.

On 2026-11-20, the baseline `save-events` run delivered the command
start and `save_begin` callbacks. It produced an `AC1032` file of
31,498 bytes. The launcher exited with code 9 after `0xc0000409` at
`coreclr.dll+0x14922d`. The trace contains no `saveComplete` or
command-end callback. The delivered events place the failure after
save began. Reopening the output will determine whether the written
DWG can be read and contains the test line.

Set `ACAD_SAVED_DWG` to that run's `saved.dwg` below `base-prefix/drive_c`
and pass it to `core-open`. The launcher stages a copy before opening it.

~~~bash
bash scripts/trace-autocad-dwg.sh baseline core-open "$ACAD_SAVED_DWG"
~~~

On 2026-11-20, Core Console opened that same 31,498-byte `AC1032`
output on the baseline. It reported `core_console_dwg=1` and
`core_console_lines=1`, left the copy unchanged, and exited with code 0.
The crash therefore left a readable drawing with the test geometry.
The desktop trace still ends before a save or command completion
callback was delivered.

### Compare the ActiveX document save path

`save-activex` uses the same drawing, baseline and save reactors as
`save-events`. It calls `Document.SaveAs` on the active document,
explicitly requesting the 2018 DWG format used in the command trial.
The probe records entry, return, or a caught ActiveX error. The
launcher checks the fresh output file as in `save-events`.
[AutoCAD ActiveX SaveAs method](https://help.autodesk.com/cloudhelp/2027/ENU/AutoCAD-ActiveX-Reference/files/GUID-ED0D0B02-25FC-4ED2-9DEE-2B0C9AA9416A.htm)

~~~bash
export ACAD_OPEN_EVENTS_LISP="$PWD/docs/wine-repro/shell32-autocomplete-timing/dwg-open-events.lsp"
unset ACAD_DWG_IO ACAD_EVENTPIPE ACAD_FAILFAST_OBSERVER
bash scripts/trace-autocad-dwg.sh baseline save-activex
~~~

Use one blank drawing with `FILEDIA` set to 0. Load the printed setup
line, draw one line, run `._REGEN`, and paste the `winum-save-activex`
line once. Close AutoCAD if it stays open. Return `variant=`,
`save_event=`, `dwg_saved=`, `dwg_header=`, and any `fault_module=`.
A matching fail-fast after `activex_save_enter` reproduces the crash
through both desktop save entry points. An `activex_save_return` and
a readable output show that this ActiveX route can complete the save.

On 2026-11-20, the baseline ActiveX trial reached `activex_save_enter`
and `save_begin` and produced a 32,046-byte `AC1032` file. The launcher
exited with code 9 after `0xc0000409` at `coreclr.dll+0x14922d`. The
trace contains no save completion, method return, or caught ActiveX
error. Both desktop save entry points reproduce the failure. The
earlier Core Console readback covered the command trial's output.

### Compare Shell activity during ActiveX open and save

The desktop `Documents.Open` and `Document.SaveAs` paths fail at the
same CoreCLR offset. Desktop document creation, tab switching, and
ObjectDBX reads succeeded. Check whether the two failing operations
show shared activity at the Windows Shell boundary before attributing
the failure to file metadata or recent-document integration.

Set `ACAD_DWG_SHELL_TRACE=1` and repeat `open-activex` and `save-activex`
as separate launches on the same baseline. Use the same 2018 line DWG
for open and a new one-line drawing for save. The opt-in adds the
Wine `shell` channel for `acad.exe` and counts messages from a fixed
set of Shell functions across the run. It reports process, fatal-thread
and scratch-path counts. The scratch-path count requires a message to
print the unique scratch directory name. Some messages log only
pointers, and a function may emit multiple messages for a single call.
These counts describe observed trace output. Each raw log stays in the
private run directory, and the summary omits function arguments.

~~~bash
(
    export ACAD_OPEN_EVENTS_LISP="$PWD/docs/wine-repro/shell32-autocomplete-timing/dwg-open-events.lsp"
    export ACAD_DWG_SHELL_TRACE=1
    unset ACAD_DWG_IO ACAD_EVENTPIPE ACAD_FAILFAST_OBSERVER
    bash scripts/trace-autocad-dwg.sh baseline open-activex "$ACAD_DWG_CONTROLS/line-2018.dwg" || [[ $? -eq 9 ]] || exit 1
    bash scripts/trace-autocad-dwg.sh baseline save-activex
)
~~~

For each run, return `variant=`, the `open_event=` or `save_event=`
markers, `fault_module=`, `shell_trace_acad_processes=`, and `shell_api=`.
Compare the fault signature before using the counts to select a new
probe. A shared message pattern needs a targeted follow-up to establish
whether it contributes to the failure.

On 2026-11-20, both baseline ActiveX trials again exited with code 9
after `0xc0000409` at `coreclr.dll+0x14922d`. `SHAddToRecentDocs`
produced seven trace lines during Open and eight during Save As. Each
run included one line with its scratch path and zero lines on the
fatal thread. The other four watched functions produced zero lines.
The traces establish that both paths reached recent-document
processing on another thread. They leave its role in the crash open.

### Skip Windows Shell recent-document history

Use `ACAD_DWG_NO_RECENT=1` on the same baseline test prefix. The
script temporarily sets `NoRecentDocsHistory` to `REG_DWORD` 1 in the
Explorer policy key selected by Wine 11.22. It uses the machine key
when present and the user key otherwise. Wine's `SHAddToRecentDocs`
checks this value before resolving the document path or recording
history. This isolates the Shell's history processing from AutoCAD's
own recent-file list.
[Wine 11.22 shell32 implementation](https://gitlab.winehq.org/wine/wine/-/blob/wine-11.22/dlls/shell32/shellord.c)

The script verifies the temporary value before launch, enables the
same `shell` trace, and restores the original zero or removes the
value when it was absent. It removes keys it created if still empty.
The scratch prefix should be used by one trial at a time.

A synthetic caller on winum-11.22-1 reached the normal path trace
with the policy absent and the early-return trace with it enabled.
The wrapper restored an absent policy and an existing machine DWORD
of zero. It preserved the user value and refused a machine value of
an unexpected type.

~~~bash
(
    export ACAD_OPEN_EVENTS_LISP="$PWD/docs/wine-repro/shell32-autocomplete-timing/dwg-open-events.lsp"
    export ACAD_DWG_NO_RECENT=1
    unset ACAD_DWG_IO ACAD_EVENTPIPE ACAD_FAILFAST_OBSERVER
    bash scripts/trace-autocad-dwg.sh baseline open-activex "$ACAD_DWG_CONTROLS/line-2018.dwg" || [[ $? -eq 9 ]] || exit 1
    bash scripts/trace-autocad-dwg.sh baseline save-activex
)
~~~

`shell_recent_policy_skip_lines` above zero confirms that AutoCAD's
Shell calls reached the policy branch. Zero `scratch_path_lines` for
`SHAddToRecentDocs` confirms that no matching path-processing message
was emitted. Compare `fault_module=` and the open or save completion
markers with the earlier pair. Record `recent_history_restored=` for
both runs. A changed outcome would need a repeat after restoring the
policy to confirm the association.

On 2026-11-20, `open-activex` and `save-activex` both reached the
policy branch once and neither emitted a scratch-path message from
`SHAddToRecentDocs`. Each still exited with code 9 after `0xc0000409`
at `coreclr.dll+0x14922d`. The temporary policy was removed after
each run. Save As produced a 32,046-byte `AC1032` file before the
crash, without a recorded completion. Shell recent-document history
processing is unnecessary to reproduce either crash.

### Compare the desktop operations on native Windows

Use native AutoCAD 2027 at the same product update level when that
installation is available. This is a program check. If AutoCAD requires
sign-in, use the separate online snapshot and leave the offline MEASURE
snapshot for Windows API tests.

Put `run-dwg-desktop-check.bat`, `dwg-open-events.lsp`, and the
SHA-256-pinned `line-2018.dwg` from the earlier control in one writable
folder. Run the batch file. It verifies the DWG, stages copies in a new
temporary directory, and prints the same ActiveX calls used under Wine.
Run each in a separate AutoCAD session. If the AutoLISP load is blocked,
record that as a setup result. The batch file writes
`result-autocad-dwg-windows.txt` with the Windows version and selected
markers. Return only that text, the AutoCAD version from About, and
whether the drawing opened, saved, or crashed. Leave DWGs and raw
logs in the VM.

On 2026-11-20, native AutoCAD on Windows 10 build `19045.6466` completed
both trials with the SHA-256-verified fixture. In the open trial,
`documentToBeActivated` and `documentBecameCurrent` were delivered
between `activex_open_enter` and `activex_open_return`. The probe
verified the returned document name. In the save trial, AutoCAD
reported `save_begin`, `save_target_complete`, and `activex_save_return`
in order. The maintainer saw the drawing take the name `saved.dwg`,
and the output was 32,596 bytes.

| Operation | Native Windows | Baseline winum 11.22-1 with the MSXML3 fix |
| --- | --- | --- |
| ActiveX Open | Document activation callbacks and method return | Fail-fast after method entry, with no activation callback delivered to the starting namespace |
| ActiveX Save As | File completion callback and method return | File written, then fail-fast without a completion callback or method return |

About reported `X.60.0.0 AutoCAD 2027` on both native Windows and the
baseline prefix on Arch Linux. The desktop calls complete on Windows
and fail on winum in the same AutoCAD product build. The shared CoreCLR
offset identifies where the process terminates. A Wine fix needs a
smaller reproducer that isolates the triggering compatibility difference.

### Compare the installed AutoCAD and .NET files

Build `autocad-file-inventory.cpp` with a C++17 compiler and the
OpenSSL development files on Linux. Build the same source on Arch with
64-bit MinGW for the Windows 10 VM. The Windows executable uses
Windows CNG for SHA-256 and runs without AutoCAD, .NET or administrator
permissions. The Linux executable reads the tested prefix directly.

~~~bash
export ACAD_FILE_TOOLS="$PWD/docs/wine-repro/shell32-autocomplete-timing"
export ACAD_FILE_OUT="$ACAD_AUTOCOMPLETE_WORK/file-comparison"
mkdir -- "$ACAD_FILE_OUT"
c++ -std=c++17 -O2 -Wall -Wextra -Werror \
    "$ACAD_FILE_TOOLS/autocad-file-inventory.cpp" -lcrypto -o "$ACAD_FILE_OUT/autocad-file-inventory"
x86_64-w64-mingw32-g++ -std=c++17 -O2 -Wall -Wextra -Werror -static -municode \
    "$ACAD_FILE_TOOLS/autocad-file-inventory.cpp" -lbcrypt -o "$ACAD_FILE_OUT/autocad-file-inventory.exe"
cp -- "$ACAD_FILE_TOOLS/run-autocad-file-inventory.bat" "$ACAD_FILE_OUT/"
~~~

Close AutoCAD on both systems. In the offline Windows snapshot with the
reference installation, put the batch file and `.exe` in a writable
folder and run the batch file. It defaults to 64-bit `Program Files`
and writes `windows-autocad-files.tsv`. An installed copy is sufficient
and AutoCAD does not need to be started or signed in. Transfer only that
generated text inventory to `ACAD_FILE_OUT` on Arch. The inventory
contains paths relative to the program roots, byte counts, and SHA-256
digests. Hashes are calculated for whole files without parsing their
contents. The tool skips filesystem links and omits user profiles and
Windows system files.

The inventory covers program binaries, configuration files and drawing
templates in `Autodesk/AutoCAD 2027`. It also covers x64 .NET 10
`host/fxr`, `Microsoft.NETCore.App`, and `Microsoft.WindowsDesktop.App`
for each installed 10.x version, plus `dotnet.exe`. Three entries at
the `Program Files` level check for known installer misplacements
involving `AcJab.dll`, `AdMigrator.xml`, and `AcCommandWeight.xml`.

~~~bash
"$ACAD_FILE_OUT/autocad-file-inventory" \
    "$ACAD_AUTOCOMPLETE_WORK/base-prefix/drive_c/Program Files" > "$ACAD_FILE_OUT/prefix-autocad-files.tsv"
python3 -I "$ACAD_FILE_TOOLS/compare-autocad-files.py" \
    "$ACAD_FILE_OUT/windows-autocad-files.tsv" "$ACAD_FILE_OUT/prefix-autocad-files.tsv" \
    "$ACAD_FILE_OUT/local-differences.tsv"
~~~

Return only the printed `root=`, `group=` and `component=` summary.
Keep the full inventories and `local-differences.tsv` private. Differences
can reflect language, configuration or installed package choices. A
matching hash confirms bytes for that file. Registry and COM registration
state need separate checks.

On 2026-11-20, all 1,216 inventoried AutoCAD files matched between the VM
and the baseline prefix. There were no differing files or files unique
to either side in the AutoCAD directory. `acad.exe`, `AcJab.dll`, and
the two other probed `acadprivate` files matched at their intended paths.
The known misplaced copies were absent on both sides. In the .NET
inventory, the VM had x64 10.0.15 and the prefix had 10.0.0, including
hostfxr, CoreCLR and Windows Desktop. Each runtime contributed 475
files at version-specific paths. The shared `dotnet.exe` path also
had a different hash. This verifies the measured AutoCAD payload and
identifies a concrete runtime difference. The inventory records installed
files and does not establish what the process loaded.

### Test the baseline prefix with .NET 10.0.15

Keep winum and the AutoCAD installation fixed. Close AutoCAD before
running `scripts/prepare-autocad-dotnet-10.0.15.sh`. It verifies the
official Windows x64 Core and Desktop ZIPs, then clones the baseline
prefix alongside the original on the same filesystem. The clone adds
the .NET 10.0.15 host, Core and Desktop version directories and takes
the matching `dotnet.exe`. Its older runtime directories remain available
for normal framework selection. The original `base-prefix` and Wine
runtime are untouched. The space check allows for a full copy if the
filesystem cannot reflink.
[Core archive and vendor checksum](https://dotnet.microsoft.com/en-us/download/dotnet/thank-you/runtime-10.0.15-windows-x64-binaries-zip)
· [Desktop archive and vendor checksum](https://dotnet.microsoft.com/en-us/download/dotnet/thank-you/runtime-desktop-10.0.15-windows-x64-binaries-zip)

~~~bash
bash scripts/prepare-autocad-dotnet-10.0.15.sh
export ACAD_DOTNET_TEST="$ACAD_AUTOCOMPLETE_WORK/dotnet-10.0.15"
export ACAD_OPEN_EVENTS_LISP="$PWD/docs/wine-repro/shell32-autocomplete-timing/dwg-open-events.lsp"
(
    export ACAD_AUTOCOMPLETE_WORK="$ACAD_DOTNET_TEST"
    unset ACAD_DWG_IO ACAD_EVENTPIPE ACAD_FAILFAST_OBSERVER ACAD_DWG_SHELL_TRACE ACAD_DWG_NO_RECENT
    bash scripts/trace-autocad-dwg.sh baseline open-activex "$ACAD_DWG_CONTROLS/line-2018.dwg" || [[ $? -eq 9 ]] || exit 1
    bash scripts/trace-autocad-dwg.sh baseline save-activex
)
~~~

Use the same ActiveX open and save steps as in the baseline trials.
The `dotnet_loaded=` lines report versions from native module load paths
in the AutoCAD process without exposing the paths. Require the CoreCLR
line to report `version=10.0.15` before attributing any change to the
updated runtime. Compare method returns, save completion, and the
exception code. Module-relative addresses can differ between the two
CoreCLR binaries. Retain the baseline copy until the paired result is
recorded and remove the extra private clone when it is no longer needed.

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
