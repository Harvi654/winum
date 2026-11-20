# Ole32 API comparison

This pilot checks twelve named exports in the system `ole32.dll` and
measures four scenarios using the public Windows SDK contracts. The
program resolves exports by name. The probe records presence through
`GetProcAddress`, without parsing Microsoft DLL files or calling unknown
ordinals. An export can be present while its behavior differs.

| Scenario | Observations |
| --- | --- |
| `init_sequence` | First and repeated STA initialization, an MTA request in the STA, and MTA initialization after releasing all successful calls |
| `apartment` | Apartment type and qualifier before initialization, in each apartment, and after cleanup |
| `stream_io` | Creating an HGLOBAL-backed stream, writing four bytes, stat, short read and end-of-stream read |
| `stream_clone` | Cloning that stream and checking data and independent read positions |

The SDK documents [COM initialization](https://learn.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-coinitializeex),
[apartment queries](https://learn.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-cogetapartmenttype),
[HGLOBAL streams](https://learn.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-createstreamonhglobal)
and [stream cloning](https://learn.microsoft.com/en-us/windows/win32/api/objidl/nf-objidl-istream-clone).
The Windows VM run supplies the reference outcomes for the comparison.

## Build and run

Use `build-measure-bundle.sh` with this directory's list, or dispatch
the existing **Build MEASURE bundle** workflow with the same list path.

```bash
bash scripts/build-measure-bundle.sh out/ole32-api-measure \
    docs/wine-repro/ole32-api-measure/measure.list
```

1. Verify `SHA256SUMS` in the bundle. Retain the same two executables
   for every run. On the offline Windows reference VM, double-click
   `run-measure.bat`. The text results are `result-ole32-api-x86_64.txt`,
   `result-ole32-api-i686.txt` and `exitcodes.txt`.
2. Run each executable with vanilla Wine and with Winum built from the
   same upstream version. Use separate test prefixes with comparable
   settings and builtin Ole32. For example, with already prepared
   runtime locations and prefixes, run the following for x86_64 and
   repeat with `arch=i686`.

   ```bash
   arch=x86_64
   bundle="$PWD/out/ole32-api-measure"
   mkdir -p results
   WINEPREFIX="$VANILLA_PREFIX" "$VANILLA_WINE" "$bundle/ole32-api-$arch.exe" \
       > "results/vanilla-$arch.txt"
   WINEPREFIX="$WINUM_PREFIX" "$WINUM_WINE" "$bundle/ole32-api-$arch.exe" \
       > "results/winum-$arch.txt"
   ```

3. Keep the executable checksums and the two Wine build identifiers
   with the results. Put the matching Windows result in `results/` and
   compare one architecture at a time.

   ```bash
   python3 -I scripts/compare-api-measure.py \
       "results/result-ole32-api-$arch.txt" \
       "results/vanilla-$arch.txt" "results/winum-$arch.txt"
   ```

Use `--wine-spec` and `--winum-spec` to annotate the measured exports
with `declared_stub` and `forwarded` from each build's own `ole32.spec`.
Those annotations describe the source declarations. The presence check
and scenario results describe the runtime. The comparator prints export
differences and differing or incomplete scenario steps. It rejects a
missing completion marker or mixed architectures. The OS build in the
probe header is the version reported to the process. Wine reports its
configured Windows version in that field.

This synthetic pilot tests Ole32 independently of AutoCAD. Agreement
establishes coverage for the listed cases. Relating a discrepancy to
the desktop Save As failure requires a separate product test.
