# L3: XI_BadDevice during legacy XInput mapping

Original, AI-assisted (Codex), LGPL-2.1-or-later. An own raw-input Win32
window and a fixture-only Linux fault helper exercise the Wine X11 mapping
error path. No vendor process, device reconfiguration, device removal,
privileged operation, synthetic key input or Wine code change is involved.

Wine obtains XOpenDevice/XGetDeviceButtonMapping through dlsym of libXi's
handle. Merely preloading same-named exports would not reliably replace
those function pointers, so the helper interposes exactly those dlsym
lookups, retaining the real implementations. For mode `open` it requests
the reserved pseudo-id 0 instead of a real legacy device. For mode `map`
it copies XDevice onto the stack and changes only that query's device ID
to 0. The real object/ID remains intact for the real XCloseDevice. The X
server should reject the legacy request with XI_BadDevice; the helper does
not fabricate a Wine error return or install its own X error handler.
The helper checks only its own Linux argv for basename raw-input.exe
(the documented fixture name) and leaves other processes' lookups alone.
It prints no argv or paths. Every hook/call/fault record includes its Unix
PID and fixture=1. Keep the executable name raw-input.exe; renaming it
makes injection uncovered. Mode `observe` delegates real calls unchanged and provides a healthy-device
control. All other dlsym lookups are delegated unchanged.

The fixture registers mouse raw input, logs raw button flags and ordinary
clicks, and exits after four five-second intervals. A separate 35-second
watchdog bounds a stalled GUI. Exit 0 means it completed with raw button
and ordinary-click delivery and no read errors; **it does not by itself
prove fault injection was reached**. Early close/no input is inconclusive
(exit 2). There are no guessed native Windows expectations.

## Why L3 only

XI 1.x errors from a device reported through XI2, Xlib's error handler and
Wine's fallback mapping are Linux/X11 mechanisms absent from Windows.
Native Windows cannot reproduce this driver failure. Public Win32 input
alone also cannot force an XInput device-query error; the scoped Linux
helper supplies a deterministic request failure without hotplugging a
real device. This is fault-injection L3, not Windows L1 or a claim that an
unmodified physical device naturally triggers the same condition.

## Run the CI-built pair on Arch / KDE Wayland

Use [the shared L3 runner](../l3-ci.md). CI builds one full production
series and three variants omitting exactly one winex11 patch each; no
local Wine compilation or system update is needed. From this checkout:

```bash
bash scripts/run-l3-local.sh RUN_ID
```

The runner downloads all artifacts with `gh` (GH_TOKEN unset), verifies
checksums, commit identity and exact omission lists, and runs this fixture
and its documented controls with throwaway prefixes. It also runs the other
two winex11 fixtures and the MEASURE bundle's NdrStubCall3 probes. Follow
its prompts for physical clicks and visual observations. Return the single
`l3-summary-RUN_ID.txt`; completion is not an automatic pass.

The earlier six-build local procedure is retained in
`docs/archive/l3-local-build-2026-11-09/` for history. It is superseded by
the four shared CI builds. Fixture source and interpretation are unchanged.

## Expected comparison and coverage checks

- Healthy `observe`: real device mapping, ongoing raw/ordinary clicks,
  complete=1 and exit 0 in both builds.
- Injected `open`/`map`: require XI_HOOK for the relevant symbol **and a
  FAULT line for the actual call in that same PID, fixture=1**, plus the
  X error/Wine fallback evidence. The Win32 REPRO windows_pid is a separate
  Wine ID, not the helper's Unix PID.
  Without the patch, an unhandled XI_BadDevice is expected to terminate
  the process before the final summary. With the patch, Wine should catch
  it, retain identity mapping and continue raw/ordinary button delivery.
- Further clicks from the same source should not repeatedly re-query that
  failed mapping: compare XI_CALL counters/source_id. A genuine source
  switch can legitimately require a new mapping; do not demand one call
  for the entire desktop. Healthy control must remain functional.

If mapping is never called (no FAULT), the driver/XI2 path was not covered.
If the real XOpenDevice already fails on this server, map mode cannot prove
the later mapping-query branch; record it as an uncovered branch rather
than a pass. If pseudo-id 0 does not provoke the expected error on a
different server, the injection is inconclusive. If both builds survive,
inspect whether another error guard already handled it. The helper does
not force the close-error path, malloc failure, every XI2 event type or
all physical-button remappings. No claim is made that identity mapping
matches user-specific nonstandard mapping after a failed query.

2026-11-09: strict PE x86_64/i686 builds and Linux x86_64 helper build
passed with -Wall -Wextra -Werror. A Linux symbol-lookup-only smoke check
against the system libXi confirmed both dlsym lookups return the helper's
functions; it sent no X requests and did not run Wine. No paired
fault-injection result or new consumer L4 is claimed. The page marker
records the X11-only reason and reproducible procedure, not a runtime pass.

Run these commands in a fresh terminal. At the end, close the fixture and
unset LD_PRELOAD and WINUM_XI_FAULT as shown above; the disposable test
prefixes require no restoration of an application's registry or DLLs.

A second-model source advisory found no recognizable copied code in the
bounded review (not proof of originality), and no obvious ABI/ownership
blocker. Its process-attribution suggestion was applied: only the named
fixture process is wrapped, and helper records identify the Unix PID.
The advisory and lookup smoke are not Wine fault-injection results.

## After the test

The runner stops each matching private wineserver and removes its temporary
prefixes, runtimes and raw output, including on interruption. Only the
filtered summary remains. Application prefixes and installed Wine are not
changed. A timeout or absent required observer/fault evidence remains
inconclusive; use the interpretation above before recording L3 success.
