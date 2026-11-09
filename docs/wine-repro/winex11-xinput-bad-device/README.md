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

## Build

On Arch use official `mingw-w64-gcc`, `gcc`, `libx11` and `libxi` packages.
The helper is deliberately Linux x86_64/glibc (versioned dlsym symbol),
suited to winum's pure WoW64 Unix side. It is not a macOS, musl or 32-bit
Unix interposer. The PE fixture also compiles as i686.

```bash
export OUT="$HOME/.cache/winum-xinput-$(date +%Y%m%d-%H%M%S)"
mkdir -p "$OUT"
x86_64-w64-mingw32-gcc -O2 -Wall -Wextra -Werror -static -o "$OUT/raw-input.exe" docs/wine-repro/winex11-xinput-bad-device/repro.c -luser32
cc -O2 -Wall -Wextra -Werror -shared -fPIC -o "$OUT/fault-xinput.so" docs/wine-repro/winex11-xinput-bad-device/fault-xinput.c -ldl -pthread
```

## Paired test on the maintainer's machine

Use two matched Wine 11.20 builds differing only in this patch; use a
separate new prefix for each. Never replace installed application DLLs.
Stay in the Wayland session through Xwayland (or repeat on Xorg); explicitly
select the x11 Wine driver. Initialize the prefix before preloading:

```bash
export WINEBIN="/absolute/path/to/runtime/bin/wine"
export WINEPREFIX="$OUT/prefix-without"
export WINEDLLOVERRIDES='mscoree,mshtml='
export WINEDEBUG=-all,+x11drv,+event
"$WINEBIN" reg add 'HKCU\Software\Wine\Drivers' /v Graphics /t REG_SZ /d x11 /f
"${WINEBIN%/wine}/wineserver" -k
"${WINEBIN%/wine}/wineserver" -w
export LD_PRELOAD="$OUT/fault-xinput.so"
export WINUM_XI_FAULT=observe
"$WINEBIN" "$OUT/raw-input.exe" > "$OUT/observe.txt" 2>&1
export WINUM_XI_FAULT=open
"$WINEBIN" "$OUT/raw-input.exe" > "$OUT/open.txt" 2>&1
export WINUM_XI_FAULT=map
"$WINEBIN" "$OUT/raw-input.exe" > "$OUT/map.txt" 2>&1
unset LD_PRELOAD WINUM_XI_FAULT
grep -E '^(REPRO|XI_HOOK|XI_CALL|FAULT|RAW|CLICK|ALIVE|SUMMARY|INCONCLUSIVE|ERROR)|XI_BadDevice|identity.*mapping|no button mapping' "$OUT"/observe.txt "$OUT"/open.txt "$OUT"/map.txt
```

At each window, move the real pointer into it and click three times. Do
not use SendInput/SendMessage as a substitute: those may bypass XI raw
events and the changed query path. Keep other applications alone and use
this preload only for the fixture. Raw INPUTSINK may receive background
mouse packets, but the program records only counts/button flags and never
keyboard data or device names. The helper logs numeric source IDs only.

Repeat with the patched runtime, a fresh prefix and distinct output files.
Stop and wait for the selected prefix's wineserver before changing runtimes.
Unset both helper variables before launching any other Wine application.

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
