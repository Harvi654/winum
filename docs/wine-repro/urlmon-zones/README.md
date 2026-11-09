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
