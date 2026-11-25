# msado15-stream-charset.patch

<!-- winum-test: l1 -->

- **DLL:** `msado15` (`dlls/msado15/stream.c`), test in `msado15`
- **Status:** In the winum-11.22-3 series. [Candidate history](../../patches/candidates/msado15-stream-charset/README.md)
- **Target:** winum-11.22-3

## Symptom

After the wbemdisp fix, Nastran's `addin.vbs` still wrote its `.addin` without the installation path (`<Assembly>NEiInventor.dll</Assembly>`), because `ADODB.Stream` refused UTF-8 text.

## Cause and fix

Wine's `ADODB.Stream` accepted only Unicode text and returned E_NOTIMPL for `SaveToFile`, `LoadFromFile` and `CopyTo`. The patch handles the measured charsets (both UTF-8 spellings, us-ascii, windows-1252, iso-8859-1) with the Windows code-page APIs: the UTF-8 BOM, byte positions, bounded and line reads (including how Windows advances over a split surrogate), the file methods and `CopyTo` text -> binary; the Unicode path is unchanged.

In plain words: scripts can now read and write UTF-8 text files through `ADODB.Stream`, as Nastran's registration does.

## How it was found

`WINEDEBUG=fixme+msado15` in the registration; native C and VBScript probe `docs/wine-repro/msado15-stream-charset/`.

## Wine test

`dlls/msado15/tests/stream_charset.c`: writes, saved bytes, reads by count and line, loads with and without BOM, truncation, `CopyTo` both ways, for the five charsets (335 checks).

## Validation

| Level | Result |
| --- | --- |
| L0 | Applies with `--fuzz=0` after the full series on Wine 11.22; built x86_64 and i386 in the combined next-release build [45485575952](https://github.com/Harvi654/winum/actions/runs/45485575952) (2026-11-24) and by "Build Wine" on the release branch |
| L1 | v2 on offline Windows 10: 335 checks per architecture, no failures, todos or skips (2026-11-24) |
| L2 | Paired v2 [45469035401](https://github.com/Harvi654/winum/actions/runs/45469035401); [Run 45485567767](https://github.com/Harvi654/winum/actions/runs/45485567767): the released 11.22-2 series against it plus the seven 11.22-3 patches, 452 units in five groups, both architectures; no new failure (details in `docs/wine-repro/next-release-1/README.md`) (`stream_charset` 335 checks, 0 failures) |
| L3 | C and VBScript results match the Windows reference for the changed charsets |
| L4 | Nastran's registration writes the full path; With the combined next-release build (maintainer, 2026-11-24/25): Inventor 2027.2 + Nastran prefix copy - part, assembly, drawing, open/save/save-as and reopen, Stress Analysis, Frame Analysis, Frame Generator, Nastran Generate Mesh / Run / results without the 0x80040154 dialog; AutoCAD 2027.1.1 fresh install from its Create Installer image (33 steps, exit 0), sign-in, open, SAVE, SAVEAS, reopen. SOLIDWORKS deferred under AGENTS.md |
| L5 | Pending |
| Cross review | Written by Codex; read in full by Claude (2026-11-24): three required changes (test link, todos, TRACE newlines), all done in v2 |

## Open

- Other ADO charset names, text -> text `CopyTo` and Unicode -> Unicode charset changes at a nonzero position (Windows refuses) are not covered.
- Reading a large stream line by line decodes the rest of the stream each time.

## Notes for a Wine bug report

`ADODB.Stream` with `Charset = "utf-8"` fails in Wine (E_NOTIMPL / unsupported charset). Public test: the msado15 test above.

## History

- v1 2026-11-23: candidate.
- v2 2026-11-24: review fixes, test in the patch.
- v3 2026-11-25: no change; promoted to the series.
