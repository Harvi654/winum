# win32u-font-smoothing.patch

<!-- winum-test: l1 -->

- **DLL:** `win32u` (`dlls/win32u/font.c`), test in `gdi32`
- **Status:** In the winum-11.22-3 series. [Candidate history](../../patches/candidates/win32u-font-smoothing/README.md)
- **Target:** winum-11.22-3

## Symptom

Text in Inventor looked blurry or without ClearType depending on the fontconfig setup (Inventor issue #13); diagnosis and the first probe by @krokosik (Inventor PR #18).

## Cause and fix

Wine 11.22 let fontconfig's per-face flags decide before the program's request: with `rgba=none` explicit `CLEARTYPE_QUALITY` and desktop ClearType were lost, with `rgba=rgb` subpixel rendering appeared although the desktop had smoothing off. Windows 10 (measured) draws explicit ClearType even with desktop smoothing off, and default, draft and proof quality follow the desktop setting. The patch resolves explicit ClearType first and the desktop setting for default quality, and uses fontconfig only where the desktop setting is not configured; driver overrides, explicit grayscale and monochrome are unchanged.

In plain words: fonts follow what the program and the Windows desktop setting ask for, not only the Linux font configuration.

## How it was found

@krokosik's DIB probe; expanded native matrix `docs/wine-repro/win32u-font-smoothing/` (408 native cases per quality and architecture).

## Wine test

`dlls/gdi32/tests/font.c`: glyph rendering per quality under the current settings, and an explicit `font smoothing` mode that walks all eight desktop settings and restores them.

## Validation

| Level | Result |
| --- | --- |
| L0 | Applies with `--fuzz=0` after the full series on Wine 11.22; built x86_64 and i386 in the combined next-release build [45485575952](https://github.com/Harvi654/winum/actions/runs/45485575952) (2026-11-24) and by "Build Wine" on the release branch |
| L1 | v2 on offline Windows 10: 397 checks per architecture, no failures, todos or skips (2026-11-24) |
| L2 | Paired v2 [45460436638](https://github.com/Harvi654/winum/actions/runs/45460436638); [Run 45485567767](https://github.com/Harvi654/winum/actions/runs/45485567767): the released 11.22-2 series against it plus the seven 11.22-3 patches, 452 units in five groups, both architectures; no new failure (details in `docs/wine-repro/next-release-1/README.md`) (`gdi32 font.c:8294` fixed) |
| L3 | 301 targeted checks under both fontconfig profiles, both architectures (v1) |
| L4 | Improved text in Inventor 2027 (maintainer, 2026-11-23); With the combined next-release build (maintainer, 2026-11-24/25): Inventor 2027.2 + Nastran prefix copy - part, assembly, drawing, open/save/save-as and reopen, Stress Analysis, Frame Analysis, Frame Generator, Nastran Generate Mesh / Run / results without the 0x80040154 dialog; AutoCAD 2027.1.1 fresh install from its Create Installer image (33 steps, exit 0), sign-in, open, SAVE, SAVEAS, reopen. SOLIDWORKS deferred under AGENTS.md |
| L5 | Pending |
| Cross review | Written by Codex; reviewed by Claude (2026-11-23), Draft/Proof measurement requested and added in v2 |

## Open

- Other grayscale rasterization differences and exact RGB/BGR pixel mirroring are outside the test.
- A check on a large 4K display (@krokosik) is pending.

## Notes for a Wine bug report

Wine applies fontconfig's antialias/rgba before `CLEARTYPE_QUALITY` and the desktop smoothing setting. Public test: the gdi32 test above.

## History

- v1 2026-11-23: candidate.
- v2 2026-11-24: Draft and Proof follow Default (measured).
- v3 2026-11-25: no code change; test moved into the patch; promoted to the series.
