# shell32-parentrelativeforui.patch

<!-- winum-test: l1 -->

- **DLL:** `shell32`
- **Status:** In the winum-11.22-2 series on `topic/next-release`. [Candidate history](https://github.com/Harvi654/winum/blob/43693a2/patches/candidates/shell32-parentrelativeforui/README.md)
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
was identified in the trace. The maintainer confirmed that the candidate restores the drive and folder
labels in the Inventor 2027 Open dialog.

## Wine test

`dlls/shell32/tests/shlfolder.c` compares names against the same item's
parent-relative name for a filesystem folder, a drive root, Documents and
This PC. It also distinguishes the drive root from its address-bar name.
See the [trace and Windows results](../wine-repro/shell32-parentrelativeforui/README.md).

## Validation

| Level | Result |
| --- | --- |
| L0 | Passed, Wine 11.22 x86_64 and i386 with `--fuzz=0`, [build 44203957500](https://github.com/Harvi654/winum/actions/runs/44203957500) |
| Initial consumer check | Breadcrumb labels restored in Inventor 2027 and 2026 on Arch/KDE, candidate `9cd7d88` (2026-11-18) |
| L1 | Passed for the new checks on x86_64 and i386 in the offline Windows 10 VM. The full shlfolder unit reported 3903 checks / 0 failures on x86_64 and 2116 checks / 3 failures on i386. All three i386 failures are in the pre-existing file timestamp check at shlfolder.c:1974. |
| L2 | Passed, paired winum-11.22-1 shell32 suites with and without the fix, x86_64 and i386 ([run 44210344999](https://github.com/Harvi654/winum/actions/runs/44210344999)) |
| L3 | No separate sequence or fuzz test planned for this branch |
| L4 | Passed in Inventor 2027 and 2026 on Arch/KDE, candidate `9cd7d88` (maintainer, 2026-11-18). The Open breadcrumb works in both versions and the maintainer reported no regression in the L4 smoke test. SOLIDWORKS and AutoCAD deferred under AGENTS.md. |
| L5 | Pending |
| Cross review | Approved by an independent Codex reviewer (2026-11-18). Both candidate diffs were read in full. |
| Cross review (second model) | Approved by Claude Opus 5.5 (2026-11-22), patch and test read in full. The fix maps only `SIGDN_PARENTRELATIVEFORUI` onto the existing `SIGDN_PARENTRELATIVE` branch of `SHGetNameFromIDList` (`SHGDN_INFOLDER`), which matches the four measured items; `IShellItem::GetDisplayName` reaches it through the same function. The test compares against names Wine already returns for the same item, not English strings, and keeps the drive-root distinction from `PARENTRELATIVEFORADDRESSBAR`. Open, not blocking: only folders were measured - a file item (extension shown or hidden) is not covered; the breadcrumb itself only shows folders. |

## Inventor 2027 Stress Analysis smoke check

The maintainer solved a static cantilever on three meshes on Linux. The
steel beam was 300 mm long, with a 20 mm by 10 mm cross section, a fixed
end and a 20 N transverse force at the free end. The free end displacement
was 0.5124, 0.5125 and 0.5126 mm for 2516, 18306 and 153324 elements.
The beam formula gives 0.5143 mm with a Young's modulus of about 210 GPa.
The two finer meshes reported 9.17 MPa von Mises stress at the same probe
near midspan. This check exercises mesh generation and the static solver.

## STEP import observation

During a separate Inventor 2026 STEP import attempt, a Wine debugger
reported an execute access fault in `C:\windows\system32\reg.exe`.
The fault address had no accessible code and the module list for the
faulting process did not contain shell32. Inventor stayed running after
the debugger was dismissed and the STEP import completed. The maintainer
then created a new assembly and imported STEP three more times with the
same candidate. All three attempts succeeded. The follow-up excerpt
shows no repeat of the reg.exe fault. Its cause remains undetermined.
