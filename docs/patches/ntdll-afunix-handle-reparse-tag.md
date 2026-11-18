# AF_UNIX reparse tag in file-handle queries

<!-- winum-test: l1 -->

Programs can query metadata on a bound socket's file opened with
`FILE_FLAG_OPEN_REPARSE_POINT`. On Windows 10 19045.6466,
`FileAttributeTagInfo` and `FileStatInformation` both return attributes
`0x420` and tag `0x80000023`. Wine v3 returned the same attributes
with a zero tag.

In `ntdll/unix/file.c`, `fd_get_file_info` cleared the tag after
assigning `IO_REPARSE_TAG_AF_UNIX`. Initialize the tag before
inspecting the inode. The existing mount-point and extended-attribute
checks retain their order.

[The MEASURE report](../wine-repro/ws2_32-afunix/v4.md) records the
offline Windows 10 and paired Wine v3 observations. The candidate adds
both assertions to the shared AF_UNIX test and the native L1 bundle.

| Level | State |
| --- | --- |
| L0 | Passed on Wine 11.22, x86_64 and i386, `36b088f`, 2026-11-19. Applied without fuzz |
| L1 | Passed on Windows 10 offline VM, 2026-11-18, `c3fc029`. 225 checks, zero failures, x86_64 and i686 |
| L2 | Zero new hard failures against v3 and the test-only baseline. [Run 44406951061](https://github.com/Harvi654/winum/actions/runs/44406951061) |
| L4 | Pending maintainer checks of the combined candidate |

v4 (2026-11-18) records the file-handle correction separately from
the original path-based AF_UNIX reparse support.
