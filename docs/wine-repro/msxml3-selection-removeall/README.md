# MSXML selection removal measurement

This synthetic probe measures the `IXMLDOMSelection::removeAll` call
seen after an empty Content Center category query in Inventor 2027
[issue #45](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/issues/45).
It does not use Inventor data.

Each architecture runs 80 cases across DOMDocument30 and DOMDocument60.
The fixtures select zero, one, or three direct children, children of
different parents, and a selection containing both an ancestor and a
descendant. The probe obtains the selection through the native
`IXMLDOMNodeList` return and QueryInterface, or through the dispatch
return of selectNodes and a direct QueryInterface to IXMLDOMSelection.
It checks ISupportErrorInfo on both paths.

Each fresh case calls removeAll or removeNext, with the cursor either
at the start or after one nextNode call. Both removal methods are stubs
on the release baseline. The probe records HRESULTs, the root's child
count and XML, the selection length, and the next node before reset
and again after reset. It appends a new matching child, then compares
the old selection with a fresh query. A returned node is reported with
its XML and whether it still has a parent.

## Offline Windows 10

Verify SHA256SUMS, extract to a new folder, and run `run-measure.bat`.
Return these text files:

- `result-msxml3-selection-removeall-x86_64.txt`
- `result-msxml3-selection-removeall-i686.txt`
- `exitcodes.txt`

A complete probe reports `measure_complete cases=80 failed=0`.
An API failure under measurement is a result. A setup failure counts
against completion. CI also runs the same bundle against winum-11.22-3.
Windows results determine the test and candidate scope.

## winum-11.22-3 baseline

Both architectures completed all 80 cases with exit 0 and identical
case records. All 40 removeAll calls and all 40 removeNext calls
returned E_NOTIMPL. QueryInterface for ISupportErrorInfo returned
E_NOINTERFACE in all 80 cases.

- [Verified offline Windows bundle](https://github.com/Harvi654/winum/actions/runs/45906581797/artifacts/15156934679)
- [Wine baseline results](https://github.com/Harvi654/winum/actions/runs/45906581797/artifacts/15156994739)

## Offline Windows 10 comparison

Both architectures completed all 80 cases with exit 0. The case records
are identical between x86_64 and i686 and the two selection acquisition
paths agree on removal behavior.

| Call | Windows 10 | winum-11.22-3 |
| --- | --- | --- |
| removeAll on an empty or populated selection | S_OK in all 40 cases. Detaches all matching nodes, including with an advanced cursor. | E_NOTIMPL, no change. |
| Selection after removeAll | Length zero, nextNode returns S_FALSE. Reset and a later matching insertion do not repopulate the snapshot. A fresh query sees the inserted match. | Selection retains its original result. |
| removeNext | S_OK and detached node when a next node exists (28 cases), otherwise S_FALSE and no node (12 cases). Only the removed entry leaves the selection. The cursor stays at its current index in the compacted result. | E_NOTIMPL in 40 cases. |
| QueryInterface for ISupportErrorInfo | E_NOINTERFACE on both DOM versions and both acquisition paths. | Same. |

The candidate implements removeAll. removeNext is a separate cursor and
returned-object change. Error-info support already agrees with Windows.
