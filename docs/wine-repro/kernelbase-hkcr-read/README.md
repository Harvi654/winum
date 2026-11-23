# HKCR read behavior with per-user COM registration

Nastran's NEWPFUI class is registered under HKCU Software\Classes. With the
combase candidate the application reaches mscoree, which opens
HKCR\CLSID\<class>\InprocServer32 and then tries HKLM Software\Classes.
Both opens fail under Wine and no NEWPFUI bind is attempted. Windows
tries the assembly name and then loads the CodeBase. The independent
SharedViews and InteractiveTutorial bind failures also occur on Windows.
The trace observed the native .NET component externally.

This probe uses synthetic registry keys and data. It does not load a COM
server or read Autodesk data. Results from offline Windows 10 will define
the expected behavior. No Wine implementation changes are made by this
measurement. [Wine bug 17019](https://bugs.winehq.org/show_bug.cgi?id=17019).

## Cases

A fresh random CLSID identifies each case. Its synthetic InprocServer32
key may exist in HKCU, HKLM, both or neither. Registered keys have distinct
default and Shared strings, and each hive adds its own exclusive value.
This allows the reader to distinguish hive precedence from value merging.

| Case | Measurement |
| --- | --- |
| HKCU only, HKLM only, both | Register a separate class in each of the default, explicit 32-bit, explicit 64-bit and literal Wow6432Node paths. |
| Absent | Use a fresh class without a registration. |
| Open | RegOpenKeyExW and RegOpenKeyExA from HKCR. Open CLSID from HKCR and perform a relative RegOpenKeyExW from the resulting handle. Read the four fixture values with RegQueryValueExW. |
| Other reads | RegGetValueW for all four fixture values and all supported view flags. RegQueryValueW for the default value using the default view. |
| Enumeration | RegEnumKeyExW records whether the probe's own CLSID is seen in the default view. Other registry names are omitted. |
| Writes | RegCreateKeyExW and RegSetValueExW through HKCR on the random class, then probe the two physical hives for the write marker. The existing fixture tests use their registration view. The absent case uses the default view. |

Both bitnesses are built. WOW64 variants run on 64-bit Windows.
The ordinary process remains non-elevated. It asks once for an elevated
helper per architecture to create machine fixtures and clean them up.
The helper checks that the GUIDs are unused, operates only on the
temporary fixture classes and removes them on completion or expiry.
The main process creates and cleans its own user fixtures. All fixture
trees are volatile. A nonzero exit or cleanup status invalidates the
affected run.

## Offline Windows 10

Download and extract the **kernelbase-hkcr-read-measure** artifact.
Check **SHA256SUMS** before copying the bundle into the offline VM. From
a non-elevated desktop, run **run-measure.bat**. Accept each prompt with
the same Windows account. Return only **exitcodes.txt**,
**result-hkcr-read-x86_64.txt** and **result-hkcr-read-i686.txt**. Those
reports contain API statuses, string markers and fixture coverage.
Expected values and tests come after the native measurement.

## Local build and Wine observation

~~~bash
bash docs/wine-repro/kernelbase-hkcr-read/build-bundle.sh out
(cd out && sha256sum -c SHA256SUMS)
~~~

Use the pinned LLVM MinGW toolchain from the workflow. In a disposable Wine
prefix use each executable with `--direct-machine`. This skips the UAC
helper and reports the token status explicitly, so the runs can be
distinguished from the normal Windows reference.

After the Windows run, add focused tests in advapi32's registry suite and
measure the smallest change to kernelbase's read/open paths. Treat
enumeration, writes, deletes and notifications as separate questions.
Before promotion, pair advapi32, kernelbase, ole32, combase and shell32.
Run Nastran and the Inventor smoke test, and AutoCAD if available.
Claude cross-review follows Windows measurement and the candidate change.
