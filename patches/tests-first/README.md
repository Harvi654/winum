# Tests first

Wine tests whose expected values were measured on Windows, before the fix
that makes them pass exists (or before an existing patch is corrected).
`scripts/test-series.sh` applies the files listed in `series` here to both
test variants (vanilla and series, after `patches/series`), so the L2
report shows which checks fail on each side. The L1 bundle then carries
the test to the Windows VM, which confirms the test itself.

They are never part of a normal build (`scripts/build-wine.sh` reads only
`patches/series`). When the fix is written, the test becomes part of the
fix's patch, with `todo_wine` where the fix does not reach Windows yet,
and its line leaves `series` here; the file moves to `docs/archive/`.

| Patch | For | Windows values |
| --- | --- | --- |
| `urlmon-tests-res-zones.patch` | `urlmon-res-zone-local-machine`, `urlmon-maptozone-null-terminate` | `docs/wine-repro/urlmon-zones/windows-2026-11-09` |
| `advapi32-tests-token-integrity-level.patch` | moved into `ntdll-token-integrity-level` (winum-11.22-3) | `docs/wine-repro/server-token-integrity/windows-2026-11-24` |
| `combase-tests-per-user-classes.patch` | moved into `combase-per-user-classes` (winum-11.22-3) | `docs/wine-repro/combase-per-user-classes` |
| `advapi32-tests-hkcr-class-reads.patch` | moved into `kernelbase-hkcr-class-reads` (winum-11.22-3) | `docs/wine-repro/kernelbase-hkcr-read` |
| `wbemdisp-tests-method-invoke.patch` | moved into `wbemdisp-method-invoke` (winum-11.22-3) | `docs/wine-repro/wbemdisp-method-invoke` |
| `msado15-tests-stream-charset.patch` | moved into `msado15-stream-charset` (winum-11.22-3) | `docs/wine-repro/msado15-stream-charset` |
| `win32u-tests-font-smoothing.patch` | moved into `win32u-font-smoothing` (winum-11.22-3) | `docs/wine-repro/win32u-font-smoothing` |
