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
- **L1 passed** on Windows 10 22H2 (19045), 2026-11-17:
  `checks=15 failures=0 skipped=0`, exit 0, x86_64 and i686
  (`docs/wine-repro/iphlpapi-adapters/l1-windows10-19045-2026-11-17/`).
  15 checks there against 21 under Wine because the VM has two adapters
  and the Linux machine four (one length check per adapter and family).
- L4 approximation (2026-11-17, maintainer): the same condition reached
  from the host side (`nameserver ::1` in `/etc/resolv.conf` for one
  run) makes `GetAdaptersAddresses(AF_INET6, 0)` succeed, but CodeMeter
  still logs no "Found ... address" and no access, and RobotStudio still
  exits with 103. The fix corrects a measured difference but does not,
  alone, fix the program: it stays a candidate (AGENTS.md). Next
  differences from the MEASURE table: `Ipv6IfIndex` and `ZoneIndices`,
  then the loopback addresses' origins and lifetimes.
- **L2 paired CI run** (winum `test-run/robotstudio-codemeter`, run
  44113630473, commit 1cbd224, both architectures, with the AF_UNIX
  candidate): the test-only build fails `iphlpapi.c` 4174 (DNS family)
  and 4222, 4227, 4232 (zones) in addition to the lines that fail in CI
  anyway (3725-3913, network of the CI container); the build with all
  candidates fails only those pre-existing lines. `nsi`, `ws2_32`,
  `ntdll file`, `kernel32 file`, `advapi32 security`: no new failure.
- **L4** (maintainer, 2026-11-17, `winum-11.22-1-robotstudio-x86_64`
  from that run, prefix `try-11-winmgmt`): CodeMeter still lists no
  address at a normal prefix start. A relay trace showed why: its first
  `GetAdaptersAddresses()` fails with `ERROR_FILE_NOT_FOUND` because the
  NSI device does not exist yet - Wine's `services.exe` starts the
  `nsiproxy` driver and CodeMeter (both automatic) in no defined order
  (load order groups are ignored). With CodeMeter's start delayed
  (`DelayedAutostart`) it lists its IPv4 and IPv6 addresses as on
  Windows. RobotStudio still exits with 103: the Windows CodeMeter
  service under Wine answers local clients with error 302 (see
  Harvi654/ABB-RobotStudio-on-Linux, steps 13-16). So this candidate
  corrects a measured difference but does not make the program work; it
  stays a candidate.
- Cross review: pending.
