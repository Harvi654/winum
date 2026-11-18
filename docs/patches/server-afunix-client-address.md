# AF_UNIX client address after connect

<!-- winum-test: l1 -->

A Windows 10 19045.6466 client that binds a relative name before
connecting keeps that name in getsockname. Bound ConnectEx returns
`WSA_IO_PENDING`, completes successfully, and reports a full-size
peer address after the connect context is updated.

The Wine v3 server replaced the stored local name with the generic
unnamed AF_UNIX conversion and completed an immediate ConnectEx
synchronously. Preserve the server's existing bound address and
use the async completion queue for overlapped AF_UNIX connects.
When the host has connected immediately, keep the established state
while the completion is queued. Independent review reproduced a
native peer send and close leaving POLLOUT and POLLHUP set together,
with SO_ERROR zero. Treating that as a failed connection would make
the peer name unavailable after a successful completion.
The extended peer address uses the full buffer size measured on
Windows. The server also validates the declared length before
reading the address family and uses the measured `WSAEFAULT` for
a short synchronous AF_UNIX connect.

The [MEASURE report](../wine-repro/ws2_32-afunix/v4.md) shows both
architectures. The Wine test covers ordinary connect and ConnectEx.
The listener-side named peer assertions remain todo. Their design
and scope are recorded in the candidate README.

| Level | State |
| --- | --- |
| L0 | Passed on Wine 11.22, x86_64 and i386, `36b088f`, 2026-11-19. Applied without fuzz |
| L1 | Passed on Windows 10 offline VM, 2026-11-18, `c3fc029`. 225 checks, zero failures, x86_64 and i686 |
| L2 | Zero new hard failures against v3 and the test-only baseline. [Run 44406951061](https://github.com/Harvi654/winum/actions/runs/44406951061) |
| L4 | Pending SpaceMouse integration and maintainer checks |

v4 (2026-11-18) corrects the connecting client's local address and
overlapped completion. The Linux host probe also exercises pending
completion when the peer immediately sends data and closes.
