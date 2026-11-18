# AF_UNIX address translation errors

The AF_UNIX measurement on Windows 10 19045.6466 found `WSAEFAULT`
for bind and connect given a valid backing buffer and a declared
length of 0 or 1. Wine v3 already returned that value for bind and
returned `WSAEINVAL` for connect.

In `ws2_32/socket.c`, `afunix_path_addr` inspected the family before
validating the declared length. Its null return also represented both
pass-through and translation failure. Distinguish those outcomes for
bind, connect and ConnectEx so that a failed conversion no longer
submits an untranslated named AF_UNIX path. The companion server
patch chooses the measured short connect error by socket family.

The [MEASURE report](../wine-repro/ws2_32-afunix/v4.md) records the
short-buffer test. Allocation failures and DOS-path translation
failures use Wine's existing Winsock error categories. The harness
does not inject allocation failures.

| Level | State |
| --- | --- |
| L0 | Applied without fuzz to the verified Wine 11.22 source and base series. Both-architecture build pending |
| L1 | Both test binaries built. Offline Windows 10 run pending |
| L2 | Paired Wine v3/v4 run pending |
| L4 | Pending SpaceMouse integration and maintainer checks |

v4 (2026-11-18) adds length guards and reports named-address
translation failures at the Winsock call.
