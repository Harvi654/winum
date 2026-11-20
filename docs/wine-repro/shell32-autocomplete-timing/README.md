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

On 2026-11-20, both desktop trials loaded native `coreclr.dll`,
`hostfxr.dll`, `hostpolicy.dll` and `wpfgfx_cor3.dll` from 10.0.15.
Open reached `activex_open_enter` without a return. Save reached
`save_begin` and wrote a 32,078-byte `AC1032` file without a completion
event or method return. Both exited with code 9 and exception
`0xc0000409` at `coreclr.dll+0x1855fd`. The previous 10.0.0 runs had
the same outcomes at `coreclr.dll+0x14922d`. Upgrading the runtime
alone leaves the desktop failure in place.

### Compare existing Wine loader diagnostics

Run the loader summary using the original test work directory. It
selects the latest available `acad-*.log` for baseline idle, Open and
Save and for Open and Save in the 10.0.15 copy. It counts explicit
module-load errors, missing imports and dynamic exports, stub allocations
and initialization failures for `acad.exe` before its first unhandled
exception. The reported names contain only module basenames and matching
export tokens. Full paths stay in the logs.

~~~bash
python3 -I scripts/summarize-autocad-loader.py "$ACAD_AUTOCOMPLETE_WORK"
~~~

Return `loader_run=`, `loader_event=`, and `loader_omitted=` lines.
`fault_thread` is the number of matching messages in the eventual
failing thread. `idle_seen` uses the baseline startup-only trace as a
noise reference, also for runs using the newer runtime. These reports
cover the whole captured run up to the exception. An event unique to
one trial needs a more focused trace to identify its timing or effect.

On 2026-11-20, the idle run recorded 235 loader messages and 94 distinct
signatures. The four faulting runs recorded 243 to 248 messages and 94
to 95 distinct signatures. All listed signatures on a failing thread
also appeared in the idle run. That includes `crypt32.dll` and
`sspicli.dll` probes. The `imm32.dll` probes and `user32.dll` lookups of
`GetCursorPosW` and `SetMessageExtraInfoW` were on other threads.
Wine exports `GetCursorPos` and `SetMessageExtraInfo` without suffixes,
matching the documented API names. The suffixed probes alone do not
identify a missing API or a cause of the crash.
[GetCursorPos](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getcursorpos)
· [SetMessageExtraInfo](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setmessageextrainfo)

### Compare file lookups during ObjectDBX and desktop Open

Use the baseline prefix and the same 2018 line fixture for both runs.
`ACAD_DWG_FILE_TRACE=1` enables Wine's `file` channel only for `acad.exe`.
The printed AutoLISP setup opts into separate marker files immediately
before and after the call. The summary counts Wine's `NtCreateFile`
name-resolution warnings from the entry marker to the return, caught
error, or unhandled exception. It reports the location class, selected
program basenames, status, and count. Paths in user directories stay
private. This covers those explicit file lookup warnings. A successful
name resolution can still be followed by a failed file open.

~~~bash
(
    export ACAD_OPEN_EVENTS_LISP="$PWD/docs/wine-repro/shell32-autocomplete-timing/dwg-open-events.lsp"
    export ACAD_DWG_FILE_TRACE=1
    unset ACAD_DWG_IO ACAD_EVENTPIPE ACAD_FAILFAST_OBSERVER ACAD_DWG_SHELL_TRACE ACAD_DWG_NO_RECENT
    bash scripts/trace-autocad-dwg.sh baseline open-dbx "$ACAD_DWG_CONTROLS/line-2018.dwg" || exit 1
    bash scripts/trace-autocad-dwg.sh baseline open-activex "$ACAD_DWG_CONTROLS/line-2018.dwg"
)
~~~

Use the same blank drawing setup for both trials, run `._REGEN` and
paste the second printed line once. After the ObjectDBX run returns,
close AutoCAD normally. Compare `file_window=return` with
`file_window=fault`; a missing marker means the window was not captured.
Report `variant=`, `open_event=`, `file_window=`, `file_lookup=`,
`file_omitted=` and `fault_module=`. A lookup specific to the desktop
Open path is a lead for comparison with the installed files or an
explicit API measurement.

On 2026-11-20, the ObjectDBX process exited with code 0. The event log
contains two Open returns, two one-line counts, and two unchanged active
document checks. The file summary reports zero warnings for the first
completed ObjectDBX Open window. The maintainer also captured an AutoLISP
console error reporting `0xc0000005` during the ObjectDBX trial. The
current markers end at the return from ObjectDBX Open, before model-space
enumeration and before `vlax-release-object`. They cannot place that error
relative to cleanup.

Desktop Open reached `0xc0000409` at `coreclr.dll+0x14922d`. Its window
contained 113 `NtCreateFile` warnings. Of these, 96 were
`STATUS_OBJECT_NAME_COLLISION` (`0xc0000035`), with 84 on the calling
thread. The other 17 were `STATUS_OBJECT_NAME_NOT_FOUND` (`0xc0000034`),
all on other threads. The original summary grouped collisions with
failed lookups but omitted request dispositions and object types.

### Classify the saved file warnings and ObjectDBX exceptions

Use `scripts/summarize-autocad-file-window.py` on the original test work
directory. It rereads the two latest saved logs and matches each warning
to the `NtCreateFile` request by thread and path. It reports disposition,
file or directory flags, broad location, a distinct-path count, and the
counts for the calling and failing threads. It prints no file paths or
names. Each ObjectDBX call gets its own window. The access violation
counts are SEH messages before the first call, within calls, and after an
Open return. The last phase also includes later UI actions and shutdown.

~~~bash
python3 -I scripts/summarize-autocad-file-window.py "$ACAD_AUTOCOMPLETE_WORK"
~~~

Return `file_detail=`, `file_request=`, `file_request_omitted=`, and
`access_violation_phase=`. A collision on a create-only request can be an
ordinary attempt to create an existing directory. Confirm its request
type before treating these counts as a lead. Investigate ObjectDBX cleanup
separately if the exception messages cluster after the return.

The follow-up on 2026-11-20 classified 92 collisions as create-only
requests for four directories and four as create-only requests for one
file. None of the logged file warnings occurred on the thread that
reported the fatal exception. The ObjectDBX trace contains two
access-violation messages within the two recorded calls and eleven
after a return, all on the caller thread. Desktop Open contains two
within its call, also on the caller thread. These are trace messages,
and repeated messages may refer to one exception.

The same summary now reports `access_violation_site=` using the module
and relative offset printed by Wine, or a unique active PE image range
from the loader trace. Addresses with no match are unresolved. Return
`file_detail=`, `access_violation_phase=` and `access_violation_site=`
for both saved runs. `fault_thread` identifies a message on the thread
that eventually reported the unhandled exception. Matching sites can
identify a shared earlier path. Their appearance in a returning call
also establishes that reaching the site can be survivable.

The saved ObjectDBX call and post-return messages resolved to
`oleaut32.dll+0x1a481`. The desktop Open call had one access violation
message at `accore.dll+0x5cc8c3` and one with an unresolved address.
Both were on the calling thread. Neither appeared on the thread that
reported the terminal `0xc0000409`. `oleaut32.dll+0x1a481` also appeared
before Open, and `adwindows.dll+0x303edb` appeared before both calls.
The measured in-call sites differ between the two paths. The terminal
thread is a stronger lead for the common Open and Save As failure.

Read the latest saved Open and Save As traces from both runtimes with
`--fault-chain`. This reports up to eight exception dispatch messages
from the eventual fatal thread in order, with the observed instruction
sites and only small fast-fail or HRESULT-shaped CLR arguments when
logged. The argument remains a raw numeric observation. Other threads,
absolute addresses, paths and register dumps stay out of the summary.

~~~bash
python3 -I scripts/summarize-autocad-file-window.py --fault-chain "$ACAD_AUTOCOMPLETE_WORK"
~~~

Return only `fault_chain=` and `fault_dispatch=`. A terminal exception
without preceding dispatch messages bounds what the saved trace can
show. An earlier dispatch shared by both operations and runtime
versions would give a more specific compatibility probe.

On 2026-11-20, all four saved runs reported zero earlier dispatch
messages on the eventual fatal thread. On .NET 10.0.0, both Open and
Save As still terminated at `coreclr.dll+0x14922d`. With 10.0.15, both
terminated at `coreclr.dll+0x1855fd`. All four terminal records were
`0xc0000409`. Wine 11.22 sends the x86_64 fast-fail interrupt directly
to `NtRaiseException` with `first_chance` set to false. In a separate
winum-11.22-1 run, an original x86_64 program calling `__fastfail`
also produced the terminal record with zero dispatch messages.
The result describes the visibility of the saved SEH trace. The earlier
debug observer reported a CLR first-chance exception on the fatal thread
during a separate Save As trial.

### Investigation checkpoint

The matched AutoCAD 2027 `X.60.0.0` desktop operations complete on
Windows 10 `19045.6466` and fail under winum-11.22-1 with MSXML3 PR #4
(`aeb7a59`). The inventoried AutoCAD files match by SHA-256. Installing
the VM's .NET version in a sibling prefix reproduces both failures.
ObjectDBX reads the fixture and the Core Console opens the DWG written
before the desktop crash. The bounded Shell, loader, file and SEH traces
provide no specific failing Wine API to reproduce independently. The
best remaining hypothesis is a compatibility difference somewhere in
the desktop document lifecycle leading to a fast-fail on another thread.

Keep the DWG test recipes and the baseline intact. Resume when there
is a comparable newer Wine build or a specific API difference to test.
For the build comparison, clone the same baseline, use the same MSXML3
fix and .NET version, change only the Wine runtime, and run the same
`open-activex` and `save-activex` trials. Match successful completion
against the Windows markers above. A Wine patch needs a separate
reproducer that distinguishes Wine from the Windows API behavior.

### Compare the Drawing Recovery registry read

The Windows `SAVEAS` registry capture compared against the .NET 10.0.15
test prefix listed 51 Windows hits missing from the Wine registry
snapshot. One was flagged by the comparison tool's Autodesk filter,
the `LoggedFileCount` value below the active AutoCAD profile's
`Drawing Recovery` key. This is a static difference. The comparison
does not show whether AutoCAD queries it during Save As under Wine.

Use the .NET 10.0.15 test directory and run `save-events` with
`ACAD_DWG_RECOVERY_TRACE=1`. The script enables registry tracing for
`acad.exe` and uses the existing AutoLISP reactor to mark the Save As
command window. Follow the printed instructions with `FILEDIA=0` and
save to the supplied scratch path. Return `recovery_save_window=`,
`recovery_query=`, `variant=`, and `fault_module=`. The summary counts
`NtQueryValueKey` calls by value name and phase. It also counts calls
from the fatal thread. It does not report raw registry data, the full
key path or the query's return status. A query in the command window
is a candidate for a further check with the exact key and measured
Windows value. A confirmed window with zero queries lowers its priority.

On 2026-11-20, the .NET 10.0.15 prefix created an `AC1032` DWG and
then exited with `0xc0000409` at `coreclr.dll+0x1855fd`. The reactor
confirmed the Save As command window up to the crash. The registry
trace counted three `LoggedFileCount` queries before the command and
zero during it. None of the three ran on the fatal thread. This lowers
the priority of the value as a direct trigger and does not justify
adding it to the prefix. The trace leaves any effect of the earlier
queries and the remaining registry differences open.

### Compare with the installed system Wine

Use `scripts/prepare-autocad-system-wine-ab.sh` with the .NET 10.0.15
test directory, `/usr` for the installed Wine on Arch, and a new empty
directory on the test disk. Check space for two complete prefix copies
and close any application using the source prefix. The script creates
a reference copy for the existing winum runtime and a candidate copy
for the system runtime. It runs `wineboot -u` with `mscoree=d` on each
copy and then disables automatic prefix updates there. It verifies that
the native `mscoree.dll` and Framework CLR files for both architectures
and the .NET 10.0.15 `coreclr.dll` have kept the source prefix hashes.

For both launches use `mscoree=n` in `WINEDLLOVERRIDES`. Run
`trace-autocad-dwg.sh baseline save-events` and then
`trace-autocad-dwg.sh candidate save-events` with `FILEDIA=0` and the
provided scratch paths. Verify the same hashes again after each run.
The trace reports the `mscoree`, `clr`, `coreclr`, and Mono runtime DLLs
when the desktop process loads them. An absent Framework DLL line can
mean the operation used only the separately installed .NET 10 runtime.
Compare Save As completion and the crash site. If system Wine exits
before it reaches Save As, record where startup stopped. If the system
version differs from the reference, the pair compares complete Wine
builds, including the version and the patch set.

On 2026-11-20, the maintainer compared winum `aeb7a59` with the installed
system Wine. Both reported Wine 11.22. The preparation verified all
five selected .NET file hashes in both copies. Both desktop processes
loaded native `mscoree.dll` and .NET 10.0.15 CoreCLR. Winum reached
`save_begin`, wrote the scratch DWG, and exited with `0xc0000409` at
`coreclr.dll+0x1855fd`. Both system Wine attempts exited with status 1
before reaching the Save As reactor. The visible `.NET Assertion Failed`
dialog named `AcSbGridDisplay` in status bar XAML initialization. This
matches the earlier [AutoCAD URLMON startup symptom][dwg-urlmon-startup].
The system Wine trial ended before answering the Save As comparison.

For a Save As comparison with the [three-patch Wine 11.22 build][dwg-review-startup-b],
prepare only the candidate copy from the same .NET 10.0.15 source test
directory. Pass the unpacked `c5ddcfe` runtime as the second argument
and `--candidate-only` as the last argument to the existing preparation
script. Retain `mscoree=n` during `candidate save-events` and verify
the hashes after the run. The known `aeb7a59` reference result above
provides the Save As control for this follow-up. The three-patch build
contains the two URLMON startup fixes and MSXML3 PR #4.

On 2026-11-20, `c5ddcfe` reached `:vlr-commandwillstart` and `save_begin`
on the fresh candidate copy. It wrote a 31,434-byte `AC1032` scratch DWG
and exited with launcher status 9 and exception `0xc0000409` at
`coreclr.dll+0x1855fd`, matching the full `aeb7a59` build on .NET 10.0.15.
No save completion event was recorded. The preparation reported unchanged
hashes for the five selected .NET files. The desktop trace confirmed
native `mscoree.dll` and .NET 10.0.15 CoreCLR. Post-run hashes were not
captured in the submitted summary.

The omitted patches are unnecessary to reproduce Save As as well as
desktop Open with the tested prefix. The three retained patches, the
copied prefix state and Wine 11.22 remain in scope. System Wine 11.22
still has only a startup result.

#### Compare the Save As operation window

Use `ACAD_DWG_API_TRACE=1` with `candidate save-events` on the `c5ddcfe`
Wine 11.22 test copy running .NET 10.0.15. This adds Wine `fixme` and
`err` lines for `acad.exe` and uses AutoLISP marker files to delimit the
command. Leave EventPipe, the debugger observer and full relay off
for this run. The summary reports counts before the command, from
command entry to `save_begin`, and from `save_begin` to command exit
or the first reported unhandled exception. It also reports how many
in-window lines came from the eventual fault thread. The summary prints
Wine source labels and selected exception codes. `stub_hint` means the
message contains an implementation-related term. It does not establish
what the function returned. `before` includes startup and the preparatory
`REGEN`. Lines that also occur before the command may still matter if
they have a different caller or inputs during the command.

For the Windows control, put `run-dwg-save-window.bat` and
`dwg-open-events.lsp` in a fresh folder and double-click the batch file.
Use the same `FILEDIA=0` Save As command with a blank drawing and one
line. In Process Monitor, include `Process Name is acad.exe`, clear the
capture after `REGEN`, and keep file and registry activity through the
return of Save As. Locate the marker files ending in `save_command_enter`,
`save_begin`, and `save_command_exit`. They delimit the same phases as on
Wine. Inspect `saved.dwg` and the nearby temporary files first, looking
for create, rename and close results. `result-autocad-save-window.txt`
records the operation events. Keep the raw capture local.

Look first for a new repeatable Wine implementation hint or error in
the saving phase, especially on the fault thread. Process Monitor can
verify filesystem and registry behavior. It does not show arbitrary API
returns. For a lead outside that scope, trace the specific Wine API and
build a small synthetic test of the same contract on both systems before
changing Wine. If the logged symptoms provide no lead, the result bounds
this diagnostic run and leaves silent behavioral differences open.

On 2026-11-20, the reduced build produced a 31,467-byte `AC1032` DWG,
then reached the same fail-fast at `coreclr.dll+0x1855fd`. The only new
implementation hint after `save_begin` was one
`shell:IPersistFile_fnSaveCompleted` message on a different thread. In
Wine 11.22 this is the Shell Link object's completion method. It logs
a stub message and returns `S_OK`. `SHAddToRecentDocs` calls this method
after writing the `.lnk` file. The summary alone does not identify the
caller. An earlier test with Shell recent history disabled retained the
ActiveX Save As crash on the full build.
[Wine Shell Link implementation](https://gitlab.winehq.org/wine/wine/-/blob/wine-11.22/dlls/shell32/shelllink.c)

`EtwEventWrite` also appeared during startup and the command, and
returns `ERROR_SUCCESS` without writing an event in Wine 11.22.
`GetTempPath2W` appeared during startup and before `save_begin` and
delegates to `GetTempPathW` in this Wine version. The OLE clipboard
message also appeared before the command. The only logged error on the
fatal thread was the terminal `NtRaiseException` message.

For a direct control, repeat `candidate save-events` once with both
`ACAD_DWG_API_TRACE=1` and `ACAD_DWG_NO_RECENT=1` in the same test copy.
The existing policy helper now supports this trial on the candidate
copy and restores the original setting. The Shell summary counts
`IPersistFile_fnSaveCompleted` messages across the desktop process.
Check `shell_recent_policy_skip_lines`, the count for that method, the
save events, the terminal exception, and `recent_history_restored`.
If the method disappears and the crash persists, leave this Shell
history path out of further Save As patch work. The native Windows
capture above can still locate differences in post-write file work.

[dwg-urlmon-startup]: https://github.com/Harvi654/Autodesk-AutoCAD-on-Linux/blob/135b273/docs/autocad-2027-status.md#fixed-along-the-way

### winum patch review

The review used the [series from the crashing build][dwg-review-series]
(`aeb7a59`, Wine 11.22). The [November 17 AutoCAD notes][dwg-review-history]
already record `SAVEAS` with `FILEDIA=0` writing a DWG and then ending
with `0xc0000409` on the winum-11.22-1 release before MSXML3 PR #4.
The autocomplete candidate also reproduced the failure with `FILEDIA=0`.
The breadcrumb and background-menu candidates were absent from this
build. Disabling recent-document history left both crashes reproducible.

- [`combase-stub-activation-context`][dwg-review-combase] was the first
  patch isolated. It activates the context saved when the stub manager
  was created for incoming RPC stub invocations. The Windows measurement
  found no activation context at an STA server and the caller's context
  at an MTA server. The saved context can change class or DLL resolution.
  The ActiveX trials establish the entry point into AutoCAD, while the
  traces leave use of this patched RPC path unconfirmed.
- [`oleaut32-typelib-keep-released`][dwg-review-typelib] keeps file-backed
  type libraries in a bounded cache after their last release. The
  `oleaut32.dll+0x1a481` access violations also appeared around ObjectDBX
  calls that returned successfully. The desktop Open trace places its
  call-time access violations outside the eventual fatal thread. The
  saved offsets do not identify a cache operation. Confirm the loaded DLL
  kind and look for type-library activity before treating it as a match.
- The `rpcrt4-ndrstubcall3`, `ntdll-locate-xstate-feature-enabled`, and OLE
  structured-storage patches are lower priority. The traces have yet to
  show an affected `NdrStubCall3`, extended-context lookup or storage
  call in the failing operation. The series leaves Wine's x86_64
  fast-fail dispatch unchanged.

For the first patch comparison, use the [reference build][dwg-review-a]
and the [build without the COM patch][dwg-review-b]. The preparation
script verifies both archives and compares their patch manifests. It uses
the reference Wine runtime in both trials and stages only the x86_64 and
i386 `combase.dll` from the comparison build in the candidate runtime
and prefix. Both trials keep MSXML3 PR #4, the URLMON startup fixes,
and the same .NET version from the previous test prefix.

Close AutoCAD before copying the previous test prefix. Pass the previous
test directory with `base-prefix` and `dwg-controls/line-2018.dwg` to
`scripts/prepare-autocad-dwg-ab.sh combase` along with a new empty
directory on the test disk. Check that the disk can fit two copies of
the prefix.
Start with `open-activex` on each copy, one launch at a time. Set
`ACAD_AUTOCOMPLETE_WORK` to the new directory and `ACAD_OPEN_EVENTS_LISP`
to `dwg-open-events.lsp` in this directory of the repository. Pass
`$ACAD_AUTOCOMPLETE_WORK/line-2018.dwg` to `trace-autocad-dwg.sh` with
`baseline open-activex` and `candidate open-activex` in turn. Follow the
printed AutoLISP steps once per launch. A successful Open should report
`open_event=activex_open_return` and `open_event=activex_open_expected_dwg`.
Compare `variant=`, `open_event=`, the `combase.dll` load, `dotnet_loaded=`,
`unhandled` and `fault_module=`. Record startup separately if an arm
cannot reach the operation. Use a fresh pair for `save-activex` if Open
changes either prefix's application state. Save success needs the return
marker and DWG readback because the baseline already writes a file.

#### COM stub activation context result

On 2026-11-20, desktop `open-activex` reached `activex_open_enter` on
both copies. Both loaded built-in `combase.dll` and .NET `10.0.0`,
terminated with `0xc0000409` at `coreclr.dll+0x14922d`, and returned
launcher status 9. The fixture was unchanged in both copies. Neither
trial reached `activex_open_return`. The Open crash reproduced with
the COM stub patch omitted. Save As was outside this paired run.

#### Type library cache comparison

The [second comparison build][dwg-review-cache-b] omits only
`oleaut32-typelib-keep-released.patch` from `aeb7a59`. It includes the
COM stub patch and both `oleaut32-typedesc` patches. Check for a built-in
`oleaut32.dll` load in the original Open trial before staging the DLLs.
Use `scripts/prepare-autocad-dwg-ab.sh oleaut32` with the original
test directory and a fresh empty directory. Repeat `open-activex`
against both copies and record the `oleaut32.dll` load alongside the
return or crash markers. A successful comparison merits a repeat
and a separate Save As pair. Another identical Open failure lowers
the priority of the cache hypothesis.

On 2026-11-20, the maintainer verified both runtime archives and recorded
distinct `oleaut32.dll` hashes for both architectures. With and without
the type library cache patch, AutoCAD loaded built-in `oleaut32.dll`
and .NET `10.0.0`, reached `activex_open_enter`, and terminated with
`0xc0000409` at `coreclr.dll+0x14922d` (launcher status 9). Neither
run returned from Open. The input fixture remained unchanged. The
Open crash therefore reproduces with either of the two suspected patches
omitted individually. This comparison did not exercise Save As.

#### Wine 11.22 with the AutoCAD startup and profile fixes

The [third comparison build][dwg-review-startup-b] keeps the two URLMON
patches for AutoCAD startup and MSXML3 PR #4 for the profile. This removes
the remainder of the winum series from the Wine 11.22 runtime. Use
`scripts/prepare-autocad-dwg-ab.sh startup-patches` with the original
test directory and a new empty directory. Run the same `open-activex`
pair with the same fixture and .NET version. This comparison selects
the full comparison runtime for its candidate copy. The two DLL-only
comparisons above continue to select the reference runtime with a DLL
substitution.

On 2026-11-20, the maintainer verified both archive checksums and the
candidate patch manifest. AutoCAD 2027 reached a blank drawing and then
`activex_open_enter` with both the full series (`aeb7a59`) and the three
retained patches (`c5ddcfe`). Both runs used .NET `10.0.0` and the same
2018 DWG fixture. Both ended with launcher status 9 and exception
`0xc0000409` at `coreclr.dll+0x14922d` before `activex_open_return`.
The input DWG stayed unchanged in both copies.

The patches omitted from `c5ddcfe` are unnecessary to reproduce this
desktop Open failure in the copied prefix, including the COM stub
activation context and type library cache. The three retained patches,
the copied prefix state, and Wine 11.22 behavior remain in scope. The
[.NET 10.0.15 Save As comparison](#compare-with-the-installed-system-wine)
later reproduced the Save As failure on the same three-patch build. The
best remaining hypothesis is a compatibility difference in the desktop
document lifecycle. The terminal CoreCLR offset identifies where the
failure was reported. Its trigger remains unknown.

This closes the three patch comparisons. For a newer Wine comparison,
use `c5ddcfe` as the 11.22 control. Keep the three patches, the source
prefix, the fixture and the .NET version matched within each pair. Start
with `save-events` on .NET 10.0.15 and check for save completion and a
usable drawing. Repeat `open-activex` separately with the same 2018 DWG
and .NET 10.0.0 as the earlier Open comparison. Resume individual patch
work if a trace identifies a specific API difference.

[dwg-review-series]: https://github.com/Harvi654/winum/blob/aeb7a59/patches/series
[dwg-review-history]: https://github.com/Harvi654/Autodesk-AutoCAD-on-Linux/blob/135b273/docs/autocad-experiment-2026-11.md#2026-11-17-quick-try-on-the-desktop-pc-installer-v040-winum-1122-1
[dwg-review-combase]: ../../patches/combase-stub-activation-context.md
[dwg-review-typelib]: ../../patches/oleaut32-typelib-keep-released.md
[dwg-review-a]: https://github.com/Harvi654/winum/actions/runs/44528762825
[dwg-review-b]: https://github.com/Harvi654/winum/actions/runs/44739990758
[dwg-review-cache-b]: https://github.com/Harvi654/winum/actions/runs/44749216299
[dwg-review-startup-b]: https://github.com/Harvi654/winum/actions/runs/44757013362

### Capture the fail-fast reason

Wine's x86_64 fast-fail dispatch preserves the numeric reason in the
first exception parameter. The terminal SEH message omits it. The
observer reads Win32 debug events to recover that parameter and the
preceding exception codes on the failing thread. For CLR exception
events it also reports the first numeric argument. At a last-chance
fast-fail event it reads the paused thread context and uses `StackWalk64`
for up to 12 frames. The output includes module names and relative
offsets. `observer_stack_next_return=` reports the return address the
walker provided for its last frame, when available. This address is
separate from the successfully walked frames. For addresses outside
dbghelp's known modules, `VirtualQueryEx` reports the mapping type and
whether the page permits execution. File-backed regions can also report
a basename and an allocation-relative offset. Generated managed code
may be in executable private or mapped memory. Debug information lookup
uses a local search path. Managed JIT frames may stop the walk. Attaching
sets the process's debugger flag, so compare the resulting fault module
and offset with the previous run.

The official Windows x64 .NET 10.0.0 and 10.0.15 `coreclr.dll` files
both invoke [`__fastfail(2)`][autocad-fastfail] at the observed offsets
(`0x14922d` and `0x1855fd`). The debug event confirmed reason 2 on .NET
10.0.0. The offsets identify the termination routine. The preceding
frames are needed to distinguish the caller. For example, an
[independent .NET 6 report][autocad-fastfail-similar] shows the same
reason with `CrawlFrame::SetCurGSCookie` in the stack, but the AutoCAD
stack has yet to identify that path.

On 2026-11-20, the maintainer repeated `save-0` on winum-11.22-1 with
.NET 10.0.0. The fatal thread had one earlier CLR exception (`0xe0434352`)
and stopped with reason 2. The observer walked `coreclr+0x14922d`
and `coreclr+0x15456b`. In the official Windows x64 DLL, the second
address immediately follows a call to the termination routine from the
helper at `coreclr+0x154560`. That helper passes zero to the routine,
matching [`DoJITFailFast`][autocad-fastfail-helper] on Windows x64.
The DLL has two direct calls to the helper, ending at `0xee09b` and
`0x156e10`. The first follows checks against the process's GS cookie.
One more return address can distinguish those paths or identify an
indirect caller. The two reported frames do not identify the caller of
`DoJITFailFast`.

The repeat with the return-address reporter walked the same two frames
and reported `observer_stack_next_return=2 module=unresolved`. It also
observed one earlier CLR exception on the fatal thread. The address
looked up by dbghelp was outside its known modules or module lookup
failed. Query its memory mapping to distinguish a loaded image,
executable memory without a known module, and a nonexecutable or
unmapped address before choosing a follow-up trace.

The next `save-0` trial reported `memory=mapped executable=1` for
that return address with allocation offset `0x5a467`. No mapped
filename was reported. CoreCLR can use file mappings for generated
code on Windows, so look up the address in the JIT method-load events
of the same process. The mapping alone leaves the caller unidentified.

### Identify the return address through EventPipe

Use the stack observer and EventPipe in one `save-0` run. Set both
`ACAD_EVENTPIPE=1` and `ACAD_EVENTPIPE_METHODS=1` and save the launcher
summary locally. The trace script enables exception, JIT and loader
events. The observer prints `observer_return_pc=` in this mode. The
summary script selects the specific EventPipe run and process, then
compares the byte before that return address against live method ranges.
The summary emits a method name only for a single match and omits source
paths, signatures and raw code addresses. A truncated trace can omit
method loads. A zero-match result leaves the caller unresolved.

With the packaged observer and summary reader, run the updated
`trace-autocad-dwg.sh baseline save-0` and pass the saved summary to
`summarize-autocad-eventpipe.sh --from-summary`. Return only
`eventpipe_parse_status=`, `eventpipe_method_events=`,
`eventpipe_return_method=` and the existing stack summary lines.

On 2026-11-20, the combined observer and EventPipe trial recorded 12
native CoreCLR frames before the observer's frame limit. The first caller
of the termination routine was `coreclr+0xf040c`. Disassembly of the
official Windows x64 .NET 10.0.0 DLL identifies this as the return from
the failure branch of `CrawlFrame::CheckGSCookies`. The native stack also
includes `coreclr+0x1fdcce`, whose call to the stack walker passes `0x511`.
The flags and the surrounding instructions match
[`ep_rt_walk_managed_stack_for_thread`][autocad-eventpipe-stackwalk].
EventPipe's own stack collection therefore took this path to fail-fast.
The first-chance CLR exception happened on the same thread in this trial.

The reader recovered 27,220 method-load events before a format error.
The selected return address was `coreclr+0x32a403`, which belongs to the
native CoreCLR DLL. That address yielded zero matches in the managed
method ranges and differs from the executable mapping in the earlier
observer-only trial.

The maintainer repeated the trial with `DOTNET_EventPipeEnableStackwalk=0`.
This disables EventPipe stack collection while retaining exception and
method events in .NET 10.0.0. A synthetic run under winum-11.22-1 still
matched its known managed return address with that setting.

The AutoCAD run again recorded first-chance `0xe0434352` on the fatal
thread followed by `0xc0000409` with reason 2. The stack matched the
observer-only run, with `coreclr+0x14922d` and `coreclr+0x15456b`
followed by an executable mapping without a resolved module. Its
allocation offset was `0x9c087`. The partial recording contained
27,425 method loads and 781 exception events before the format error.
The return address matched none of the recovered method ranges. The
reader recovered zero exception events for the fatal thread, while
the debugger independently confirmed the first-chance CLR exception
on that thread. The trace therefore leaves both the managed caller and
the exception type unresolved.

This closes the EventPipe comparison. The observer-only and
no-stack EventPipe runs reproduce the same signature on .NET 10.0.0.
The remaining hypothesis is a difference in exception handling or
stack state during the desktop document operation. There is no specific
failed Wine API identified for a relay trace. Resume with the
[matched Wine version comparison](#wine-1122-with-the-autocad-startup-and-profile-fixes)
or a new reproducible API difference. Wine 11.23 had no public release
tag or WineHQ source archive when checked on 2026-11-20.

[autocad-eventpipe-stackwalk]: https://github.com/dotnet/runtime/blob/v10.0.0/src/coreclr/vm/eventing/eventpipe/ep-rt-coreclr.cpp#L69-L88

### Build and run the fail-fast observer

Build with `scripts/build-measure-bundle.sh` and the
`docs/wine-repro/shell32-autocomplete-timing/failfast.list` manifest.
With no arguments, each executable observes a synthetic child. The
x86_64 self-test under winum-11.22-1 reported reason 7 and six frames.
A separate attach test returned reason 2, the synthetic CLR exception
from the fatal thread, and five frames. In a local build limited to two
frames, the observer reported the third frame's address as the last
frame's return address. In the offline Windows VM, run
the two self-tests with `run-measure.bat`. For AutoCAD, start with Save
As because the earlier `save-0` trial observed a CLR exception on the
fatal thread. Use the x86_64 observer with the existing trace script:

~~~bash
export ACAD_FAILFAST_OBSERVER="$ACAD_OBSERVER_BUNDLE/autocad-failfast-observer-x86_64.exe"
bash scripts/trace-autocad-dwg.sh baseline save-0
~~~

Wait for AutoCAD to reach the start screen before pressing Enter in
the terminal. After `observer_ready=1`, set `FILEDIA` to 0, start a blank
drawing, add one line and run `SAVEAS` with a new scratch DWG name.
Return the `observer_stack_`, `last_chance=`, `fast_fail_reason=` and
`preceding_exception=` summaries. The observer detaches after two
minutes if the process is still running. Use a separate, filtered
Wine relay trial if the stack points to a particular exported Wine API
whose call and return need verification.

[autocad-fastfail]: https://learn.microsoft.com/en-us/cpp/intrinsics/fastfail
[autocad-fastfail-helper]: https://github.com/dotnet/runtime/blob/v10.0.0/src/coreclr/vm/jithelpers.cpp#L982-L1016
[autocad-fastfail-similar]: https://github.com/dotnet/runtime/issues/89309

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
