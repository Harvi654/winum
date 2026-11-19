# Shell autocomplete timing on Windows 10

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

Windows results are pending. No runtime patch is included here.
