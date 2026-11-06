# Candidate: oleaut32 type library cache

`oleaut32-typelib-keep-released.patch`: a type library read from a file
stays in oleaut32's cache after its last Release (up to 8), holding no
references, and is used again if its file did not change. Made for
Inventor's 3D PDF export: ~7 minutes before, 15 s with it. Background,
Windows measurements and results: [docs/wine-repro/oleaut32-typelib-cache/](../wine-repro/oleaut32-typelib-cache/README.md).

Moved here from Autodesk-Inventor-for-Linux `patches/drafts/` (last change
there `b2bce4e`, results `34d626a`), unchanged.

## State (2026-11-06)

Cross review 2026-11-06 (Codex,
[cross-review-2026-11-06-oleaut32.md](../wine-repro/cross-review-2026-11-06-oleaut32.md)):
two P2 findings, both addressed in **v2** (same day):

- P2-1, unsaved edits survived the last Release: a library that was asked
  for `ICreateTypeLib(2)`, or whose type info was asked for
  `ICreateTypeInfo(2)`, is no longer kept. The test now edits the name of a
  loaded library and checks that the next load sees the file's name.
- P2-2, a file replaced while it was read got the new file's stamp: size
  and time are taken before opening and after closing; the library is kept
  only if both agree.

v2 L1 (2026-11-06, `docs/wine-repro/oleaut32-typelib-cache/windows-2026-11-06-v2/`):
33 tests, 0 failures on x86_64 and i686. Windows offers `ICreateTypeLib2`
on a loaded library (S_OK), `SetName` returns S_OK, and the next load has
the file's name again - the behaviour v2 keeps. The two traces became
`ok()` checks with these measured values afterwards; that final test
text runs on Windows again with the next bundle.

Re-review of v2.1 (Codex, same day, section "v2 re-review" of the review
document): builds on both architectures, both original P2 reproducers
fixed, x64 `typelib` 7733/0 and `tmarshal` 754/0 paired. One new P2: two
references asking for the creator interfaces at the same time wrote the
flag without synchronisation. **v2.2** clears it with
`InterlockedExchange` (field now `LONG`).

Codex's focused check of v2.2 (same day, section "v2.2 check"): race
closed, both architectures build, x64 pairs unchanged - **cross review
done**.

Final v2.2 test text on Windows 10 (2026-11-06,
`docs/wine-repro/oleaut32-typelib-cache/windows-2026-11-06-v2.2/`): 35
checks, 0 failures on x86_64 and i686.

Still open for v2.2: i686 L2, full L0, SOLIDWORKS L4.

| Level | Result |
| --- | --- |
| L0 | applies without fuzz on Wine 11.20 + winum `series` (incl. `oleaut32-typedesc-pointee` v2); full build pending |
| L1 | v2.2 passed 2026-11-06 (35/0, both arch.); v2 33/0; v1 passed 2026-11-06, Windows 10 x86_64 and i686: 26 tests, 0 failures each (`docs/wine-repro/oleaut32-typelib-cache/windows-2026-11-06/`). Windows also lets the file be deleted right after the last Release. |
| L2 | Wine 11.20 x86_64: `typelib` 7724 tests / 0 failures, `tmarshal` 754 / 0 (2026-11-03); i686 open |
| L3 | `measure.c`: load after Release to 0 0.09 ms (was ~20 ms) |
| L4 | Inventor 2027.2 3D PDF: part 15 s, assembly 20 s, PRC model present (2026-11-03); SOLIDWORKS open |
| cross review | v2.2 OK (Codex, 2026-11-06, after two P2 in v1 and one in v2.1) |

The test passes on vanilla Wine too: it guards correctness (refcounts,
reloading, a changed file is read anew); the speed is shown by the MEASURE
program. On Windows it also tells whether Windows keeps the file open
after the last Release (the `DeleteFileA` checks).

## How to build the L1 bundle

On the maintainer's machine (needs `mingw-w64-gcc` and any Wine 11.20
source tree for `include/wine/test.h`):

```bash
export WINE_SRC="$HOME/path/to/wine-11.20-source"   # a tree or an unpacked source archive
bash scripts/build-l1-oleaut32-cache.sh "$WINE_SRC" ~/vm-share/oleaut32-cache-l1
```

Then in the Windows 10 VM: run `run-tests.bat`, return the three text
files.

## How to resume

If L1 passes on both architectures: cross review, then into `series` with a
page in `docs/patches/`. If a check fails on Windows, the test is wrong:
correct the test from what Windows returned, then check the patch again.
