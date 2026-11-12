# Inventor Import CAD "Reference Model": candidate

Original, AI-assisted (Claude), LGPL-2.1-or-later. Not submitted upstream.
Promoted into `patches/series` on 2026-11-12 for winum-11.20-2 (the files there are byte-identical to these).

## Problem

Inventor 2027 (Autodesk-Inventor-for-Linux issue #10): Import CAD of a STEP
file ends with "Could not Insert Object". Inventor embeds its translation
report (`.htm`) with `OleCreateFromFileEx` (Convert Model only while
`Translators\EmbedInDocument` is on, Reference Model always), and Reference
Model stores the part inside the assembly (`LocalDocs`); without the
storage fixes the part is unresolved after reopening.

## The four fixes, in the order Inventor hits them

| Patch | What Inventor does | Wine before | Windows (measured) |
| --- | --- | --- | --- |
| `ole32-createfromfile-package` | `OleCreateFromFileEx` on the report `.htm` | fails (MSHTML cannot load it) | creates a Package `{0003000C-...}` |
| `packager-save-view-advise` | asks the Package for IViewObject2, Advise, user class, save | E_NOTIMPL / no interface | all work |
| `ole32-storage-localdocs` | `StgCreateDocfile("**<iam>*LocalDocs*<id>*<part>.ipt")`, then opens `LocalDocs` with `STGM_SIMPLE` and creates the part's storage in it | E_FAIL, then STG_E_INVALIDFUNCTION | STG_E_INVALIDNAME; the sub-storage works normally |
| `ole32-propset-null-strings` | writes a VT_LPWSTR property whose pointer is NULL | access violation in lstrlenW, save aborted | stored with length 0, read back as NULL |

Found with Wine traces only (no Autodesk code read beyond crash
addresses). Windows measurements and the programs that made them:
[docs/wine-repro/ole32-createfromfile-html](../../../docs/wine-repro/ole32-createfromfile-html/README.md).

## Validation (patches v5, 2026-11-12)

| Level | Result |
| --- | --- |
| L0 | the full series plus this candidate applies to clean Wine 11.20 with `--fuzz=0`; the Wine tree builds with it (Inventor repository's Nix build, WoW64) |
| L1 | Windows 10 22H2 (10.0.19045.5555), x86_64 and i686, run over ssh: `packager_test oleobj` 93, `ole32_test storage32` 19119, `stg_prop` 304, `ole2` 3271 checks, 0 failures, no skips (exact extents checked: Segoe UI 25 px, 96 dpi) |
| L2 | paired, x86_64: winum-11.20-1 with its own ole32/packager vs. with this candidate's, every suite of `ole32_test` (clipboard, compobj, defaulthandler, dragdrop, errorinfo, hglobalstream, marshal, moniker, ole2, ole_server, propvariant, stg_prop, storage32, usrmarshal) and `packager_test oleobj`: with the candidate 0 failures in all (ole2 skips the exact extents: Wine's icon title font is not a measured configuration); without it failures only in the suites with the new checks |
| L4 | NixOS, Inventor 2027.1 (`nix/wine.nix` `candidates`), v5, 2026-11-12, in an isolated X11 session on a copy of the user state: Import CAD Reference Model imports and places, the assembly saves, and after a restart reopens with the part resolved and drawn; Convert Model with the import report embedded (`EmbedInDocument=1`) imports, places and saves, and the converted part holds the report as an OLE Package (`RSeEmbeddings/Embedding 1/\1Ole10Native`). v2 earlier on the user's own session. SOLIDWORKS (ole32 is shared): deferred, not being worked on since 2026-11-09 |

## Review

Codex cross-reviewed v2 to v5 on 2026-11-12: **v5 approved** (all four
patches; no remaining blocking defect, no distinctive copied code
recognized). The Package patches were
revised twice after measuring each open point on Windows with `pkgprobe`:
v3 - bounded temporary path, no 1 GiB limit, Save writing the class and
`\1CompObj`, EnumAdvise and error codes as on Windows, only a CLSID
`Insertable` key keeps a class's path; v4 - the temporary path checked
exactly as Windows does (with and without a collision), Draw keeping the
caller's mapping, the extent matching Windows for every measured case, no
unmeasured ANSI-path restriction in the writer, and the review's test
notes (unique names, the GetAdvise sink, NULL strings in Unicode and ANSI
sets and as replacements); v5 - an ANSI-only Package's name no longer
counted with its terminator (Windows has the same limit with and without
the UTF-16 names, measured), exact extents asserted only in the measured
configurations. Details in the vN paragraphs of the headers.

Known differences from Windows: the extent rule is inferred from two
icon title font sizes (the font cannot be changed for a measurement
outside an interactive desktop); Draw places the icon and label in its
own layout inside the mapped bounds, not at Windows' exact pixels; what
Windows does with a path whose ANSI form is longer than MAX_PATH (a
multibyte code page) is not measured; Wine's packager has no IDataObject.

## Open

- promotion into `patches/series`: done 2026-11-12 (winum-11.20-2). The
  maintainer's L1 (offline Windows 10 VM), the winum L2 runs and the L4 on
  Arch are on the four pages in `docs/patches/`.

## Resume

Apply `series` after `patches/series`. Tests:
`dlls/ole32/tests/{ole2,storage32,stg_prop}.c` and
`dlls/packager/tests/oleobj.c`; the ole32 extent test needs the packager
patch. Wine's HKEY_CLASSES_ROOT does not show HKCU-only classes to
RegQueryValueW, so the ole2 test registers in HKCR where it may and falls
back to HKCU (Windows refuses new HKCR keys to a standard user).
