# Candidate: oleaut32 type library cache

`oleaut32-typelib-keep-released.patch`: a type library read from a file
stays in oleaut32's cache after its last Release (up to 8), holding no
references, and is used again if its file did not change. Made for
Inventor's 3D PDF export: ~7 minutes before, 15 s with it. Background,
Windows measurements and results: [docs/wine-repro/oleaut32-typelib-cache/](../../../docs/wine-repro/oleaut32-typelib-cache/README.md).

Moved here from Autodesk-Inventor-for-Linux `patches/drafts/` (last change
there `b2bce4e`, results `34d626a`), unchanged.

## State (2026-11-06)

| Level | Result |
| --- | --- |
| L0 | applies without fuzz on Wine 11.20 + winum `series` (incl. `oleaut32-typedesc-pointee` v2); full build pending |
| L1 | open: bundle from `scripts/build-l1-oleaut32-cache.sh` |
| L2 | Wine 11.20 x86_64: `typelib` 7724 tests / 0 failures, `tmarshal` 754 / 0 (2026-11-03); i686 open |
| L3 | `measure.c`: load after Release to 0 0.09 ms (was ~20 ms) |
| L4 | Inventor 2027.2 3D PDF: part 15 s, assembly 20 s, PRC model present (2026-11-03); SOLIDWORKS open |
| cross review | open (written by Claude, so Codex reviews) |

The test passes on vanilla Wine too: it guards correctness (refcounts,
reloading, a changed file is read anew); the speed is shown by the MEASURE
program. On Windows it also tells whether Windows keeps the file open
after the last Release (the `DeleteFileA` checks).

## How to build the L1 bundle

On the maintainer's machine (needs `mingw-w64-gcc` and any Wine 11.20
source tree for `include/wine/test.h`):

```bash
export WINE_SRC="$HOME/path/to/wine-11.20-source"
bash scripts/build-l1-oleaut32-cache.sh "$WINE_SRC" ~/vm-share/oleaut32-cache-l1
```

Then in the Windows 10 VM: run `run-tests.bat`, return the three text
files.

## How to resume

If L1 passes on both architectures: cross review, then into `series` with a
page in `docs/patches/`. If a check fails on Windows, the test is wrong:
correct the test from what Windows returned, then check the patch again.
