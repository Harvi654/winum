# MEASURE: GetAdaptersAddresses and GetIpAddrTable as a service sees them

Original AI-assisted (Claude), LGPL-2.1-or-later. A public-API
observation program, not a Wine test: it prints what the functions
return and judges nothing. Test first: the Windows output becomes the
expected values of a Wine test if a difference matters.

## Why

ABB RobotStudio 2026 (Harvi654/ABB-RobotStudio-on-Linux,
`docs/robotstudio-2026-first-try.md`, steps 11-12) uses Wibu-Systems'
CodeMeter runtime 9.10. On Windows 10 its service logs at every start,
also after `net stop`/`net start` without a reboot:

```
Found IPv4 address: 127.0.0.1 | <lan>
Found IPv6 address: ::1 | fe80::...%13
```

and RobotStudio's later access is logged as "Access from local(IPv6)"
(answered with WB201, then RobotStudio opens its licence dialog). Under
Wine (winum-11.22-1 with the AF_UNIX candidate) the service logs no
"Found" line at all, RobotStudio connects over IPv4, CodeMeter logs no
access, and RobotStudio exits with 103.

Traced with Wine's `iphlpapi`, `nsi` and `winsock` channels, CodeMeter's
process at its start calls `GetAdaptersAddresses(AF_UNSPEC, 0)` (6x),
`GetAdaptersAddresses(AF_UNSPEC, 0x2e)`, `GetAdaptersAddresses(AF_INET6,
0)` (2x), `GetIpAddrTable` (2x) and `NotifyAddrChange` (3x); Wine's only
`fixme` there is `NotifyIpInterfaceChange` with `init_notify` 0, which
cannot be the cause (the Windows log has the lines at every start).
Hypothesis: one of these answers lacks something CodeMeter requires of a
usable local address (loopback entry, IfType, OperStatus, interface
indexes, zone, prefix/suffix origin, DAD state, lifetimes, flags), so it
ends up with no local address list.

## What it measures

The same calls (plus `GetAdaptersAddresses(AF_INET, 0)` and a sorted
`GetIpAddrTable`), and for each adapter in the order returned: `Length`,
`IfType`, `OperStatus`, `Flags` (also decoded), whether `IfIndex` and
`Ipv6IfIndex` are set and equal, `Mtu`, `PhysicalAddressLength`,
`TunnelType`, `ConnectionType`, `Luid.Info.IfType`, `Ipv4Metric`,
`Ipv6Metric`, `ZoneIndices` (link, site), whether names are set, and the
number of unicast/anycast/multicast/DNS/gateway entries; each prefix; and
for each unicast address its class, `iSockaddrLength`, `Length`,
`Flags`, `PrefixOrigin`, `SuffixOrigin`, `DadState`,
`OnLinkPrefixLength`, scope id and the three lifetimes (infinite, zero
or finite). For `GetIpAddrTable`: return codes, entries, class, mask
length, index set, broadcast, reassembly size, type.

Privacy: no adapter names, descriptions, GUIDs, MAC addresses, DNS
suffixes or non-loopback addresses are printed; loopback addresses in
full, link-local as "link-local" with the scope id, others as
"other-v4"/"other-v6".

## Build and run

```sh
bash scripts/build-measure-iphlpapi-adapters.sh /path/to/NEW-bundle
```

Windows 10 (offline VM): copy the bundle into a new folder, run
`run-measure.bat`, return `result-x86_64.txt`, `result-i686.txt`,
`exitcodes.txt`.

Wine (the RobotStudio prefix, the same winum as for RobotStudio):

```sh
export WINEPREFIX="$HOME/.wine-robotstudio2026.try-11-winmgmt"
cd /path/to/NEW-bundle
wine ia-measure-x86_64.exe > result-wine-x86_64.txt 2>/dev/null
wine ia-measure-i686.exe > result-wine-i686.txt 2>/dev/null
```

The outputs are compared line by line; the network differs between the
VM and the Linux machine (number of adapters, LAN addresses), so what
counts is the loopback adapter and the fields of its addresses.

## Results

- Wine in this repository's build sandbox (x86_64, no IPv6, 2026-11-17),
  only to check the program:
  [`wine-sandbox-2026-11-17/`](wine-sandbox-2026-11-17/). Already
  visible: the loopback address's lifetimes are finite and both metrics
  are 0xffffffff; whether Windows differs is the measurement.
  `GetAdaptersAddresses(AF_INET6)` returns 9852 there (no IPv6 in the
  sandbox kernel; not used).
- Windows 10: pending.
- Wine on the maintainer's machine (try-11-winmgmt): pending.
