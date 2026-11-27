# riched20-textrange-expand-paragraph

<!-- winum-test: l1 -->

Implement TOM paragraph navigation for Expand, StartOf, EndOf, Move, MoveStart and MoveEnd.

Promoted from the reviewed [v3 candidate](https://github.com/Harvi654/winum/blob/5ad91f4/patches/candidates/riched20-textrange-expand-paragraph/README.md). The production patch contains the strict Windows-validated regression test.

| Level | Result |
| --- | --- |
| L0 on 11.23 | All 84 patches apply in series order with `--fuzz=0`; build pending. |
| L1 | Offline Windows 10, 2026-11-26: 1,847 checks per architecture. Both passed. |
| L2 | [Candidate pair on 11.22](https://github.com/Harvi654/winum/actions/runs/45871395054) passed; 11.23 pair pending. |
| L4 | [Combined 11.22 overlay](https://github.com/Harvi654/winum/actions/runs/45880891853) passed Inventor and AutoCAD text editing and Revit New Project; 11.23 smoke pending. |
| Cross review | Approved on the candidate branch. |
