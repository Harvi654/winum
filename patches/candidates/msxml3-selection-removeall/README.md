# MSXML selection removeAll

The Inventor Content Center ISO filter calls removeAll on an empty
selection after querying the saved category group. This candidate
uses the existing DOM detachment helper while the selection holds
references, then clears the snapshot and its cursor. It handles
empty selections and populated selections spanning multiple parents.

- [Issue history and validation](../../../docs/patches/msxml3-selection-removeall.md)
- [Native comparison](../../../docs/wine-repro/msxml3-selection-removeall/README.md#offline-windows-10-comparison)
- [Tests-first patch](../../tests-first/msxml3-tests-selection-removeall.patch)

removeNext is measured separately. Both native versions return
E_NOINTERFACE for ISupportErrorInfo. Neither API is changed here.


For the application check, verify SHA256SUMS from the
[v2 overlay](https://github.com/Harvi654/winum/actions/runs/45907358465/artifacts/15158681305)
and copy both msxml3.dll files to the matching architecture directories
under lib/wine in a test copy of winum-11.22-3. In the Content Center
editor, select ISO and check the list, change the filter again, restart
Inventor and check the persisted setting.
