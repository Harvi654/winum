# ole32: OleCreateFromFileEx for a file without an OLE server

Inventor 2027's **Place Imported CAD Files → Reference Model** ends with
"Could not Insert Object". Inventor embeds its translation report, an `.htm`
file in `%TEMP%`, into the assembly with

```
OleCreateFromFileEx(CLSID_NULL, "...\\Temp\\<name>.htm", IID_IUnknown, 0,
                    OLERENDER_NONE, 0, NULL, NULL, NULL, NULL,
                    client_site, storage, &obj)
```

(from a `WINEDEBUG=+ole` trace). Wine's `OleCreateFromFileEx`
(`dlls/ole32/ole2.c`) binds the file moniker to `IDataObject`, i.e. it needs
the file's class (MSHTML for `.htm`) to load and embed the file. MSHTML's
`IPersistFile::Load` is a stub, so the call fails. For files whose class is
unknown it fails too (`MK_E_INVALIDEXTENSION`). Microsoft's documentation
says a file without an OLE server is wrapped in a Package object; Wine has
no such fallback. Convert Model honours the `EmbedInDocument` preference
(the NixOS launcher sets it to 0), Reference Model embeds anyway.

`createfromfile.c` makes the same call for an `.htm`, a `.txt` and a file
with an unregistered extension, and prints the HRESULT, the class written to
the storage, the object's user class and type, which interfaces it has and
the storage's elements. It also prints what MSHTML's HTML document supports.
No Autodesk code is involved.

## Build and run

```sh
nix-build docs/wine-repro/ole32-createfromfile-html \
  --arg pkgs 'import (builtins.getFlake "path:'$PWD'").inputs.nixpkgs {}' \
  -o result-cff
wine result-cff/createfromfile.exe
```

On Windows: copy `createfromfile.exe` and run
`createfromfile.exe > cff-result.txt 2>&1`.

## Results

Wine 11.20 (this repository's build), new prefix, MSHTML registered,
no Gecko:

```
GetClassFile hr 0x00000000, class of .htm {25336920-03F9-11CF-8FD0-00AA00686F13}
CoCreateInstance(HTMLDocument) hr 0x80004005
.htm  (with/without client site): hr 0x80004005, object NULL, storage empty
.txt  : hr 0x800401e6 (MK_E_INVALIDEXTENSION), object NULL
.qqzz : hr 0x800401e6, object NULL
```

In the Inventor prefix (with Gecko) MSHTML is created and the call fails
after `IPersistFile::Load` (`E_NOTIMPL`), see `docs/arch-hyprland.md`.

Windows 10 (a Windows host with a working Inventor), first run:

```
GetClassFile hr 0, class of .htm {25336920-03F9-11CF-8FD0-00AA00686F13}
CoCreateInstance(HTMLDocument) hr 0; IOleObject, IPersistFile, IDataObject,
  IPersistMoniker yes, IPersistStorage no; IPersistFile::Load hr 0
.htm (with and without client site), .txt, .qqzz: hr 0, object returned,
  class in storage and user class {0003000C-0000-0000-C000-000000000046},
  user type "Package", IOleObject/IPersistStorage/IDataObject/IViewObject yes
  storage: \1CompObj (76 bytes), \1Ole10Native (497 / 456 / 467 bytes)
```

So Windows packages the file even though MSHTML could load it. With the draft
patches (`patches/drafts/ole32-createfromfile-package.patch`,
`patches/drafts/packager-save-view-advise.patch`) Wine also returns a Package;
its `\1Ole10Native` is 12 bytes shorter than Windows' for the same path and
contents and `\1CompObj` is 64 bytes, and the object has no `IDataObject`.
The second version of `createfromfile.c` dumps both streams and also checks
`GetClassID`, `IsDirty` and `OleSave` - to be run on Windows.

`stglist.exe file.iam` lists the storages and streams of a compound file
(names, sizes, classes; no contents), used to compare an assembly saved with
Reference Model on Windows and under Wine.

## pkgprobe (2026-11-12, for v3 of the candidate)

`pkgprobe.c` measures the Package object for the points left open by the
cross review: what Save writes (A), extents and Draw per aspect (B), view
advise (C), EnumAdvise (D), OleCreateFromFile failures (E),
OLERENDER_FORMAT (F), which registered classes avoid the Package (G), a
long temporary directory (H), and where the extent comes from (I, J).
Build with `default.nix` (x86_64; i686 with `pkgsCross.mingw32`), run
`pkgprobe.exe`; it prints text only and cleans up its files and HKCU keys.

Windows 10 22H2 (10.0.19045.5555, ACP 1250), the same on x64 and x86:

| Part | Windows |
| --- | --- |
| A | OleSave and IPersistStorage::Save both write the class, `\1CompObj` ("OLE Package") and `\1Ole10Native` |
| B | IViewObject2/IOleObject GetExtent and Draw: S_OK and the same extent for every aspect (1, 2, 4, 8) and lindex (-1, 0, 1) |
| C | SetAdvise with ADVF_PRIMEFIRST (also with ONLYONCE) sends no OnViewChange; GetAdvise returns what was set |
| D | EnumAdvise: E_FAIL before Advise (pointer untouched), E_NOTIMPL and NULL after; Advise/Close send nothing |
| E | missing file, directory, file opened without sharing, path over MAX_PATH: STG_E_FILENOTFOUND; an empty file gives a Package |
| F | OLERENDER_FORMAT (METAFILEPICT, ENHMETAFILE, TEXT): S_OK, no cache stream; GetData works for the two pictures, CF_TEXT DV_E_FORMATETC |
| G | ProgID with or without Insertable: Package; CLSID Insertable without a server: REGDB_E_CLASSNOTREG, nothing written |
| H | OleLoad with a temporary directory too long for the name: E_FAIL |
| I, J | extent in whole pixels at 96 dpi: 40x54 ("r.txt"), 62x54 ("report.txt"), 325x54 (a 49-character name); label text 21/49/259 px in Segoe UI 9 pt; neither the Package's CF_METAFILEPICT (1504x1279 HIMETRIC for "report.txt") nor OleMetafilePictFromIconAndLabel |

### pkgprobe parts K-O and propnull, 2026-11-12 (over ssh, Windows 10 22H2)

Run in an ssh session (icon title font Segoe UI, 25 px high; the earlier
runs from the desktop had 15 px).

| Part | Windows |
| --- | --- |
| K | a Package loads while GetTempPath + name + 1 <= 260 (a 150-character name: lengths 251-260 load, 261 fails with E_FAIL); a second load, which needs " (2)", fails from 257 on |
| L | Draw on a DC scaled x2 with the viewport origin at 50,50: pixels at 98-139 x 59-114 for bounds 10,10-60,60; with the window origin at -20,-20 too, at 138-179 x 99-154 - moved by exactly the mapped offset (40 px); the DC's mapping is unchanged afterwards |
| M | label widths per font (r.txt, report.txt, 49-character name): icon title 30/73/386 px (25 px high) |
| I, J | extents 40x65, 92x65, 483x65 px (text 30/73/386 x 25); with the 15 px font earlier 40x54, 62x54, 325x54 (text 21/49/259 x 15): width = 5/4 x max(icon 32, text + 1), height = 32 + (11 x text height + 55) / 10, both rounded down |
| N | changing the icon title font for a measurement is refused outside an interactive desktop (error 1459), so the height rule rests on these two sizes |
| O | a Package without the UTF-16 names (ANSI only, a 150-character name): the same limit as K - first load fails only at GetTempPath + name + 1 = 261, a second (" (2)") from 257 on; x64 and x86 |

`propnull.c` labelled its two cases wrongly before 2026-11-12 ("ANSI" was
the default, Unicode, set; "Unicode" was flag 4, PROPSETFLAG_UNBUFFERED,
which Windows refuses for a docfile). Corrected and rerun: in both a
Unicode (default, codepage 1200) and an ANSI (PROPSETFLAG_ANSI, codepage
1250) property set, NULL VT_LPWSTR and VT_LPSTR values are written with
length 0 and read back as NULL.
