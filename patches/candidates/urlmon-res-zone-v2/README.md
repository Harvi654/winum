# urlmon res: zone v2 (candidate)

Replaces `patches/urlmon-res-zone-local-machine.patch` (same file name,
so it can be swapped in on a test branch). Its test is the one from
`patches/tests-first/urlmon-tests-res-zones.patch`, with `todo_wine` on
the one URL the fix does not cover (a res: URL of an existing module);
when this candidate goes into the series, that tests-first entry leaves
`patches/tests-first/series`.

What Wine answered, from L2 run 42569334513 (`test-run/tests-first-2`):

| URL | Windows | vanilla Wine | series (v1) |
| --- | --- | --- | --- |
| `res:///btn.xmx/icon.png` | 3 | -1 | 0 |
| `res://shell32.dll/#2/#130` | 0, file: security URL | 3, res: URL | 0, res: URL |
| `res://notexisting.dll/x.png` | 3 | 3 | 0 |
| `res:`, `res:x` | -1 (S_OK) | -1 | 0 |

v1 matched Windows only for the module case, by accident. v2 matches
Windows for every URL except the module mapping. To check: L2 against
vanilla, L1 of the test (urlmon:sec_mgr is in the L1 bundle), then
AutoCAD on the `autocad` branch (WPF must accept zone 3 for the icons).
Cross review by the other assistant before the series.

## L2: test-run/all-1 (2026-11-10)

[docs/wine-repro/l2-all-1-2026-11-10](../../../docs/wine-repro/l2-all-1-2026-11-10/README.md), run 42788810738, with every current candidate: no new failure in the touched DLLs, both architectures.
