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
