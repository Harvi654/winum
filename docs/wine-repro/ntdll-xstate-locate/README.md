# ntdll-locate-xstate-feature-enabled: standalone L1 check

The patch's check lives in Wine's `ntdll:exception` test unit, and that unit
restarts Windows 10 VMs when run there (2026-11-08: the maintainer's Linux
VM and a VM on a Mac, x86_64 and i386 each). The unit exercises debug
registers, single-stepping and extended CPU state, which the hypervisors do
not survive; the restart is not attributed to the patch's check.

`l1.c` repeats only that check: `RtlLocateExtendedFeature` for a feature
that is not enabled (CET_U) on a `CONTEXT_EX` whose XSAVE offset is invalid
must return NULL without reading it; AVX in a valid context is located as a
control. Build both architectures with MinGW and run in the VM:

```bash
x86_64-w64-mingw32-gcc -O2 -Wall -o xstate-l1-x86_64.exe l1.c
i686-w64-mingw32-gcc -O2 -Wall -o xstate-l1-i686.exe l1.c
```

Expected on Windows: `ok` twice and `SUMMARY checks=2 failures=0`, or a
`SKIP` line when the machine has no AVX or has CET_U enabled.
