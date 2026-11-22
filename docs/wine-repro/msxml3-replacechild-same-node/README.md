# msxml3: replaceChild with the same node (DWG TrueView 2027 profile)

How the loop in DWG TrueView 2027 under Inventor 2027 was found, and the
paired test runs of the candidate
`patches/candidates/msxml3-replacechild-same-node/`.

## Symptom

Inventor 2027 (Wine 11.20, the Inventor project's `wine-11.20-6`) imports
a DXF; the import dialog shows "Loading presentation" and never finishes.
`dwgviewr.exe /Automation /Language en-US /InPlaceServer -Embedding` is
running (Linux comm `Main/UI`, so match the command line, not the name),
one core busy, and
`AppData/Roaming/Autodesk/DWG TrueView 2027 - English/R25/enu/Support/Profiles/Unnamed Profile/Profile.aws`
grows without end (1.1 GB after one run; 25 KB when clean). Killing the
viewer lets the import finish (the lines appear; the earlier "point
cloud" result was the viewer's half-done preview). With the viewer
disabled (renamed), the import works and only the preview is missing.

## Trace

`WINEDEBUG=+msxml,+pid,+timestamp` on the launcher; the viewer inherits
it. The last pass over the profile before the save that never returns
(wrappers abbreviated, the full pass is 157 lines):

```
domelem_get_parentNode   ...9FF8
domelem_selectSingleNode ...9FF8, L"AcApData"      -> ...18F8
domelem_get_parentNode   ...9FF8
domelem_selectSingleNode ...9FF8, L"AcApData"      -> ...1DB8
domelem_replaceChild     ...9FF8, ...18F8, ...1DB8   (new, old: the same node)
domelem_selectSingleNode ...9FB8, L"Name"   put_text L"<<Unnamed Profile>>"
domelem_selectSingleNode ...9FB8, L"Description"   put_text L""
domdoc_save  {VT_BSTR: L"C:\\users\\<user>\\...\\Unnamed Profile\\Profile.aws"}
node_save Using L"UTF-8" encoding.
```

`winedbg` (`attach`, `thread`, `bt`) on the viewer's main thread showed
`node_save` walking the children of `AcApData`. In Wine's msxml3 every
`IXMLDOMNode` is a wrapper created per access over one `struct domnode`;
`selectSingleNode` twice gives two wrappers for the one element.
`node_replace_child(parent, new, old)` calls `node_insert_before(parent,
new, ref = old)`: `domnode_insert_domnode` does `domnode_unlink_child(new)`
(list_remove) and `list_add_before(&old->entry, &new->entry)`, which with
`old == new` links the entry to itself. `node_remove_child(old)` then
unlinks it once more. The previous sibling still points at the node, the
node points at itself: `list_count`, `node_save` and every other walk of
the children loop forever.

## Fix

`patches/candidates/msxml3-replacechild-same-node/`: compare the
`struct domnode` of both children in `node_replace_child` and return S_OK
with the old child when they are the same; in `node_insert_before`, skip
`domnode_insert_domnode` when the reference is the node itself.

## Running the test against a different msxml3

Wine 11 loads a builtin DLL from its own `lib/wine/x86_64-windows/`
directory whenever a file of that name is found - a `msxml3.dll` next to
the test executable, in the prefix's `system32`, or in `WINEDLLPATH` is
reported by `+loaddll` but the stock copy runs (`WINEDLLPATH` entries are
searched after the default directory, and a Wine-built PE is refused as
"native"). The paired runs therefore swap the file in the runtime:

```
R=~/.local/share/inventor-on-linux/wine-inventor-11.20-6-x86_64
L=$R/lib/wine/x86_64-windows/msxml3.dll; cp -p $L $L.orig
cd wine-11.20 && make dlls/msxml3/x86_64-windows/msxml3.dll dlls/msxml3/tests/x86_64-windows/msxml3_test.exe
cp dlls/msxml3/x86_64-windows/msxml3.dll $L        # unpatched or patched build
WINEPREFIX=~/mx-prefix WINEDEBUG=-all $R/bin/wine dlls/msxml3/tests/x86_64-windows/msxml3_test.exe domdoc
```

Check with `winedbg` (`x/8i` at the hang) or `+loaddll` plus the symbol
offsets that the code in memory is the file you built: this cost an
evening when the system32 copy and the app-dir copy were silently
ignored.

## Paired runs (Wine 11.20 x86_64, 2026-11-17)

The same `msxml3_test.exe` (with the new checks), unpatched vs patched
`msxml3.dll` from the same build tree, each in its own prefix:

| Unit | unpatched | patched |
| --- | --- | --- |
| domdoc | 42905 checks, 6 failures (`4493` hr S_FALSE from `get_parentNode`, `4494` child lost its parent, `4503` last child `empty`, `4521` `insertBefore` E_FAIL, `4522` NULL, `4529` last child `empty`) | 42913 checks, 0 failures |
| saxreader | 7531, 0 failures | 7531, 0 failures |
| schema | 643, 0 failures | 643, 0 failures |
| httpreq | 417, 0 failures | 417, 0 failures |
| xmlview | 2 failures (no XMLView instance, no display) | the same 2 |

Before the test was restructured, both variants of the first version hung
in `xmlnodelist_get_length` after `replaceChild`: the checks that walk
the list now run only when the last child is still `fo`.

## L4 (2026-11-18)

Patched `msxml3.dll` in the runtime, viewer enabled, Inventor 2027: 2D
sketch, Insert AutoCAD file, the same DXF. The preview renders, the import
finishes, `dwgviewr.exe` runs about ten seconds and exits, `Profile.aws` is
rewritten at 21 KB. A watcher logged the profile size and the viewer
process every two seconds. Direct Open of a DXF uses the DWG/DXF wizard
without the viewer and was never affected.

## Other findings of the same session (not msxml3)

- First run of the viewer in a fresh prefix: `RegLoadAppKey` is a stub in
  Wine, the viewer's profile seed crashes; seeding its HKCU keys from a
  Windows install works around it (Inventor project).
- `d3dcompiler_47.dll` must sit next to `dwgviewr.exe` (Inventor project).
