# urlmon zones (MEASURE)

For two patches of batch 1 (`docs/patches/test-plan-2026-11.md`):

- `urlmon-res-zone-local-machine.patch` - a `res:///` URL without a module
  gets zone 0 (`URLZONE_LOCAL_MACHINE`) instead of `URLZONE_INVALID`.
- `urlmon-maptozone-null-terminate.patch` - when `CoInternetGetSecurityUrl`
  fails, urlmon copies the URL without its terminating null.

`measure.c` prints, for a list of URLs, the result and zone of
`MapUrlToZone` and the result of `CoInternetGetSecurityUrl`. The values the
Wine tests expect are taken from its Windows output (x86_64 and i686), which
goes to `windows-<date>/`. The output contains only the fixed URLs of the
list, no machine data.

Built by the "Build MEASURE bundle" workflow (name `urlzones`).

## Windows result (2026-11-09, Windows 10, x86_64 and i686 identical)

`windows-2026-11-09/result-*.txt`. `MapUrlToZone`:

| URL | hr | zone |
| --- | --- | --- |
| `res:///btn.xmx/icon.png` | S_OK | 3 (Internet) |
| `res:///%prodname%btn.xmx/RIBBON/icon.png` | E_INVALIDARG | -1 |
| `res://shell32.dll/#2/#130` | S_OK | 0 (security URL `file://` + the DLL path) |
| `res://notexisting.dll/x.png` | S_OK | 3 |
| `res:`, `res:x` | S_OK | -1 |
| `unknownscheme:abc`, `unknownscheme://host/path`, `about:blank` | S_OK | 3 |
| `x`, `""` | E_INVALIDARG | -1 |
| `file:///C:/Windows/notepad.exe` | S_OK | 0 |
| `http://localhost/` | S_OK | 1 |
| `http://www.example.com/` | S_OK | 3 |

`CoInternetGetSecurityUrl` returned S_OK for every URL, the URL itself
except for the existing module.

What it means for the patches:

- `urlmon-res-zone-local-machine.patch` gives a `res:///` URL without a
  module zone 0; Windows gives 3. The patch fixed AutoCAD (WPF refused
  zone -1), but its value is not the Windows one, and the URL from the
  AutoCAD log, with `%prodname%` written literally, fails on Windows
  too - so on Windows AutoCAD most likely passes a different URL. The
  patch is to be corrected to zone 3 and checked again in AutoCAD.
- `urlmon-maptozone-null-terminate.patch`: Windows never takes the
  "no security URL" path for these URLs; the test checks the results
  that Wine reaches through that path (`x`, `""`, `unknownscheme:`).

The Wine test: `patches/tests-first/urlmon-tests-res-zones.patch`.
