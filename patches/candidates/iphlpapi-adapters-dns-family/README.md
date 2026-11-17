# Candidate: GetAdaptersAddresses with no DNS server of the family

## Problem

CodeMeter's service (ABB RobotStudio, Harvi654/ABB-RobotStudio-on-Linux,
steps 11-12) logs its local IPv4 and IPv6 addresses at every start on
Windows and none under Wine; RobotStudio then reaches it over IPv4,
CodeMeter logs no access and RobotStudio exits with 103. The service
calls `GetAdaptersAddresses(AF_INET6, 0)` twice at its start. Under Wine
that call fails with `DNS_ERROR_NO_DNS_SERVERS` (9852) on a host whose
resolver lists no IPv6 DNS server; Windows 10 returns the adapters
(MEASURE `docs/wine-repro/iphlpapi-adapters/`).

## Cause and fix

`dns_info_alloc()` (`dlls/iphlpapi/iphlpapi_main.c`) asks dnsapi for the
servers of the requested family per adapter and returns any error for
the whole call. The fix keeps the adapter's server list empty for
`DNS_ERROR_NO_DNS_SERVERS`; other errors fail as before.

## Patches (`series`)

1. `iphlpapi-tests-adapters-dns-family.patch` - `test_GetAdaptersAddresses_dns_family`
   (the same code as `docs/wine-repro/iphlpapi-adapters/test-body.h`):
   for `AF_UNSPEC`, `AF_INET`, `AF_INET6` the call returns the same as
   with `GAA_FLAG_SKIP_DNS_SERVER`, and the full call succeeds.
2. `iphlpapi-adapters-dns-family.patch` - the fix.

## State (2026-11-17)

- L0: both apply to winum-11.22-1's source with `--fuzz=0`; built
  locally (x86_64).
- Local run (x86_64, build sandbox without IPv6 DNS servers):
  without the fix the new test fails once (`family 23: got 9852,
  without DNS servers 111`), with it passes; the rest of `iphlpapi`
  shows the same failing lines with and without (66 pre-existing in
  this sandbox). The native L1 harness under the fixed build:
  `checks=21 failures=0`.
- L1 (Windows 10, `scripts/build-l1-iphlpapi-adapters-dns-family.sh`):
  pending.
- L2 paired run in CI: pending. L4 in RobotStudio (does CodeMeter log
  "Found ... address", and RobotStudio's access?): pending.
- Cross review: pending.
