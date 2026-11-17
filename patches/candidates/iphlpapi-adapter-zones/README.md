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
- L1 (Windows 10, `scripts/build-l1-iphlpapi-adapter-zones.sh`):
  pending. L2 paired CI: pending. L4 (CodeMeter's "Found ... address",
  RobotStudio): pending, with a build of all candidates.
- Cross review: pending.
