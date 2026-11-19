# Shell autocomplete callback timing

AutoCAD 2027's Save As file-name edit can stop responding while the
application enumerates autocomplete suggestions. Wine 11.22's
`dlls/shell32/autocomplete.c` calls `IEnumString::Next` in the
edit window's message handler and gathers the full list. This
program observes the Windows implementation with its own edit
window, without launching AutoCAD or enumerating the filesystem.

Each case creates a fresh edit and initializes `IAutoComplete2`
with autosuggest and autoappend. The program sends one synthetic
character, pumps the UI, and summarizes callbacks. The source is
finite, reports the number of strings produced and returns
`S_FALSE` when it runs out.

- `empty`: empty control case.
- `fast-long`: 1,024 in-memory suggestions, no added delay.
- `slow-standard`: 128 suggestions, 12 ms added delay per returned
  item, standard COM marshaling for an STA object.
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
| Slow, standard marshaling | 123 / 122 | Call still active at sampling in both runs | 60 / 62 |
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

## Wine regression test

`patches/tests-first/shell32-tests-autocomplete-thread.patch` adds the
`shell32:autocomplete_thread` test to Wine 11.22. It uses the empty
source case and checks that autocomplete calls `Next` after input on a
thread other than the one owning the edit. It uses a time limit only
to stop waiting for a missing callback. It does not assert an input
duration or a suggestion limit.

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

### L1 result

Passed on 2026-11-19 in the maintainer's offline Windows 10 VM using the
[test bundle](https://github.com/Harvi654/winum/actions/runs/44554709236)
from `5d308e9`. Both x86_64 and i686 ran 10 checks with zero failures,
zero skips and process exit code 0. This covers the Wine test itself
as well as the earlier MEASURE result for the empty source.

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

On 2026-11-19 the maintainer reported that editing the filename worked
with the v2 candidate. AutoCAD still crashed on saving and when opening
an existing DWG.

Both Save As runs with FILEDIA set to 0 used the same MSXML3 build
(`aeb7a59`). Both exited with code 9 and raised unhandled exception
0xc0000409 with flags 0x1.

| Variant | Fault location | Output |
| --- | --- | --- |
| Autocomplete v2 | Initial trace lacked image ranges | DWG exists, 30,927 bytes. Reopening is unverified |
| Baseline | `coreclr.dll+0x14922d`, resolved from the loaded image range | Unchecked |

The baseline reproduces the Save As crash without the autocomplete
candidate. Whether both failures terminate at the same module offset
remains to be checked. The Open trial with FILEDIA set to 0 is pending.

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

The trace uses Wine SEH, DLL load and module mapping channels in
acad.exe. Raw logs stay in the private run directory. For an
unhandled exception, the summary reports the image and offset
when the fault address falls within a recorded loaded image range.
It also checks for the previously recorded fail-fast site in
D3DCompiler_47_cor3.dll at offset 0x11922d.
