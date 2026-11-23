# Per-user in-process COM classes

Non-elevated Windows 10 uses the per-user in-process registration ahead
of the machine registration for the same CLSID. The candidate applies
that lookup to InprocServer32, ProgID and TreatAs reads in combase.
Direct registry calls retain the existing Wine HKCR implementation.

| Level | Status |
| --- | --- |
| L0 | Candidate build pending. |
| L1 | 54 successful native checks on x86_64 and 59 on i686. [Results](../../../docs/wine-repro/combase-per-user-classes/results/l1-windows-10/) |
| L2 | Paired COM suites pending. |
| L3 | Candidate measurement pending. |
| L4 | Pending joint Nastran run with wbemdisp and msado15 and the Inventor application smoke test. |
| Cross review | Claude review pending. |

[Native and Winum measurement](../../../docs/wine-repro/combase-per-user-classes/README.md).
