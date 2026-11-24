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

## Offline Windows 10 results

Both architectures ran with a non-elevated token and an elevated setup
helper. Each completed all 13 cases, exited zero, and reported zero user
and machine cleanup errors.

| Read in the matching view | User only | Machine only | Both | Absent |
| --- | --- | --- | --- | --- |
| Open from HKCR or its CLSID handle | Success | Success | Success | ERROR_FILE_NOT_FOUND |
| Default and Shared values | USER | MACHINE | USER | Missing |
| UserOnly value | USER | Missing | USER | Missing |
| MachineOnly value | Missing | MACHINE | MACHINE | Missing |

RegQueryValueExW showed the same value results after ANSI, Unicode and
relative opens. RegGetValueW also returned the machine-only value in the
collision. RegQueryValueW returned the user default in the collision.
A class registered in a single view only opened in that view. A literal
Wow6432Node registration matched the explicit 32-bit view in the probe.
The default view tracked process bitness on the 64-bit host.

Default-view enumeration saw the probe class when either hive contained it.
For the default-view user-only and both-hive cases the write marker landed
in HKCU. The absent write returned access denied. Other writes varied by
view and architecture. These are observations for separate work.

On Winum 11.22-2 the user-only open returned ERROR_FILE_NOT_FOUND both
directly and relative to the CLSID handle. A collision yielded the machine
value. Returning only a physical user handle would lose the measured
machine-value fallback. Relative HKCR opens also need to keep their
provenance, since an explicit HKLM handle must remain a machine handle.

## Exact Wine test (L1)

Verify the checksums in the `kernelbase-hkcr-read-l1` artifact before
copying it into the offline Windows 10 VM. Run `run-tests.bat` from the
normal desktop and accept the two prompts for the setup helpers. The
test processes stay non-elevated. The runner extracts the exact new
tests from the Wine test patch and uses the measured helper to hold a
both-hive class while they run.

Return `exitcodes.txt`, `result-hkcr-class-reads-x86_64.txt` and
`result-hkcr-class-reads-i686.txt`. A native pass has zero exit codes,
failures, todos, skips and cleanup errors.

The maintainer's offline native run completed 57 checks on each of
x86_64 and i686, with zero failures, todos, skips or cleanup errors.
Both processes ran without elevation and used the setup helper.
[Verified L1 bundle](https://github.com/Harvi654/winum/actions/runs/45294088865/artifacts/14874679050).

## High integrity measurement

In the same offline Windows 10 snapshot and account as the normal run,
open Command Prompt with **Run as administrator**, then run
`run-measure-high.bat`. It runs both architectures in that console
and requires `token_integrity=0x3000 elevated=1` before setting up the
temporary keys. Return `result-hkcr-read-high-x86_64.txt`,
`result-hkcr-read-high-i686.txt` and `exitcodes-high.txt`.

On 2026-11-24, x86_64 and i686 both reported RID 0x3000, completed
all 13 cases, exited 0 and reported zero cleanup errors. Against the
non-elevated Windows run, each open, query and enumeration result matched.
This includes HKCU-only opens directly and relative to the HKCR CLSID
handle, user precedence and machine-only value fallback in collisions,
and the explicit WOW64 views. Elevated HKCR still reads HKCU classes.
[Comparison counts](results/windows-10-high/comparison.tsv).

The write probes did differ. For example, elevated writes to the
machine-only fixture succeeded in all four view cases, while the
corresponding non-elevated writes failed. Writing through HKCR is
outside this read candidate. The measured reads need no integrity gate.
