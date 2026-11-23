# Direct WMI method calls through IDispatch

Inventor Nastran 2027 registration reaches StdRegProv through the WMI
automation moniker. Wine 11.22 traces show the direct method invocation
reaching wbemdisp with flag 0x1 and the script reporting an unsupported
action. The probes here are original and use synthetic registry data.
The problem was observed externally. See the [Nastran result
log](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/fef50b4/docs/arch-hyprland.md#result-log)
and [Wine bug 39463](https://bugs.winehq.org/show_bug.cgi?id=39463).

## Measure

| Probe | Observations |
| --- | --- |
| C automation moniker | Binding and name-lookup HRESULTs, then Invoke HRESULT, returned VARIANT, and by-reference values with each flag combination. |
| Direct C methods | CreateKey, SetStringValue, GetStringValue, EnumKey, SetDWORDValue and DeleteKey. Reads cover a typed BSTR reference, a VARIANT reference and a missing key. |
| C argument counts | CreateKey without arguments, GetStringValue without its output argument and GetStringValue with an extra argument. |
| Direct VBScript | Statement-style CreateKey and function-style calls for the same methods, reporting each script error, result and output. |
| VBScript runtime error | An unhandled intentional error after a marker. The C wrapper records the cscript process exit code independently of the direct-call script. |

Each architecture uses a unique temporary key under
`HKCU\Software\winum-test`. The C probe removes the unique key and removes
the parent only when it created the parent. No administrator rights are
needed. Use the offline Windows 10 snapshot on a 64-bit installation so
both x86_64 and i686 executables can run.

Build with the x86_64 and i686 MinGW toolchains on PATH, from the repository
root.

~~~sh
probe=docs/wine-repro/wbemdisp-method-invoke
bash scripts/build-measure-bundle.sh out "$probe/measure.list"
cp "$probe"/*.vbs out/
(cd out && sha256sum ./*.exe ./*.vbs ./README.txt ./measure.manifest ./run-measure.bat > SHA256SUMS)
~~~

Download the workflow artifact `wbemdisp-method-invoke-measure`, verify its
`SHA256SUMS` before transferring the bundle to the VM, and run
`run-measure.bat` in a fresh folder. Return
`result-wbemdisp-method-invoke-x86_64.txt`,
`result-wbemdisp-method-invoke-i686.txt`, and `exitcodes.txt` only.
The cscript diagnostic can include the local folder name. The lines
beginning with `cscript kind=` already record the process status.

## Comparison

| Runtime | x86_64 | i686 |
| --- | --- | --- |
| Offline Windows 10 | [Text](results/windows-10/result-x86_64.txt) | [Text](results/windows-10/result-i686.txt) |
| Vanilla Wine 11.22 | CI | CI |
| Winum 11.22-2 | [Text](results/winum-11.22-2/result-x86_64.txt) | [Text](results/winum-11.22-2/result-i686.txt) |

The Windows results supply the expectations for any Wine test. Record
cscript behaviour separately from wbemdisp, including on the deliberate
runtime error.

### Wine baseline (probe v1)

The C columns count the Invoke HRESULTs across the 13 calls per flag.
The cscript columns are the process exit codes for the two scripts.

| Runtime | Architecture | METHOD | METHOD + PROPERTYGET | Direct script | Intentional error |
| --- | --- | --- | --- | --- | --- |
| winum-11.22-2 | x86_64 | `0x80004001` × 13 | `0x80041002` × 13 | 0 | 0 |
| winum-11.22-2 | i686 | `0x80004001` × 13 | `0x80041002` × 13 | 0 | 0 |

[CI run](https://github.com/Harvi654/winum/actions/runs/45257510865).

### Offline Windows 10 reference (probe v1)

Both architectures produced the same observations and both probes exited
zero. The table covers both dispatch flag combinations. Out arguments
retained their sentinels on Winum 11.22-2.

| Case | Windows 10 Invoke HRESULT | Windows 10 result and out value |
| --- | --- | --- |
| CreateKey and DeleteKey, including leaf | S_OK | VT_I4 0 |
| SetStringValue and SetDWORDValue | S_OK | VT_I4 0 |
| GetStringValue, by-reference VARIANT or BSTR | S_OK | VT_I4 0, out VT_BSTR containing winum-value |
| EnumKey | S_OK | VT_I4 0, out one-element VT_ARRAY with VT_VARIANT element VT_BSTR leaf |
| GetStringValue on a missing key | S_OK | VT_I4 2, out VT_NULL |
| CreateKey with no arguments | DISP_E_EXCEPTION | Empty result, EXCEPINFO.scode 0x80041008 |
| GetStringValue without out argument, value now DWORD | S_OK | VT_I4 0x80041005 |
| GetStringValue with one extra argument, value now DWORD | S_OK | VT_I4 0x80041005, out VT_NULL |

All the direct VBScript methods completed on Windows. Winum returned
0x1bd for the statement-style call and 0x80041002 for the function-style
calls. Both Windows and Winum returned process exit status zero for the
deliberate unhandled VBScript error. This measurement does not justify a
wscript candidate.

### Round 2

Probe v1 wrote a DWORD over the test string before the two calls with an
altered argument count. Its results therefore combine an argument-count
change and a registry type change. Probe v2 adds the two calls while the
value is still a string. Their case labels are `read_no_out_string` and
`read_extra_string`. Keep the v1 observations for comparison. The
initial Wine test covers the v1 sequence. Its native L1 is pending.

The v2 Winum run still rejects the two added cases on both architectures.
The return values for the method flags are unchanged from v1.
[Winum x86_64](results/round-2/winum-11.22-2/result-x86_64.txt) ·
[Winum i686](results/round-2/winum-11.22-2/result-i686.txt) ·
[Round 2 CI](https://github.com/Harvi654/winum/actions/runs/45258932265).
