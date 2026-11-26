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


The maintainer verified that the v2 overlay fixes ISO filtering in
Inventor 2027 and that a changed filter survives restart. AutoCAD
and Revit smoke checks and maintainer cross review are open.
See the [L4 record](../../../docs/patches/msxml3-selection-removeall.md#validation)
for the tested runtime, trace counts, and outstanding release gates.
