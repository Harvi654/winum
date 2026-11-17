# msxml3-replacechild-same-node (candidate)

`IXMLDOMNode::replaceChild` with two interface pointers to the same node,
and `insertBefore` of a node before itself, corrupt the parent's child
list in Wine's msxml3 (a cycle), and the next `save` never ends. DWG
TrueView 2027 does exactly this when Inventor 2027 starts it as the
in-place server for a DWG/DXF preview: its profile `Profile.aws` grew to
1.1 GB, the viewer never answered, and the DXF import in Inventor hung
until the viewer was killed. The patch compares the underlying node, not
the wrapper, and leaves the tree alone in both cases. Original code,
AI-assisted (Claude), LGPL-2.1-or-later. Not submitted upstream.
Contributed from the Inventor project (Lukas10999); the findings are in
`docs/wine-repro/msxml3-replacechild-same-node/`.

Not in `patches/series`. Applies on vanilla Wine 11.22 and after the three
msxml3 patches of the series (`git apply --check`, 2026-11-17).

## What it changes

- `node_replace_child()`: after the cycle check, if the new and the old
  child are the same `struct domnode`, return S_OK with the node as the
  removed child and change nothing. Before, `node_insert_before` with the
  node as its own reference unlinked its list entry and linked it to
  itself; the following `node_remove_child` then unlinked it again, and
  the parent's children list ended in a loop.
- `node_insert_before()`: `domnode_insert_domnode` is skipped when the
  reference node is the node itself (it is already where it would go).
- Test (`tests/domdoc.c`, end of `test_replaceChild`): the last child of
  the `complete4A` root through two `get_item(3)` calls; `replaceChild`
  and `insertBefore` with the pair; checks S_OK, a non-NULL removed node,
  the node keeps its parent, the last child is still `fo`, the child
  count stays 4 and `get_xml` is unchanged. The list walks (`get_length`,
  `get_xml`) run only when the last-child check passed, so vanilla Wine
  fails instead of hanging.

## Test on Windows (L1, open)

The expected values are those of the W3C DOM (nothing changes, the node
is returned); they have not been measured on Windows yet. If Windows
returns something else for the same-node case (another HRESULT, or NULL),
the test is wrong and the patch follows the measurement.

## Validation (Wine 11.20, the Inventor project's `wine-11.20-6` runtime)

Paired runs of `msxml3_test.exe`, the same test binary against the
unpatched and the patched `msxml3.dll` from the same build tree
(`docs/wine-repro/msxml3-replacechild-same-node/README.md` has the table
and the commands):

| Level | Result |
| --- | --- |
| L0 | `msxml3.dll` and `msxml3_test.exe` build in the configured 11.20 tree (x86_64, mingw); not yet in winum's workflow |
| L2 | `domdoc`: 42905 checks, 6 failures unpatched (the new checks), 42913 checks, 0 failures patched. `saxreader`, `schema`, `httpreq`: identical. `xmlview`: 2 failures in both (no XMLView instance, pre-existing, no display) |
| L4 | Inventor 2027, 2D sketch, Insert AutoCAD file (DXF), viewer enabled, patched `msxml3.dll` in the runtime: the preview renders, the import finishes, `dwgviewr.exe` runs about ten seconds and exits, `Profile.aws` is rewritten at 21 KB (contributor, 2026-11-18). Before the patch the same step hung and the profile grew without end. Direct Open of a DXF goes through the DWG/DXF wizard without the viewer and was never affected. |

## Open

- L1: the test on Windows 10 (both architectures).
- The maintainer's L4 smoke test (the contributor's L4 above is the
  feature itself).
- Cross review by a second assistant.
- Whether to report it to Wine: the synthetic test alone reproduces it,
  no vendor binary is needed.
