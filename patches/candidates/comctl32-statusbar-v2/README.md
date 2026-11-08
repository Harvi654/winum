# comctl32 status-bar background v2 candidate

Original, AI-assisted (Codex), LGPL-2.1-or-later. Not submitted upstream.
Candidate only: **not in `patches/series`**. Claude's cross review and Windows
10 L1 passed (see below); paired L2 remains pending. No RX 6400 fixture work
is included.

## Problem and bounded change

[Windows MEASURE results](../../../docs/wine-repro/comctl32-statusbar-erase/README.md#windows-results-2026-11-07)
(`windows-2026-11-07/`, x86_64/i686, Windows 10) distinguish three cases:

| Control | WM_PAINT background | SB_SETTEXT requests erasure |
| --- | --- | --- |
| v5 | always painted, including non-erasing repaint | no |
| v6 without an active theme | always painted | yes |
| v6 with an active theme | only through WM_ERASEBKGND; application's erase fill survives | yes |

The current v1 applies erase-only painting and erasing invalidations to all
three. That matches the measured themed-v6 behavior but is wrong for v5 and
classic v6. This incremental correction applies **after the full 62-patch
series, including v1**; do not substitute it for v1 or apply it twice.

`STATUSBAR_HasTheme` checks both the compile-time control version and the
current per-window theme. `WM_PAINT` paints the background unconditionally for
classic controls and never paints it for themed v6. General v1 invalidations
and explicit erase handling are restricted to themed v6. `SB_SETTEXT` is the
measured exception: all v6 controls erase, v5 does not. Other setters were not
measured, so their unthemed behavior returns to vanilla instead of inventing
Windows expectations. Direct-HDC/WM_PRINTCLIENT painting and v1's themed
owner-draw pane suppression remain unchanged; they are not newly asserted.

This corrects v1's classic-control divergence. It does **not** claim to fix
Inventor's maximized-window/GPU composition overlap, which is a separate
investigation and already occurred with correct themed-v6 erase behavior.

## Ordered candidate patches

The local `series` is separate from the repository's production series:

1. `0001-status-background-tests.patch` adds the Wine test to
   `dlls/comctl32/tests/status.c`. `todo_wine` records vanilla Wine's measured
   v6 differences. This test-only stage can also be applied to pristine Wine
   11.20; the production series has no other changes to this test file.
2. `0002-status-background-v2.patch` corrects the implementation and removes
   only the todos it fixes. The full candidate therefore uses ordinary `ok()`
   for the corrected Windows behavior, rather than producing expected-todo
   successes under the corrected DLL.

The empty-string `SetWindowTheme` failure remains a separate todo. If a theme
is still active after the disable request, the test explicitly skips that
classic-v6 case instead of testing a themed control under a classic label.
A missing default v6 theme is also reported as missing themed coverage.

The focused test selects the custom-erase/empty-target rows from MEASURE:
three styles (normal, borderless, transparent owner draw), six operations
(`SB_SETTEXT`, invalidation with/without erase, RedrawWindow with/without erase,
and WM_PAINT without an update region). Its magenta marker is outside the text.
It checks marker preservation/removal, the dark custom fill versus the system
button-face fill, pending updates and erase-message presence. No font-specific
pixel count, glyph mask or unmeasured setter value is asserted. The expected
rules were checked against **all 108 corresponding native observations** across
both architectures (18 v5 + 36 v6 operations per architecture).

## Apply and build

Start with a separate clean Wine 11.20 source directory; do not patch an installed
runtime or a Wine prefix. Use existing checkouts, not a Git worktree, unless
explicitly requested. From the winum repository:

```sh
export WINUM_REPO="$(pwd)"
export WINE_SOURCE=/path/to/pristine/wine-11.20
export STATUSBAR_WORK="$(mktemp -d "$HOME/.cache/winum-statusbar-v2.XXXXXX")"
mkdir "$STATUSBAR_WORK/source" "$STATUSBAR_WORK/build"
cp -a "$WINE_SOURCE/." "$STATUSBAR_WORK/source/"
source "$WINUM_REPO/scripts/lib/wine-patches.sh"
wine_apply_patches "$STATUSBAR_WORK/source" "$WINUM_REPO/patches" "$WINUM_REPO" HEAD
while IFS= read -r name; do
    patch --batch --forward --fuzz=0 -p1 -d "$STATUSBAR_WORK/source" < "$WINUM_REPO/patches/candidates/comctl32-statusbar-v2/$name"
done < "$WINUM_REPO/patches/candidates/comctl32-statusbar-v2/series"
```

For the same compile-only validation used here (not a usable graphics runtime):

```sh
cd "$STATUSBAR_WORK/build"
"$STATUSBAR_WORK/source/configure" --enable-archs=x86_64,i386 --without-x --without-wayland --without-freetype
make -j8 dlls/comctl32/x86_64-windows/comctl32.dll dlls/comctl32/i386-windows/comctl32.dll dlls/comctl32_v6/x86_64-windows/comctl32_v6.dll dlls/comctl32_v6/i386-windows/comctl32_v6.dll
make -j8 dlls/comctl32/tests/x86_64-windows/comctl32_test.exe dlls/comctl32/tests/i386-windows/comctl32_test.exe
```

Requires Wine's host build tools (GCC/G++, make, Perl, flex, bison, m4) and both
MinGW toolchains. Use the normal full Wine feature profile for an eventual
runnable paired L2 build; the reduced configuration above only compiles the
changed DLL/test targets and must not replace a CAD runtime.

## Windows L1 bundle

The builder extracts the exact test block from `0001` into a small standalone
wrapper, retaining its Wine-only todos. The correction removes fixed todo
annotations, but its **native Windows assertions are identical**. Four EXEs
use embedded v5/v6 manifests, and verify the actual status class version through
the class procedure module. The builder does not patch the supplied Wine tree.

```sh
cd "$WINUM_REPO"
bash scripts/build-l1-comctl32-statusbar-v2.sh "$WINE_SOURCE" /path/to/NEW-statusbar-v2-L1-bundle
```

It requires both MinGW GCC/windres toolchains, Python 3 and Wine's `test.h` and
`debug.h`. Builds use `-Wall -Wextra -Werror`. Copy the whole bundle to a new
folder in the offline Windows 10 x64 VM and run `run-tests.bat`. Keep the desktop
visible/unlocked, at least 800x600, with themes enabled; do not cover or interact
with the measurement windows. The batch refuses existing result files.

Return the four bounded `result-<arch>-v<version>.txt` files and `exitcodes.txt`.
Acceptance requires all four exit codes 0, zero failures/skips, v5 coverage,
and v6 traces with `classic=0 themed=1` and `classic=1 themed=0` for every style.
A successful compile or zero-exit run with missing theme coverage is not L1.
No vendor DLLs, VM contents or program installers are needed in this bundle.

## Validation recorded 2026-11-07

- Source: Wine upstream Git `wine-11.20`, commit
  `7298a818b79446bae153b8d5aa27235ab3703bf9`. All 62 production patches applied
  with the real `wine_apply_patches` helper, then both candidate patches with
  `--fuzz=0`. The resulting implementation/test code matches the compiled code.
- All four comctl32 DLLs (v5/v6, x86_64/i386) linked successfully with the complete
  production patch set plus candidate in the reduced compile profile above.
- Both complete `comctl32_test.exe` architecture targets linked successfully,
  including the new test's v5/v6 activation path. No test execution is claimed.
- Four standalone L1 EXEs compiled with MinGW GCC 14.2, Wine 11.20 test headers,
  embedded manifests and `-Wall -Wextra -Werror`; bundle checksums verified.
- Native Windows expectation audit passed for all 108 selected MEASURE rows.
- No production patch, version pin or root `patches/series` was changed.

Not claimed: an official release-archive/full-feature Wine runtime L0 build,
a runtime pass under Wine, Windows L1, paired L2, CAD L4 or Claude's formal cross
review. The build input was the upstream tag checkout, not a checksum-verified
Wine release tarball. Those distinctions remain explicit when promoting v2.
An advisory second-assistant static review is separate from Claude's required
cross review; it found test include-order and initial counter issues, which
were corrected before the integrated suite build passed.

## L1 on Windows 10 (2026-11-07)

Bundle from `scripts/build-l1-comctl32-statusbar-v2.sh`, run in the
maintainer's offline Windows 10 x64 VM, themes on. Results in
`windows-2026-11-07-l1/`:

| EXE | Result |
| --- | --- |
| x86_64 v5 | 180 tests, 0 failures, 0 skipped |
| i686 v5 | 180 tests, 0 failures, 0 skipped |
| x86_64 v6 | 360 tests, 0 failures, 0 skipped |
| i686 v6 | 360 tests, 0 failures, 0 skipped |

All exit codes 0. Coverage complete: v5 three styles; v6 `classic=0
themed=1` and `classic=1 themed=0` for styles 0, 0x100 and 0x1100.
**L1 reached.** Claude's cross review: `docs/wine-repro/cross-review-2026-11-07-comctl32-statusbar-v2.md`
(approved; P2: the unthemed v6 path is not exercised under Wine).
Next: paired L2 of the comctl32 tests (series vs series + candidate), then
fold into `patches/comctl32-statusbar-erase-background.patch` as v2.

## Isolated graphics runtime (2026-11-08)

`scripts/build-statusbar-v2.sh [--prepare-only] NEW_OUTPUT` prepares a private
recipe containing the full production series followed by the two candidate
patches. It never changes the checkout's production series or version pin.
The Wine 11.20 public-mirror commit/archive SHA-256 are pinned and checked
before extraction. The regular full-feature builder produces the WoW64
runtime, named `winum-11.20-1-sbv2-x86_64`, and a corresponding source archive
from the actual compiled tree, each with SHA-256 sums. `SBV2-INPUTS` records
the upstream pin and checkout commit; `BUILDINFO` and `PATCHES.txt` identify
the complete patch set. This is a candidate artifact, not a release.

The separate **Build status-bar v2 candidate** workflow builds it on changes
to this directory or its builder/workflow, or by manual dispatch. Download
the `statusbar-v2-<commit>` artifact from that workflow, verify the binary
archive's `.sha256`, and extract to a new runtime directory. Use a new test
prefix; do not replace a working CAD runtime or copy individual DLLs into it.
Repeat the same MEASURE executables used with production v1, retaining both
results. V5 should clear on non-erasing repaints and not erase on SB_SETTEXT;
themed v6 should retain the Windows-derived erase-only background behaviour.
Failure to disable the theme still excludes classic-v6 coverage.

Local preparation on 2026-11-08 verified the pinned source checksum and applied
all 62 production patches plus both candidate patches with `--fuzz=0`.
Shell syntax checks passed, including the generated private recipe and its
candidate patch-date mapping. A full runtime build and its execution are
not claimed by this preparation check. The workflow uses the full normal
graphics build profile; it does not itself run paired L2 suites. A successful
maintainer MEASURE comparison is focused runtime evidence, not full L2 or CAD
L4 validation. Production-series promotion remains a separate step.
