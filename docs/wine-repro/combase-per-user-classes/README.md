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
| Winum 11.22-2, combase candidate with prepared WOW64 subtree | [Text](results/candidate/result-x86_64.txt) | [Text](results/candidate/result-i686.txt) |

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

### Wine test on native Windows (L1)

The standalone Wine unit includes its own architecture-matched COM
fixture and machine registration helper. It tests activation, view
isolation, per-user precedence over HKLM, ProgID and TreatAs. Run
`run-tests.bat` on offline Windows in a fresh folder from the normal
desktop and accept UAC for the two helpers. Return both
`result-combase-per-user-classes-*.txt` files and `exitcodes.txt`.
The Wine runtime candidate follows a passing native unit.
[L1 artifact](https://github.com/Harvi654/winum/actions/runs/45277142410/artifacts/14867891933).

The native unit passed with 54 checks on x86_64 and 59 on i686. Both
exited zero with zero failures, todos or skips.
[Offline L1 results](results/l1-windows-10/).

### Candidate round 1 on a fresh Wine prefix

The patched Winum matches the tested native COM outcomes on i686 and
selects the HKCU registration in both architectures' conflict cases.
The x86_64 exception is the explicit 32-bit registration. Trying its
opposite-bitness server produces 0x80070005 rather than 0x80040154.
In the unit, the same-bitness control server registered through that
opposite view is erroneously activated. Both failures are consistent
with Wine writing the 32-bit user class into the default x86_64 view
when the HKCU Classes WOW64 subtree has yet to be created.
[Round 1 results](results/candidate-round-1/) ·
[CI](https://github.com/Harvi654/winum/actions/runs/45278159284).

A separate registry control with the Winum 11.22-2 runtime confirmed
the key placement. It used `reg add /reg:32` for a fresh synthetic CLSID,
then checked the two physical paths with `reg query /reg:64`.

| Wine prefix | `Classes\CLSID` | `Classes\Wow6432Node\CLSID` |
| --- | --- | --- |
| Fresh | Found | Missing |
| After preparing the empty WOW64 subtree | Missing | Found |

### Candidate round 2 with the WOW64 test subtree

CI creates the empty `HKCU\Software\Classes\Wow6432Node\CLSID` subtree in
fresh Wine prefixes before executing the unit and MEASURE. Both
architectures then match every native COM result in the reference table.
The focused Wine unit passed 51 checks on x86_64 and 56 on i686 with
zero failures or skips. It sets up the temporary machine class directly
and has three fewer assertions per architecture than the UAC-helper run.
Plain HKCR remains a separate observation.
[Round 2 results](results/candidate/) ·
[CI](https://github.com/Harvi654/winum/actions/runs/45278822590).

## Medium and High integrity

With the updated bundle, run the native COM fixture
from a normal Command Prompt and an elevated one under the same VM
account. Use the matching redirection suffix in each console.

~~~bat
combase-per-user-classes-x86_64.exe --elevation-only > result-combase-medium-x86_64.txt
combase-per-user-classes-i686.exe --elevation-only > result-combase-medium-i686.txt
~~~

Repeat in the elevated console with high in the filenames. Return all
four result files. The token_integrity lines should identify the actual
Medium (0x2000) and High (0x3000) tokens. The probe records activation
and closes the temporary registration without changing the candidate.

### v2 native Wine test with normal and High tokens

The expanded L1 bundle ran on offline Windows 10 on 2026-11-24.
The normal run passed 54 checks on x86_64 and 59 on i686. The elevated
run confirmed High integrity (0x3000) and passed 12 checks on each
architecture, including the expected REGDB_E_CLASSNOTREG from both
CoGetClassObject and CoCreateInstance for the HKCU-only class. All four
runs exited 0 with no failures, todos, flaky checks or skips.
[Native v2 summary](results/l1-windows-10-v2/summary.tsv).
