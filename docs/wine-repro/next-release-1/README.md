# test-run/next-release-1: the candidates for winum-11.22-3

The released series of winum-11.22-2 (73 patches) plus seven candidate
patches, in this order (`patches/series`, `patches/next-release.list`):

| # | Patch | From (branch, commit) | Why there |
| --- | --- | --- | --- |
| 74 | `server-token-integrity-level.patch` | `topic/token-integrity-level` (ce4d904) | wineserver keeps the level per token |
| 75 | `ntdll-token-integrity-level.patch` | same | ntdll reports it (needs 74) |
| 76 | `combase-per-user-classes.patch` (v2) | `topic/combase-per-user-classes-measure` (b70dc0b) | HKCU classes first, skipped at High integrity (needs 74-75 for ordinary processes) |
| 77 | `kernelbase-hkcr-class-reads.patch` (v2, test in the patch) | `topic/kernelbase-hkcr-read-measure` (d29ad38) | HKCR read opens of per-user CLSIDs, after combase finds the class |
| 78 | `wbemdisp-method-invoke.patch` | `topic/wbemdisp-method-invoke-measure` (01cb713) | Nastran's registration script |
| 79 | `msado15-stream-charset.patch` (v2, test in the patch) | `topic/msado15-stream-charset-measure` (71cd531) | Nastran's registration script |
| 80 | `win32u-font-smoothing.patch` (v2) | `topic/win32u-cleartype-measure` (9c44f12) | unrelated to Nastran, last |

All seven apply to Wine 11.22 + the released series with `--fuzz=0`
(checked 2026-11-24). Tests not yet inside their patch are applied to both
sides from `patches/tests-first/` (token, combase, wbemdisp, win32u).

## Runs

| What | Run | Artifact |
| --- | --- | --- |
| L0: "Build Wine", workflow_dispatch on this branch | pending | `winum-11.22-2-binary` (the name is the released version; the content is the 80-patch series) |
| L2: "Test series", released series against series + candidates, five groups | pending | `l2-report` |

Builds of the topic branches do **not** contain the candidates; only builds
of this branch do. Check a runtime before the L4: the token MEASURE
program (`docs/wine-repro/server-token-integrity/`) must print
`process.integrity=S-1-16-8192` for a program started from Linux.

L2 groups: g1 advapi32 ntdll kernelbase kernel32; g2 combase ole32
oleaut32 urlmon shell32; g3 wbemdisp wbemprox msado15 msi; g4 win32u gdi32
comctl32; g5 user32. Both sides build the released series; only the
candidate side gets patches 74-80 (`scripts/test-series.sh` on this
branch).

## L4 checklist (maintainer)

Runtime: the L0 artifact above, unpacked; **no overlays** (all seven fixes
are in it). Use copies of prefixes. Stop the old wineserver first: the
server protocol differs from winum-11.22-2.

Inventor 2027.2 (fully supported release), started from the launcher
without a virtual desktop:

- [ ] start, sign-in state kept, Home page
- [ ] part: sketch, extrude, fillet; save, close, reopen
- [ ] assembly: place two parts, constraint; save, reopen
- [ ] drawing: base and projected view, dimension; save, reopen
- [ ] Open/Save dialogs (shell32 fixes of 11.22-2 still fine)
- [ ] Stress Analysis or Frame Analysis: one study, results
- [ ] fonts look as before or better (win32u font smoothing: ClearType on
      the desktop setting)
- [ ] restart Inventor, the last file opens
- [ ] add-ins: `scripts/addins.sh list` unchanged

Inventor Nastran 2027 (in the Inventor 2027.2 prefix copy):

- [ ] Nastran environment, Generate Mesh, Run (linear static), results -
      no 0x80040154 dialog
- [ ] `wine cscript //nologo 'C:\newpfui.vbs' NEWPFUI.NEProgressImpl` ->
      `err=0x0`

Install from an ODIS image (fresh prefix, the installer now runs at
Medium integrity; elevated installers stay High):

- [ ] `scripts/install.sh` with the Inventor 2027.2 image into a new
      prefix with this runtime: all phases, exit 0, Inventor starts
- [ ] Nastran from its Create Installer image into that prefix
      (`install-from-image.py`): `Register_Addin` completes, Nastran in the
      Add-In Manager

AutoCAD 2027 (wineserver, ntdll and combase are shared):

- [ ] start, sign-in kept, open a DWG, SAVE, SAVEAS, close; idle
      15 minutes without "Connection Lost"

Report per item: pass, fail (with a filtered log line), or not run.
Known and not part of this release: the black bar above the Nastran
result legend, the four unregistered mscoree CLSIDs, explorer.exe's
High token (Inventor with `INV_VDESKTOP`), the Frame Analysis repaint.
