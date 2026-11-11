# urlmon-maptozone-null-terminate.patch

<!-- winum-test: missing -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `urlmon`
- **Origin:** Autodesk-Inventor-for-Linux, commit `c1b4e90`, first shipped in `wine-11.20-5`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

AutoCAD 2027 (experimental branch): ribbon icons failed at random (`SecurityException`).

## Cause and fix

`map_url_to_zone` copies the URL with its terminating null before `CreateUri`.

## Wine test

None in the patch.

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.20-5` (2026-10-21, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06) |
| L1 | **no Wine test** - needs one, measured on Windows first |
| L2 | not recorded per patch |
| L3 | not recorded |
| L4 | AutoCAD 2027 starts (experimental); no Inventor feature depends on it. (Inventor, maintainer) |
| L5 | in Inventor's runtime since `wine-11.20-5` (2026-10-21); a week of use not formally recorded - maintainer to confirm |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

AutoCAD (decision 2026-11-09): AutoCAD is not installed and is not being worked on now. The patch stays in the series as it is; it gets its Wine test (L1, L2) like every other patch, and the AutoCAD smoke test (L4) is deferred until there is time for AutoCAD again.

Windows values measured 2026-11-09 (`docs/wine-repro/urlmon-zones/`); test first in `patches/tests-first/urlmon-tests-res-zones.patch` (URLs `x`, `""`, `unknownscheme:` that reach the copy path in Wine).

Wine test missing; added for AutoCAD, not Inventor.

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).


## Check of a test through the res-zone test (2026-11-11)

Question (maintainer): do the checks of the res-zone test that reach
this copy - URLs `x`, `""`, `unknownscheme:abc`, `unknownscheme://host/path`
in `urlmon-res-zone-v2`'s `sec_mgr.c` test - fail without this patch?

Local run, Wine 11.20 x86_64, `urlmon:sec_mgr` with the res-zone v2
test: once with this patch and res-zone v2, once with res-zone v2 only.
Both 854 checks with the same two results (two "succeeded inside todo"
for `res://shell32.dll/#2/#130`, unrelated); also identical with
`WINEDEBUG=warn+heap` (heap tail checking). A `+urlmon` trace shows
`map_url_to_zone` does reach the copy for those URLs
(`CoInternetGetSecurityUrl` fails, then `CreateUri` on the copy).

So the checks pass without the fix: the missing terminator makes
`CreateUri` read past the copied string, but for these URLs the zone
and the result come out the same (the scheme decides, and what follows
in memory does not change it). The fix stops an out-of-bounds read; its
effect is not visible through any API result, so a Windows test cannot
show it.

Proposed marker (maintainer's decision): `l3-only: the fix stops an
out-of-bounds read of the copied URL in map_url_to_zone; zone and
results are the same with and without it, so no API-visible behaviour
can be tested on Windows; reproducer: the runs above
(docs/patches/urlmon-maptozone-null-terminate.md)`.

Commands (from a Wine 11.20 build tree with the series):

```sh
WINEDEBUG=warn+heap ./wine dlls/urlmon/tests/x86_64-windows/urlmon_test.exe sec_mgr
WINEDEBUG=+urlmon ./wine dlls/urlmon/tests/x86_64-windows/urlmon_test.exe sec_mgr 2>&1 | grep -A6 'MapUrlToZone.*unknownscheme'
```
