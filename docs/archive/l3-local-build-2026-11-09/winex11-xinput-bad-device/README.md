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

## Matched builds on Arch / KDE Wayland

Use a fresh Bash terminal in the normal KDE Wayland session. These commands
build two private runtimes from the same pinned commit and compiler/packages;
only this patch is omitted from the disposable `without-source` worktree.
The main checkout, installed Wine and application prefixes are untouched.
Allow roughly 25–30 GB and several hours for two full builds; keep the same
packages between builds. Nothing is installed from either Wine build.
The packages below follow the project's Arch build workflow; `-Syu` performs
a normal Arch system update. Skip installation if they are already present
and the system is up to date.

```bash
sudo pacman -Syu --needed base-devel git python mingw-w64-gcc \
  alsa-lib libpulse gnutls krb5 dbus fontconfig freetype2 \
  libx11 libxext libxi libxrandr libxrender libxcursor libxcomposite libxinerama libxxf86vm libxkbcommon \
  wayland mesa libglvnd vulkan-icd-loader vulkan-headers \
  gst-plugins-base-libs sdl2 libcups libgphoto2 sane libpcap v4l-utils pcsclite unixodbc ocl-icd opencl-headers giflib

set -euo pipefail
cd "$HOME/winum"
git pull --ff-only
export OUT="$HOME/.cache/winum-xinput-pair-$(date +%Y%m%d-%H%M%S)"
export TEST_COMMIT=b91285035c743528e1da599b85fe9246ba7a258a
export PATCH_NAME=winex11-xinput-bad-device.patch
mkdir -p "$OUT"
unset LD_PRELOAD WINUM_XI_FAULT WINE_X11_SHAPE_ALPHA WINE_OPENGL_REQUIRE_ALPHA
unset WINEPREFIX WINELOADER WINESERVER WINEARCH WINE_VER_OVERRIDE
printf 'SESSION type=%s xwayland_display=%s\n' "$XDG_SESSION_TYPE" "$DISPLAY"
[ "$XDG_SESSION_TYPE" = wayland ]
[ -n "$DISPLAY" ]
git worktree add --detach "$OUT/with-source" "$TEST_COMMIT"
git worktree add --detach "$OUT/without-source" "$TEST_COMMIT"
python3 - <<'PAIR'
import os
from pathlib import Path
out = Path(os.environ['OUT'])
name = os.environ['PATCH_NAME']
source = out / 'with-source/patches/series'
target = out / 'without-source/patches/series'
lines = source.read_text().splitlines(keepends=True)
assert sum(line.split('#', 1)[0].strip() == name for line in lines) == 1
# Omit exactly one entry only in the disposable comparison worktree.
target.write_text(''.join(line for line in lines if line.split('#', 1)[0].strip() != name))
print('PAIR omitted=' + name + ' other_entries_unchanged=1')
PAIR
git -C "$OUT/without-source" diff -- patches/series > "$OUT/without-series.diff"
source "$OUT/with-source/wine/version.env"
export RUNTIME_NAME="winum-$WINE_VERSION-$WINUM_BUILD-x86_64"
export WINE_SOURCE_TARBALL="$OUT/wine-$WINE_VERSION.tar.xz"
export JOBS=2
for variant in without with; do
  export WORK="$OUT/$variant-work"
  if ! bash "$OUT/$variant-source/scripts/build-wine.sh" "$OUT/$variant-dist" > "$OUT/$variant-build.txt" 2>&1; then
    tail -n 40 "$OUT/$variant-build.txt"
    exit 1
  fi
  (cd "$OUT/$variant-dist" && sha256sum -c "$RUNTIME_NAME.tar.xz.sha256")
  "$OUT/$variant-work/$RUNTIME_NAME/bin/wine" --version
  printf 'BUILD variant=%s commit=%s comparison_patch=%s\n' "$variant" "$TEST_COMMIT" "$PATCH_NAME"
done
unset WORK JOBS WINE_SOURCE_TARBALL
```

Both builds use the official source archive, verified against the pinned
SHA-256 by `build-wine.sh`, and apply their lists with `--fuzz=0`.
`without-series.diff`, the two BUILDINFO/PATCHES.txt files and build output
retain the comparison provenance locally. A failed build stops this recipe;
do not substitute an unrelated system Wine as the unpatched control.
The BUILD record identifies the comparison patch; `variant=without`
omits it and `variant=with` keeps it.

## Build the fixture and run the pair

Continue in the same terminal. At each new window, move the real pointer
inside and click three times before its roughly 20-second sequence ends.
There are three modes per build; the label is printed before each one.
Do not substitute SendInput/SendMessage for physical clicks.

```bash
x86_64-w64-mingw32-gcc -O2 -Wall -Wextra -Werror -static -o "$OUT/raw-input.exe" "$OUT/with-source/docs/wine-repro/winex11-xinput-bad-device/repro.c" -luser32
cc -O2 -Wall -Wextra -Werror -shared -fPIC -o "$OUT/fault-xinput.so" "$OUT/with-source/docs/wine-repro/winex11-xinput-bad-device/fault-xinput.c" -ldl -pthread
export WINEDLLOVERRIDES='mscoree,mshtml='
export WINEDEBUG=-all,+x11drv,+event
for variant in without with; do
  export WINEBIN="$OUT/$variant-work/$RUNTIME_NAME/bin/wine"
  export WINEPREFIX="$OUT/prefix-$variant"
  "$WINEBIN" reg add 'HKCU\Software\Wine\Drivers' /v Graphics /t REG_SZ /d x11 /f > "$OUT/$variant-prefix.txt" 2>&1
  "${WINEBIN%/wine}/wineserver" -k
  "${WINEBIN%/wine}/wineserver" -w
  for mode in observe open map; do
    printf 'RUN variant=%s mode=%s: move inside and click three times\n' "$variant" "$mode"
    export WINUM_XI_FAULT="$mode"
    export LD_PRELOAD="$OUT/fault-xinput.so"
    export FIXTURE_RC=0
    "$WINEBIN" "$OUT/raw-input.exe" > "$OUT/$variant-xi-$mode.txt" 2>&1 || export FIXTURE_RC="$?"
    unset LD_PRELOAD WINUM_XI_FAULT
    printf 'EXIT code=%s\n' "$FIXTURE_RC" >> "$OUT/$variant-xi-$mode.txt"
    "${WINEBIN%/wine}/wineserver" -k
    "${WINEBIN%/wine}/wineserver" -w
    grep -HE '^(REPRO|XI_HOOK|XI_CALL|FAULT|RAW|CLICK|ALIVE|SUMMARY|INCONCLUSIVE|ERROR|EXIT)|XI_BadDevice|identity.*mapping|no button mapping' "$OUT/$variant-xi-$mode.txt" || true
  done
done
unset LD_PRELOAD WINUM_XI_FAULT WINEPREFIX WINEBIN WINEDLLOVERRIDES WINEDEBUG FIXTURE_RC
```

The prefix is initialized before the helper is exported. Each mode uses a
new Wine process, with its wineserver stopped afterwards so a failed
mapping cache cannot mask a later injection. Unpatched abnormal exit is
recorded and the loop proceeds to the next control; it is not hidden as
fixture success. The helper is unset immediately after every Wine call.
Keep the filename raw-input.exe because the helper checks that basename.

Return only this filtered output. For each injected mode require XI_HOOK
and a FAULT call in the same Unix PID with fixture=1; otherwise record the
branch as uncovered. REPRO's windows_pid is a different Wine identifier.
Healthy observe must deliver raw and ordinary clicks in both builds.

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

Only disposable prefixes and private runtimes under OUT were used. No
Inventor/SOLIDWORKS DLLs or registry settings need restoring. The commands
unset the diagnostic variables and stop the matching prefix's wineserver.
If interrupted, close the fixture first, unset LD_PRELOAD and
WINUM_XI_FAULT/WINE_X11_SHAPE_ALPHA, and stop that same prefix's wineserver
before starting another Wine program. Keep OUT and its result files until
the comparison is recorded; do not commit raw Wine debug output.

2026-11-09 procedure update: shell syntax and all three omitted-patch
source lists were checked without using the maintainer's desktop session.
The paired runtime results above remain pending.
