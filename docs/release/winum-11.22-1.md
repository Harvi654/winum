# winum-11.22-1

Wine 11.22, `WINUM_BUILD=1`: the 66 patches of winum-11.20-2 moved to
Wine 11.22, plus what `main` got since that release (v6 of
`ole32-createfromfile-package`, winum PR #2). The series is otherwise
unchanged; only the test hunk of `msxml3-domdoc-importnode` was rebased
(v2, context only).

What Wine 11.22 brings that matters here: the ECDSA private key import in
ncrypt (Wine bug 60443, needed by Revit 2027), Tahoma glyph metric fixes,
the msxml3 DTD rewrite - and address space randomization, the main risk
for the CAD programs, which the L4 below looked at first. Gecko and Mono
versions are the same as for 11.20. `WINE_SHA256` is from the maintainer's
download of `wine-11.22.tar.xz`, whose `.sign` verified with Alexandre
Julliard's key on 2026-11-15.

## Validation

| Level | Result | Date |
| --- | --- | --- |
| L0 | all 66 patches on the official 11.22 archive with `--fuzz=0`, both architectures (Build Wine [run 43703211594](https://github.com/Harvi654/winum/actions/runs/43703211594), `3c54dbd`) | 2026-11-15 |
| L1 | unchanged: the patches and their tests are those of winum-11.20-2 (one test hunk rebased, the test itself the same); Windows 10 did not change | - |
| L2 | `test-run/wine-11.22-1`: [run 43703213332](https://github.com/Harvi654/winum/actions/runs/43703213332), group g3 re-run [43739605851](https://github.com/Harvi654/winum/actions/runs/43739605851), paired against vanilla Wine 11.22: only known flaky failures - ntdll threadpool, ntdll time, urlmon `url.c:3535` (not repeated in the re-run). The workflow reports "failure" because of these | 2026-11-15 |
| L4 | Inventor 2027 on a copy of the maintainer's prefix: import, parts, assemblies (maintainer). SOLIDWORKS and AutoCAD: deferred - not being worked on since 2026-11-09 (AGENTS.md) | 2026-11-15 |

Each patch page in `docs/patches/` has the same in a "winum-11.22-1 (Wine
11.22)" table under its validation. The L4 is a smoke test of the build,
not a retest of every patch's own feature.

`scripts/check-series.sh`: 66 entries with L1 (56), an L3-only reason (9)
or a known deviation (1), 0 without.

## Open after the release

- Autodesk-Inventor-for-Linux stays on winum-11.20-2 until its release
  v0.4; then it pins winum-11.22-1 (`wine/version.env`,
  `wine/SHA256SUMS`, the `winum` flake input) in one commit, after this
  release exists.
- Revit 2027 (not public): the ncrypt ECDSA import of 11.22 in use.
- The candidate `wldap32-winsock-cleanup` (L0 and L2 done, L1 and cross
  review open).
