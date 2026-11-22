# msxml3-replacechild-same-node.patch

<!-- winum-test: l1 -->

- **DLL:** `msxml3`
- **Status:** In the series (merged into `main` 2026-11-22).
- **Base:** winum-11.22-1 (Wine 11.22)
- **Contributed by:** Lukas10999 ([pull request #4](https://github.com/Harvi654/winum/pull/4)) - the fix, the first regression test and the Inventor reproducer. The test revision from the Windows measurement is by Codex (`e2b2162`).

## Problem as the user saw it

Inventor 2027 waited for ever on a DWG/DXF preview, and DWG TrueView's
`Profile.aws` grew to 1.1 GB (Lukas10999). AutoCAD 2027's own
`Profile.aws` grew to 11 MB and 91 MB in the maintainer's tests, and
AutoCAD reported "Unable to load profile file" - same file, same code.

## Cause and fix

The program selects the same element twice (two COM wrappers for one
node) and calls `replaceChild` with both. Wine inserted the node before
itself and then removed it: `domnode_insert_domnode` unlinks the child
and links it before the reference - the same, just-unlinked entry - so
the parent's child list became a cycle, and saving appended the subtree
for ever. The fix compares the underlying nodes, not the wrappers:
`replaceChild` of a node with itself changes nothing and returns it;
`insertBefore` of a node before itself does nothing after the usual
checks, so the document root keeps Windows' `E_FAIL`.

## Validation

| Level | Result |
| --- | --- |
| L0 | Built with the patch on Wine 11.22, x86_64 and i386 ([run 44534917242](https://github.com/Harvi654/winum/actions/runs/44534917242)); the full series on `topic/next-release` builds in its own CI run |
| L1 | Windows 10 (offline VM, build 10.0.19045.6466), `msxml3:domdoc` of the test revision `e2b2162`: 37,156 checks, 0 failures, 0 todos, exit 0, on x86_64 and i386 (maintainer, 2026-11-19). Expected values from the MEASURE program, [docs/wine-repro/msxml3-same-node-measure](../wine-repro/msxml3-same-node-measure/README.md) |
| L2 | Paired msxml3 suites with and without the patch, x86_64 and i386, [run 44534917242](https://github.com/Harvi654/winum/actions/runs/44534917242): passed |
| L3 | Standalone MEASURE on independent trees (element and document root, with and without an output pointer) |
| L4 | Lukas10999: Inventor 2027 sketch preview with DWG TrueView finished, profile stayed at about 21 KB (Wine 11.20). Maintainer: AutoCAD 2027 profile starts with the patch on 11.22 ([record](https://github.com/Harvi654/winum/blob/62fb3dc/docs/wine-repro/msxml3-same-node-measure/README.md)). Inventor smoke test with the release build: see the next row |
| L4 (release build) | `topic/next-release` at `3785565`, full series built in [run 45052921398](https://github.com/Harvi654/winum/actions/runs/45052921398) (winum-11.22-2 test build): Inventor 2027 on a copy of the maintainer's prefix - Open dialog breadcrumb names, New Folder and Enter in the name editor, typing a file name in Save As, start, part, assembly, drawing, save/reopen, restart: passed (maintainer, 2026-11-22). |
| L5 | Pending |
| Cross review | Codex reviewed the contributor's patch (`126157d`). Claude Opus 5.5 (2026-11-22) read the runtime hunks and the test in full against Wine 11.22's `node.c` and approves: the same-node guard in `node_insert_before` skips an unlink/relink that cancels out in the reference counts; the `node_replace_child` guard comes after all validation; the document-root case keeps `E_FAIL`. Note: the contributor's patch was also written with Claude, so the Codex review is the one by a different model family. |

## Open points

- msxml3 is being reimplemented upstream (since Wine 11.7, without
  libxml2): check the patch against `node.c` at every Wine update.
- Bug report for Wine (in words): `replaceChild(x, x)` and
  `insertBefore(x, x)` with two wrappers of one child corrupt the child
  list; Windows returns `S_OK` and leaves the tree unchanged.

## History

- **2026-11-17**, pull request #4 by Lukas10999 (candidate).
- **2026-11-19**, Windows measurement and test revision `e2b2162`, L1 and
  paired L2 (Codex, maintainer).
- **2026-11-22**, taken into the series on `topic/next-release` with the
  contributor credited; pull request #4 closed in favour of it.
