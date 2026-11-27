# riched20-target-device-scroll

<!-- winum-test: l1 -->

Keep the horizontal scroll range large enough for the target device. On Inventor the letter may still lie just past the right edge, where a small scroll reveals it.

Promoted from the reviewed [reviewed candidate candidate](https://github.com/Harvi654/winum/blob/04717ba/patches/candidates/riched20-target-device-scroll/README.md). The production patch contains the strict Windows-validated regression test.

| Level | Result |
| --- | --- |
| L0 on 11.23 | All 84 patches apply in series order with `--fuzz=0`; build pending. |
| L1 | Offline Windows 10, 2026-11-26: 83 checks per architecture. Both passed. |
| L2 | [Candidate pair on 11.22](https://github.com/Harvi654/winum/actions/runs/45865705690) passed; 11.23 pair pending. |
| L4 | [Combined 11.22 overlay](https://github.com/Harvi654/winum/actions/runs/45880891853) passed Inventor and AutoCAD text editing and Revit New Project; 11.23 smoke pending. |
| Cross review | Approved on the candidate branch. |
