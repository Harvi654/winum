# winum-11.22-3

Wine 11.22, `WINUM_BUILD=3`: the 73 patches of winum-11.22-2 plus seven
new ones, 80 in all. Wine itself and its source pin are unchanged
(`WINE_SHA256` as for 11.22-1). The wineserver protocol changes (980),
so wineserver and ntdll must come from this build together, as they do
in every winum runtime.

## New in this build

Inventor Nastran 2027 (companion product in the Inventor 2027.2 prefix):
it installs, registers its add-in, and Generate Mesh, Run and the results
work without the "Could not start the user interface components ...
0x80040154" dialog.

- [`server-token-integrity-level`](../patches/server-token-integrity-level.md)
  and [`ntdll-token-integrity-level`](../patches/ntdll-token-integrity-level.md) -
  a normally started program now reports Medium integrity, as on Windows
  (Wine reported High for every program). The .NET Framework COM shim
  ignores per-user registrations at High; Nastran registers its .NET
  classes per user.
- [`combase-per-user-classes`](../patches/combase-per-user-classes.md) -
  COM finds in-process classes, ProgIDs and TreatAs registered for the
  current user (`HKCU\Software\Classes`) first, and, as on Windows, not
  for programs at High integrity.
- [`kernelbase-hkcr-class-reads`](../patches/kernelbase-hkcr-class-reads.md) -
  reading a class registration through `HKEY_CLASSES_ROOT` also sees the
  per-user one (part of Wine bug 17019).
- [`wbemdisp-method-invoke`](../patches/wbemdisp-method-invoke.md) -
  scripts can call WMI methods such as `StdRegProv.GetStringValue`
  directly (Wine bug 39463); Nastran's installer registers its add-in.
- [`msado15-stream-charset`](../patches/msado15-stream-charset.md) -
  `ADODB.Stream` reads and writes UTF-8 and single-byte text and saves
  and loads files; Nastran's add-in file gets its full path.

Inventor text:

- [`win32u-font-smoothing`](../patches/win32u-font-smoothing.md) -
  ClearType and the desktop font smoothing setting take precedence over
  fontconfig's per-font flags (Inventor issue #13). Diagnosis and first
  probe by @krokosik (Inventor PR #18) - thank you.

## What changes for users

Programs started from Linux (the launcher, a shell, a desktop file) now
run at Medium integrity, like a normally started program on Windows.
Elevation is unchanged (`TokenElevationType`, `TokenElevation`), and
programs that Wine elevates - a manifest that asks for administrator,
explorer.exe and what it starts - stay at High. Wine does not enforce
integrity levels in access checks, so no access changes. One
consequence: Inventor started inside a Wine virtual desktop
(`INV_VDESKTOP`) is a child of explorer.exe and stays at High, so
Nastran's dialog remains there; without a virtual desktop (the default)
it is gone.

## Validation

| Level | Result | Date |
| --- | --- | --- |
| L0 | all 80 patches apply to the official 11.22 archive with `--fuzz=0`; built for both architectures as the combined test build [run 45485575952](https://github.com/Harvi654/winum/actions/runs/45485575952) (the same code; four tests moved into their patches afterwards, their `todo_wine` marks dropped where the fixes pass) and by "Build Wine" on the release branch with the final patches, [run 45644752941](https://github.com/Harvi654/winum/actions/runs/45644752941) | 2026-11-24/25 |
| L1 | every new patch's Wine test passed on offline Windows 10, x86_64 and i686 (per patch page); the token test also from an elevated console, combase also at High integrity. The combase test changed in v4 (one skip for a fresh Wine prefix) and is run again on Windows 10 before the tag | 2026-11-23 to 25 |
| L2 | per patch, paired against the series without it (patch pages), and all seven together against the released 11.22-2 series: [run 45485567767](https://github.com/Harvi654/winum/actions/runs/45485567767), 452 units in five groups (token/ntdll/kernel, COM, WMI/ADO/MSI, fonts, user32), no new failure. The release branch itself, paired the same way: [run 45627213749](https://github.com/Harvi654/winum/actions/runs/45627213749) found the combase x86_64 32-bit user view case failing on a fresh prefix (fixed in the test, combase v4); [run 45644760664](https://github.com/Harvi654/winum/actions/runs/45644760664) with v4: all new units pass (combase 56/47, msado15 335, wbemdisp 135 checks per architecture, 0 failures); the two units the compare step lists, `ntdll.i386.time` line 460 and `ntdll.x86_64.threadpool` line 1645, are timing checks that have shown up on both sides of earlier runs (`time.c:460` listed as fixed in run 45457615532, `threadpool.c:1645` in run 45307710447); the `ntdll.x86_64.virtual` c0000220 of the first run did not recur | 2026-11-24/25 |
| L3 | reproducers without vendor files: a .NET class registered per user only (Windows: works at Medium, fails at High; Wine: the same with this build), the COM, HKCR, WMI and ADODB.Stream reference programs (per patch page) | 2026-11-23 to 24 |
| L4 | with the combined build (maintainer): Inventor 2027.2 + Nastran prefix copy - part, assembly, drawing, open/save/save-as and reopen, Stress Analysis, Frame Analysis, Frame Generator, Nastran Generate Mesh / Run / results without the dialog; AutoCAD 2027.1.1 fresh install from its Create Installer image (33 steps, exit 0), sign-in, open, SAVE, SAVEAS, reopen. Not run: Inventor install into a new prefix (the AutoCAD install covered the same installer path), a 15-minute AutoCAD idle. SOLIDWORKS deferred (AGENTS.md) | 2026-11-24/25 |
| Cross review | every new patch read in full by a second model (per patch page): Claude's token patches by Codex; Codex's combase (v1 and v2), kernelbase, wbemdisp, msado15 and win32u patches by Claude | 2026-11-23 to 25 |

`scripts/check-series.sh`: 80 entries with L1 (69), an L3-only reason (9)
or a known deviation (2), 0 without.

## Known issues

- Inventor: Nastran and then Stress Analysis in the same session show no
  mesh until the part is reopened - the same on Windows, Autodesk's
  behaviour (Inventor issue #41, closed).
- AutoCAD: DXVK and vkd3d-proton break AutoCAD 2027 (orbit crash on
  Direct3D 12 also with 11.22-2, missing overlays on Direct3D 11); Wine's
  own Direct3D works. Recorded in the AutoCAD project, not a winum change.
- SOLIDWORKS: the property manager's checkmark and cross buttons (since
  11.22-1), as recorded on
  [`comctl32-ownerdraw-existing-capture`](../patches/comctl32-ownerdraw-existing-capture.md).

## Not in this build

Recorded as open, not changed here: the token object's own label, the
label in `TokenGroups`, lowering a token with `SetTokenInformation` and
`TokenMandatoryPolicy` (measured on Windows, page of
`server-token-integrity-level`); explorer.exe's administrator token; the
black bar above Nastran's result legend; four unregistered CLSIDs that
`mscoree` is asked for in Inventor (no visible effect); the Frame Analysis
repaint. Candidates that stay candidates: AF_UNIX and iphlpapi for
RobotStudio, `wldap32`, the winex11 unmanaged-window candidate (PR #3),
the per-monitor work area for Inventor issue #31.

## Open after the release

- Autodesk-Inventor-for-Linux pins winum-11.22-3 (`wine/version.env`,
  `wine/SHA256SUMS`, the `winum` flake input) in one commit, after this
  release exists; Autodesk-AutoCAD-on-Linux likewise. Programs that hand a
  sign-in callback to a running session need the callback handler from
  the same runtime (protocol 980).
- Wine 11.23: rebase, then full validation.
