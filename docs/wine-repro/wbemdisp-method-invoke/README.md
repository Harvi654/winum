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
| Offline Windows 10 | Pending | Pending |
| Vanilla Wine 11.22 | CI | CI |
| Winum 11.22-2 | [Text](results/winum-11.22-2/result-x86_64.txt) | [Text](results/winum-11.22-2/result-i686.txt) |

The Windows results supply the expectations for any Wine test. Record
cscript behaviour separately from wbemdisp, including on the deliberate
runtime error.

### Wine baseline

The C columns count the Invoke HRESULTs across the 13 calls per flag.
The cscript columns are the process exit codes for the two scripts.

| Runtime | Architecture | METHOD | METHOD + PROPERTYGET | Direct script | Intentional error |
| --- | --- | --- | --- | --- | --- |
| winum-11.22-2 | x86_64 | `0x80004001` × 13 | `0x80041002` × 13 | 0 | 0 |
| winum-11.22-2 | i686 | `0x80004001` × 13 | `0x80041002` × 13 | 0 | 0 |

[CI run](https://github.com/Harvi654/winum/actions/runs/45257510865).
