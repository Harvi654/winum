# Per-user in-process COM classes

Non-elevated Windows 10 uses the per-user in-process registration ahead
of the machine registration for the same CLSID. The candidate applies
that lookup to InprocServer32, ProgID and TreatAs reads in combase.
Direct registry calls retain the existing Wine HKCR implementation.

| Level | Status |
| --- | --- |
| L0 | Both DLL architectures compiled after zero-fuzz apply in [round 1](https://github.com/Harvi654/winum/actions/runs/45278159284). |
| L1 | 54 successful native checks on x86_64 and 59 on i686. [Results](../../../docs/wine-repro/combase-per-user-classes/results/l1-windows-10/) |
| L2 | Paired COM suites pending. |
| L3 | Candidate measurement pending. |
| L4 | Pending joint Nastran run with wbemdisp and msado15 and the Inventor application smoke test. |
| Cross review | Claude review pending. |

[Native and Winum measurement](../../../docs/wine-repro/combase-per-user-classes/README.md).

## WOW64 registry prerequisite

Round 1 found that a fresh Wine prefix can place an explicit 32-bit user
registration in the 64-bit path because its HKCU Classes WOW64 tree is absent.
The x86_64 runner then selected the wrong registration. Round 2 prepares an empty
WOW64 Classes subtree in the isolated CI prefixes to separate COM lookup from
registry key placement. Direct HKCR reads and creation or separation of user
registry views on an existing user prefix are outside this combase candidate.
