# win32u font smoothing

Credit @krokosik for the diagnosis and original MEASURE probe in Inventor
[#13](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/issues/13) and
[PR #18](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/pull/18).
See the [full comparison and native L1](../../../docs/wine-repro/win32u-font-smoothing/README.md).

With fontconfig rgba=none, Wine 11.22 let face flags mask explicit
ClearType quality and desktop ClearType. The opposite occurred with
rgba=rgb when desktop smoothing was off. Windows 10 offline measurement
showed explicit ClearType works with desktop smoothing off, and default
quality follows the desktop setting. Fontconfig still supplies fallback
smoothing when desktop smoothing is unconfigured, and subpixel order
when desktop orientation is unconfigured. The LCD filter remains in
FreeType. The candidate does not address other greyscale rasterization
differences, and exact RGB/BGR pixel mirroring is outside the test.

Apply the [test already verified on Windows](../../tests-first/win32u-tests-font-smoothing.patch)
and the runtime patch after the normal series with `--fuzz=0`.
The test's explicit `gdi32_test.exe font smoothing` mode covers all eight
settings and restores the originals. Its normal `font` unit uses current
settings.

| Level | Status |
|---|---|
| L0 | 2026-11-23, full WoW64 build passed. The source job applied the release series and candidate with zero fuzz. [Build and artifacts](https://github.com/Harvi654/winum/actions/runs/45209934855) |
| L1 | 2026-11-23 offline Windows 10, x86_64 and i686 each 301 checks, no failures, todos or skips, both exit 0. 2026-11-24 expanded v2 passed 397 checks per architecture, both exit 0 with no todos, flaky checks, failures or skips. [v2 results](../../../docs/wine-repro/win32u-font-smoothing/results/l1-windows-10-v2/summary.tsv). [Verified v2 bundle](https://github.com/Harvi654/winum/actions/runs/45460314173/artifacts/14952323004) |
| L2 | v1 passed all 301 targeted candidate checks under both fontconfig profiles and architectures. The v2 release-baseline comparison is running. [Paired v2](https://github.com/Harvi654/winum/actions/runs/45460436638) |
| L4 Inventor 2027 | 2026-11-23, maintainer reports improved text in an initial check. @krokosik has the build for the pending check on a larger 4K display. |
| L4 AutoCAD | Deferred in this validation round. |
| Cross review | Claude reviewed on 2026-11-23. The requested DRAFT_QUALITY and PROOF_QUALITY native measurement matches Default. v2 updates the expected values and runtime. Expanded v2 L1 passed on 2026-11-24. |


The v1 baseline g2 job rejected the expected failure lines when winetest
inserted the quality and smoothing context. The v2 paired runner accepts
that context and tests both sides against the release series.

[Inventor L4 instructions](../../../docs/wine-repro/win32u-font-smoothing/README.md#inventor-2027-l4)
use the [CI binary](https://github.com/Harvi654/winum/actions/runs/45209934855/artifacts/14839823850).
SHA-256 of `winum-11.22-2-x86_64.tar.xz`:
`35579316ea9f0a1c81ba5126264c517fbf3074837bf9e3fa49e6daab45ec88e9`.

## Draft and Proof follow-up

Both offline Windows 10 runs completed 2,880 cases across eight
smoothing settings and restored the original settings. The reports
differ only in the architecture header. Across the three font faces
and 8–24 px heights, all 408 Draft cases and all 408 Proof cases per
architecture match the corresponding Default rows, including pixel
counts and RGB/BGR checksums. Each quality produced 256 bi-level,
50 greyscale and 102 subpixel cases.

The v2 regression test requests both qualities with Tahoma at 12 px
under all eight settings. The runtime applies the configured desktop
policy to both, including when smoothing is disabled. The expanded test passed native L1 on both architectures.

## Cross review (Claude Opus 5.5, 2026-11-23)

Read in full: the runtime patch (`win32u-font-smoothing.patch`, 97 lines)
against `dlls/win32u/font.c` and `freetype.c` of Wine 11.22, and the test
(`patches/tests-first/win32u-tests-font-smoothing.patch`). Skimmed: the
MEASURE program and the result matrices.

**Approved for L4**, with these notes:

1. **Scope is right and safe for existing prefixes.** `font_smoothing` and
   `subpixel_orientation` now start at 0 ("not configured"), and the
   registry is applied only when `FontSmoothing` or `FontSmoothingType`
   exists. Wine's `wine.inf` writes neither (its `SPI_GET*` defaults live in
   `sysparams.c` only), and neither the Inventor scripts nor the flake set
   them today, so a prefix without these values keeps fontconfig's choice
   as before. The visible change needs the desktop values; for the L4 they
   are set by hand, for users later by a migration (Inventor plan, "Fonts").
2. **Explicit ClearType** now reaches FreeType as a subpixel mode even when
   the face's fontconfig flags say grey or none; `freetype_get_aa_flags`
   lets subpixel modes skip the gasp check (only grey is cut by
   `GASP_DOGRAY`), which matches the measured coloured 12 px Tahoma and
   krokosik's finding. Without LCD support in FreeType it still falls back
   to grey there - fine.
3. `ANTIALIASED_QUALITY` / `NONANTIALIASED_QUALITY` are set earlier in
   `font_SelectFont` and do not reach the changed branch - unchanged, as the
   test expects.
4. **Not measured: `DRAFT_QUALITY` and `PROOF_QUALITY`.** With desktop
   smoothing configured, the patch applies it to them only when the face has
   no fontconfig flags; otherwise they keep fontconfig's choice as before.
   Add both to the MEASURE program (one more Windows run) before promotion,
   and make the branch follow the measurement; until then the conservative
   behaviour is acceptable.
5. On promotion: move the test into the patch (as `ucrtbase` v3), write
   `docs/patches/win32u-font-smoothing.md` with the `l1` marker, and keep
   the credit line for @krokosik in the header and on the page.
6. L2 is open until the paired gdi32/win32u reference finishes; L4 by the
   maintainer (ribbon, menus, status bar zoomed 8x, before/after with the
   same registry values; `fc-match` there reports no `rgba`, the case the
   patch addresses).
