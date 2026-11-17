# Candidate: zone indices and the IPv6 interface index of adapters

Applies after `../iphlpapi-adapters-dns-family/` (same file).

## Problem

Second step for CodeMeter's service (ABB RobotStudio), which lists no
local address under Wine (Harvi654/ABB-RobotStudio-on-Linux, step 12).
After the DNS-family fix `GetAdaptersAddresses(AF_INET6)` succeeds, but
CodeMeter still logs no address. MEASURE format 2
(`docs/wine-repro/iphlpapi-adapters/`): Windows fills `Ipv6IfIndex` and
`ZoneIndices` (in `GetAdaptersAddresses()` and `GetIpInterfaceEntry()`),
Wine returns 0 for all of them. A caller that uses `::1` or a
link-local address with a scope needs them.

## Cause and fix

`nsiproxy.sys` never set `zone_indices` of the IP interface table
(`ip_interface_fill()`), and `GetAdaptersAddresses()` sets neither field.

1. `iphlpapi-tests-adapter-zones.patch` - `test_GetAdaptersAddresses_zones`
   (= `docs/wine-repro/iphlpapi-adapters/test-body-zones.h`): for every
   adapter with an IP interface, all 16 zones in both functions and both
   families, and `Ipv6IfIndex`, as measured.
2. `nsiproxy-ip-interface-zones.patch` - the measured zones in the IP
   interface table (interface index up to the subnet level, 0 global,
   1 otherwise).
3. `iphlpapi-adapter-zones.patch` - `GetAdaptersAddresses()` takes
   `Ipv6IfIndex` and `ZoneIndices` from the IPv6/IPv4 interface tables,
   whatever family was asked for.

## State (2026-11-17)

- L0: the dns-family series and this series apply to winum-11.22-1's
  source with `--fuzz=0`; built locally (x86_64).
- Local (build sandbox, no IPv6): without the fix the new test fails 60
  times, with it passes; `iphlpapi` keeps its 66 pre-existing sandbox
  failures, `nsi` its 8 (no IPv6), with and without. Native L1 harness
  under the fixed build: `checks=68 failures=0`.
- **L1 passed** on Windows 10 22H2 (19045), 2026-11-17:
  `checks=100 failures=0 skipped=0`, exit 0, x86_64 and i686
  (`docs/wine-repro/iphlpapi-adapters/l1-zones-windows10-19045-2026-11-17/`;
  two adapters with both families: 2 x (16 + 16 + 1 + 16) + 2 checks).
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
