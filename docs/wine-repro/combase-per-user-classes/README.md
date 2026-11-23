# COM activation with per-user classes

The 2026-11-23 joint Nastran 2027 trial reached add-in registration and
loaded NEiInventor.dll with the wbemdisp and msado15 candidates. The
remaining UI startup failure was 0x80040154 for an in-process class
registered under HKCU\Software\Classes. Measure the COM lookup behavior
with independent test servers before choosing the combase behavior.
[Joint L4 checkpoint](https://github.com/Harvi654/winum/blob/95d9eee/patches/candidates/msado15-stream-charset/README.md#nastran-2027-joint-l4) ·
[Wine bug 17019](https://bugs.winehq.org/show_bug.cgi?id=17019).

## Cases

| Case | Registration and observations |
| --- | --- |
| User only | HKCU Software\Classes\CLSID and InprocServer32. CoGetClassObject, IClassFactory::CreateInstance and CoCreateInstance. |
| ProgID and TreatAs | Synthetic per-user ProgID in both lookup directions and a separate synthetic per-user TreatAs alias to the class. |
| Both hives | The same fresh CLSID points at two different servers in HKCU and HKLM. The returned marker identifies the selected server. |
| WOW64 | Fresh user classes registered through the default view, explicit 32-bit and 64-bit views, and a literal Wow6432Node\CLSID path. The 32-bit cases use the 32-bit server. |
| Registry control | RegOpenKeyEx on HKCR\CLSID in three views, followed by a read of the InprocServer32 default value. The report identifies the selected server without printing its path. |

The synthetic in-process servers implement IClassFactory and IPersist.
The markers are 132 and 164 for the user servers and 232 and 264 for
the machine servers. These numbers are labels for hive and server
architecture. Each process uses fresh generated CLSIDs and a
synthetic ProgID. Registrations are volatile and their test entries
are removed at the end. The elevated helper also removes its class
after its wait expires if the measuring process stops early.

## Offline Windows 10

Extract **combase-per-user-classes-measure** to a fresh folder. Verify
**SHA256SUMS** before transferring it to the offline VM. Use a normal
Windows desktop and run **run-measure.bat** from Explorer. Accept the
UAC prompt for each architecture using the same Windows account. The
probe keeps the caller token and uses the elevated helper only for the
temporary HKLM registration. The report records token elevation and
whether the conflict was measured. Return the two
result-combase-per-user-classes files and exitcodes.txt, including any
reported setup status. A skipped conflict leaves precedence open.

Build locally from the repository root with both MinGW toolchains on PATH.

~~~sh
bash docs/wine-repro/combase-per-user-classes/build-bundle.sh out
~~~

## Validation

| Runtime | x86_64 | i686 |
| --- | --- | --- |
| Offline Windows 10 | [Text](results/windows-10/result-x86_64.txt) | [Text](results/windows-10/result-i686.txt) |
| Vanilla Wine 11.22 | CI | CI |
| Winum 11.22-2 | [Text](results/winum-11.22-2/result-x86_64.txt) | [Text](results/winum-11.22-2/result-i686.txt) |

Wine CI uses direct setup of the temporary machine class in an isolated
prefix. Native Windows measurements supply the expectations for the
future Wine test. HKCR observations are a control for this combase-only
candidate.

Joint L4 requires a fresh Inventor 2027.2 prefix copy with all three
candidates. Record the 0x80040154 dialog, the separate functionality
message, and Nastran mesh, solve and results. The Inventor smoke test
covers start, part, assembly, drawing, save and reopen, and restart.


### Winum 11.22-2 baseline

Both architectures returned 0x80040154 for CoGetClassObject and
CoCreateInstance in the user-only cases. CLSIDFromProgID returned
0x800401f3, ProgIDFromCLSID returned 0x80040154, and CoGetTreatAsClass
returned S_FALSE. The same-CLSID conflict activated the machine server
with marker 264 on x86_64 and 232 on i686. In these synthetic cases
the raw HKCR lookup saw the machine registration in the matching view.
Both cleanup checks passed. [CI](https://github.com/Harvi654/winum/actions/runs/45275509058).


### Offline Windows 10 reference

Both activation processes ran with a non-elevated token. The elevated
helpers set up the HKLM side of each conflict and reported successful
cleanup.

| Case | x86_64 | i686 under WOW64 |
| --- | --- | --- |
| User-only CLSID in the default view | CoGetClassObject and CoCreateInstance select user marker 164 | Both select user marker 132 |
| User-only explicit 32-bit view | 0x80040154 | User marker 132 |
| User-only explicit 64-bit view | User marker 164 | 0x80040154 |
| User-only literal Wow6432Node | 0x80040154 | User marker 132 |
| Same CLSID in user and machine hives | User marker 164 | User marker 132 |
| CLSIDFromProgID, ProgIDFromCLSID, CoGetTreatAsClass | S_OK, all matches | S_OK, all matches |

HKCR opened the matching registration in the same view and returned
its default InprocServer32 value. In the conflict it reported the user
marker. A lookup in the opposite view returned ERROR_FILE_NOT_FOUND
for the classes created solely in the default view. HKCR is an
observation here and has no fix expectation in the COM unit.
