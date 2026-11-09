# L2: vanilla Wine against the winum series

Groups: g1, g2, g3, g4, g5. Units: 420. Regressions: 6. Fixed: 5. Moved: 2. Other changes: 39.

## Regressions (new failure locations)

- `comctl32.i386.toolbar`: new failures: toolbar.c:2134 x1 (Test failed: [4] Unexpected number of rows 5 (expected 9))
- `comctl32.x86_64.toolbar`: new failures: toolbar.c:2134 x1 (Test failed: [4] Unexpected number of rows 5 (expected 9))
- `ntdll.i386.exception`: new failures: exception.c:11638 x4 (Test failed: Got unexpected data 0, i 4.), exception.c:11692 x4 (Test failed: Got unexpected data 0, i 4.), exception.c:11695 x4 (Test failed: Got unexpected data 0, i 0.)
- `urlmon.i386.sec_mgr`: new failures: sec_mgr.c:2444 x4 (Test failed: L"res:///btn.xmx/icon.png": zone = 0, expected 3)
- `urlmon.x86_64.sec_mgr`: new failures: sec_mgr.c:2444 x4 (Test failed: L"res:///btn.xmx/icon.png": zone = 0, expected 3)
- `user32.i386.win`: new failures: win.c:4866 x1 (Test failed: hwnd 026500F2/026500F2 message 0200), win.c:4870 x1 (Test failed: hwnd 026500F2/026500F2 message 0201), win.c:4879 x1 (Test failed: hwnd 005A00E0/005A00E0 message 0202), win.c:4882 x1 (Test failed: hwnd 005A00E0/005A00E0 message 0201)

## Fixed

- `ntdll.i386.time`: fixed: time.c:460
- `urlmon.i386.sec_mgr`: fixed: sec_mgr.c:1835, sec_mgr.c:364
- `urlmon.x86_64.sec_mgr`: fixed: sec_mgr.c:1835, sec_mgr.c:364
- `user32.i386.attachment`: fixed: attachment.c:769, attachment.c:772, attachment.c:774
- `user32.x86_64.attachment`: fixed: attachment.c:769, attachment.c:772, attachment.c:774

## Moved (test file patched, same failures)

- `user32.i386.win`: same failures moved by +157 lines: win.c:10846 x1, win.c:10847 x1
- `user32.x86_64.win`: same failures moved by +157 lines: win.c:10846 x1, win.c:10847 x1

## Other changes

- `advapi32.i386.security`: checks 3567 -> 3576
- `advapi32.x86_64.security`: checks 3567 -> 3576
- `comctl32.i386.button`: checks 11998 -> 12013
- `comctl32.i386.treeview`: checks 5057 -> 5060
- `comctl32.x86_64.button`: checks 11998 -> 12013
- `comctl32.x86_64.treeview`: checks 5057 -> 5059
- `gdi32.i386.font`: checks 79635 -> 80616
- `gdi32.x86_64.font`: checks 79635 -> 80616
- `glu32.i386.glu`: checks 15 -> 386
- `glu32.x86_64.glu`: checks 15 -> 386
- `msi.i386.ancestor`: only in series
- `msi.i386.costing`: only in series
- `msi.i386.maintenance`: only in series
- `msi.i386.selection`: only in series
- `msi.x86_64.ancestor`: only in series
- `msi.x86_64.costing`: only in series
- `msi.x86_64.maintenance`: only in series
- `msi.x86_64.msi`: checks 4477 -> 4476
- `msi.x86_64.selection`: only in series
- `msxml3.i386.domdoc`: checks 42884 -> 42904
- `msxml3.i386.schema`: checks 643 -> 992
- `msxml3.x86_64.domdoc`: checks 42884 -> 42904
- `msxml3.x86_64.schema`: checks 643 -> 992
- `ntdll.x86_64.exception`: checks 5452 -> 5468
- `ole32.i386.storage32`: checks 19032 -> 19102
- `ole32.x86_64.storage32`: checks 19032 -> 19102
- `oleaut32.i386.typelib`: checks 7715 -> 7750
- `oleaut32.x86_64.typelib`: checks 7698 -> 7733
- `opengl32.i386.opengl`: checks 11986 -> 12462
- `opengl32.x86_64.opengl`: checks 11986 -> 12462
- `shell32.i386.shlfolder`: checks 2340 -> 2347
- `shell32.x86_64.shlfolder`: checks 2337 -> 2344
- `user32.i386.input`: checks 6150 -> 6161
- `user32.i386.msg`: checks 32451 -> 32452
- `user32.x86_64.input`: checks 6150 -> 6167
- `user32.x86_64.msg`: checks 32451 -> 32452
- `user32.x86_64.win`: checks 68321 -> 69367
- `wininet.i386.http`: checks 5199 -> 5203
- `wininet.x86_64.http`: checks 5215 -> 5208

A regression may still be a timing-dependent test that fails on either side;
such units are re-run before a result is recorded on a patch page.
