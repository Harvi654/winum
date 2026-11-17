# shell32-parentrelativeforui.patch

- **DLL:** `shell32`
- **Status:** [candidate](../../patches/candidates/shell32-parentrelativeforui/README.md)
- **Base:** winum-11.22-1 (Wine 11.22)

## Problem as the user saw it

Inventor 2026 and 2027 show only separators in the Open breadcrumb at a
drive root and in a subfolder.

## Cause and fix

The Inventor 2027 trace shows `SHGetNameFromIDList` rejecting
`SIGDN_PARENTRELATIVEFORUI`. In that branch Wine returns `E_FAIL` and
leaves the output name null. Windows 10 build 19045.6466 returned
`S_OK` and a nonempty name equal to `SIGDN_PARENTRELATIVE` for all four
measured items with both APIs and architectures. The drive root gave a
different address-bar name.

Route this SIGDN through the existing parent-relative name path.
`IShellItem::GetDisplayName` already delegates to `SHGetNameFromIDList`.
The change is limited to this SIGDN because only this missing name form
was identified in the trace. The connection to the missing labels is
still a hypothesis.

## Wine test

`dlls/shell32/tests/shlfolder.c` compares names against the same item's
parent-relative name for a filesystem folder, a drive root, Documents and
This PC. It also distinguishes the drive root from its address-bar name.
See the [trace and Windows results](../wine-repro/shell32-parentrelativeforui/README.md).

## Validation

| Level | Result |
| --- | --- |
| L0 | Candidate build requested on `test-run/shell32-parentrelativeforui-1` |
| Initial consumer check | Pending in Inventor 2027 |
| L1 | MEASURE completed on x86_64 and i686; Wine test on Windows pending |
| L2 | Paired shell32 suites pending |
| L3 | No separate sequence or fuzz test planned for this branch |
| L4 | Pending in Inventor 2027 and 2026 |
| L5 | Pending |
| Cross review | Pending |
