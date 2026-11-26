# riched20 ITextDocument default tab stop

Revit 2027.4 reports a failure setting the default tab size when building
the first plan view. Wine 11.22 stubs the ITextDocument default tab-stop
methods. The journal also reports an access violation during view
construction. The controlled L4 comparison below confirms that Revit
requires this change together with paragraph navigation v3.

[Native MEASURE program and instructions](../../../docs/wine-repro/riched20-default-tab-stop/README.md).

| Level | Evidence |
| --- | --- |
| MEASURE | Two offline Windows 10 rounds agree across architectures. [Observations](../../../docs/wine-repro/riched20-default-tab-stop/README.md#round-2-result-2026-11-26). |
| L0 | Both DLL architectures built after zero-fuzz application to winum-11.22-3. [Overlay build](https://github.com/Harvi654/winum/actions/runs/45854157578). |
| L1 | Offline Windows 10 passed 96 checks each on x86_64 and i686 on 2026-11-26. Both exited 0 with no failures, todos, flaky checks or skips. [Results](../../../docs/wine-repro/riched20-default-tab-stop/results/l1-windows-10/summary.tsv). [Verified L1 artifact](https://github.com/Harvi654/winum/actions/runs/45853726852/artifacts/15135495595). |
| L2 | Paired riched20 and msftedit passed on 2026-11-26. Ten units compared across x86_64 and i386, zero regressions. Each richole architecture ran 181,682 checks on the candidate (181,587 baseline), with no failures or skips in that unit. [CI](https://github.com/Harvi654/winum/actions/runs/45854157626). |
| L4 | Revit 2027.4 New Project reached “Podlaží 1” on 2026-11-26 with the combined v3 navigation, tab-stop and ole32 overlays in DirectX Software mode. Each fix in isolation still crashed at TextEngine.dll+0x2ac70. See the comparison below. |
| Cross review | Claude, 2026-11-26: approved for the series after the combined Inventor and AutoCAD smoke test; open points below. |

The editor stores the width in twips. Ordinary selection edits keep it
and WM_SETTEXT restores 720 twips. The setter relayouts implicit stops,
and the existing msftedit class flag selects the measured modern range.
A zero or overflow result stays visible to the getter. Layout uses
the normal interval for these values because their layout behavior
was outside the native range probe.

Language and typography option messages differ from Windows in the
trace and reference probe. The successful combined Revit run used the
existing handling of those messages.

## Revit L4 comparison (2026-11-26)

Revit 2027.4 was tested with the ole32 overlay and DirectX Software
rendering (WARP on wined3d).

| RichEdit overlay | New Project result |
| --- | --- |
| Default tab stop only | Crashed at `TextEngine.dll+0x2ac70`. |
| Paragraph navigation v3 only | Crashed at `TextEngine.dll+0x2ac70`. The trace logged two calls to the `ITextDocument2Old_fnGetDefaultTabStop` stub and one to the `ITextDocument2Old_fnSetDefaultTabStop` stub. [Overlay](https://github.com/Harvi654/winum/actions/runs/45875408643/artifacts/15144173677). |
| Paragraph navigation v3 and default tab stop | Opened “Podlaží 1”. The TextEngine crash did not occur. [Overlay](https://github.com/Harvi654/winum/actions/runs/45871395012/artifacts/15142867376). |

Both RichEdit changes are required for this Revit New Project fix.

The [tab-stop-only overlay](https://github.com/Harvi654/winum/actions/runs/45854157578/artifacts/15135865485) is retained for comparison.

## Cross review (Claude, 2026-11-26)

Read in full: `riched20-default-tab-stop.patch` (v2) and
`patches/tests-first/riched20-tests-default-tab-stop.patch`; checked
against Wine 11.22 with the winum-11.22-3 series (`run.c`
`ME_GetRunSizeCommon` tab branch, `editor.c` `ME_MakeEditor` and
`WM_SETTEXT`, `richole.c`). Skimmed: the MEASURE program and the two
result files, the workflows and the L1 script.

Approved for the series, with the combined L4 below.

- The getter and setter match the measured rows the test covers: 720
  twips by default, rounding to the nearest twip (18.51 -> 18.5, 18.526
  -> 18.55, 0.01 -> 0), `E_INVALIDARG` for zero, negative values, NaN and
  a NULL output, the 0xffffff-twip limit only in RICHEDIT50W, the older
  class keeping 100,000,000 points and returning the overflow value for
  FLT_MAX. `GetDefaultTabStop` no longer leaves its output unwritten,
  which is what Revit's TextEngine tripped over.
- Layout is limited well: with the default 720 twips the old
  `lDefaultTab` loop runs unchanged, so programs that never call the
  setter see no difference. A stored 0 or the overflow value falls back
  to that loop too. The new branch snaps the interval to whole pixels
  once and respects zoom and the word-wrap width.
- `WM_SETTEXT` resets the value (measured); the rewrap after it comes
  with the new text.

Open points, not blockers:

1. The RICHEDIT50W limit uses `editor->msftedit`, which the SCF_WORD
   patch sets only for windowed RICHEDIT50W. A windowless msftedit text
   services object (Revit's TextEngine creates windowless ones) gets the
   older class's range check. Revit's value (35.4331) is far inside both
   ranges; the windowless limit was not measured.
2. Not measured, so unchanged: whether `EM_SETTEXTEX(ST_DEFAULT)`,
   `EM_STREAMIN` or RTF `\deftab` change or reset the stored value.
3. Nit: the test uses `ARRAYSIZE`; Wine's tests use `ARRAY_SIZE`.

Before `patches/series`: an Inventor and AutoCAD text editing smoke test
with the final DLL carrying all three riched20 changes (tab stop,
paragraph navigation, target device scroll), as the DLL is shared.

## Combined L4 (2026-11-26, evening)

Overlay with all four candidates built from one winum-11.22-3 tree
(tab stop -> paragraph navigation -> target device scroll, ole32 v2;
[run 45880891853](https://github.com/Harvi654/winum/actions/runs/45880891853/artifacts/15147073657),
SHA-256 of archive and DLLs verified), on copies of the prefixes:

- Inventor 2027: note text, dimension text, datum identifier editor
  (letter reachable by scrolling), drag and drop, open/save/reopen - ok.
- AutoCAD 2027: MTEXT (several lines, centred, reopened), TEXT,
  save/reopen - ok.
- Revit 2027.4, DirectX Software: New project opens "Podlaží 1". A wall
  drawn afterwards gives Revit's warning that none of the created
  elements is visible in the view - the drawing problem recorded in the
  Revit repository, not RichEdit or OLE.
