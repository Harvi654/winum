# Inventor native-Wayland control (2026-11-07)

Prepared by Codex. No Wine patch. The procedure and subsequent maintainer
result are recorded here. See the [status-bar investigation](../comctl32-statusbar-erase/README.md)
for the XRender workaround and remaining frame artefact after ray tracing.

The maintainer confirmed a Wayland desktop session. The measured runs use
Wine's X11 driver, hence XWayland. This control selects `winewayland.drv`
instead, using the same original gpurt-v3 Wine/GPU/DLL configuration. It tests
menus, ordinary window transitions, then ray tracing/window transitions.

## Source checks and limits

Wine 11.20 mirror commit `7298a818b79446bae153b8d5aa27235ab3703bf9`:

- `programs/explorer/desktop.c:load_graphics_driver` reads
  `HKCU\Software\Wine\Drivers`, `Graphics`. Default order is
  `mac,x11,wayland`; a value of just `wayland` has no X11 fallback. The driver
  selected successfully is written as volatile `GraphicsDriver` under
  `HKLM\System\CurrentControlSet\Control\Video\{GUID}\0000`.
- `win32u/driver.c:load_desktop_driver` loads the selected desktop driver.
  A graphics-driver trace naming a library is not sufficient by itself:
  explorer also traces its attempted name when loading fails. Require an
  actual visible application and the successful driver's volatile registry
  value, not merely a tentative module-load line.
- `winewayland.drv` implements GDI window surfaces, OpenGL through EGL and
  native Vulkan surfaces. There is no GLX path and no XRender driver. The
  copied X11 `UseEGL=N` and `ClientSideWithRender=N` values do not select
  native-Wayland rendering paths. A successful result changes multiple host
  components (including GLX to EGL), so it cannot uniquely blame XWayland.
- Both repository build recipes request `--with-wayland`. Verify the driver
  files in the maintainer's installed build before copying a prefix.

The Inventor launcher/reference checkout at
`996576aa07b7ff153d341496d23bb76021db1fa2` was read for configuration and
startup. Its setup `cfg_wine_mode` explicitly selects `Graphics=x11`, so
unsetting `DISPLAY` alone would not be a sufficient test. Its setup comment
also records an earlier native-Wayland menu problem, without a runtime result
for this new control. The launcher itself does not call `cfg_wine_mode` on
every start; `sandbox_reg` does not reset `Graphics`.

## Preconditions and isolated prefix

Close Inventor and its old launcher first. Keep the RX 6400 bound to amdgpu,
with the same PRIME selection as before. The main prefix is stopped for a
consistent copy and never has its Graphics value changed by this procedure.
The copy contains the same program; do not install another CAD application
into either prefix. It remains private and must not be committed/uploaded.
Reflinks are used if supported; otherwise allow disk space for a full copy.
External document/font drive links remain external, so use a test document
without saving changes during the first comparison.

Terminal 1, before copying:

```bash
export INV_YEAR=2027
export INV_WINE=bundled
export INV_WINE_HOME="$HOME/.local/share/inventor-gpurt-v3"
export TEST_WINE="$INV_WINE_HOME/wine-inventor-11.20-6-x86_64"
export PATH="$TEST_WINE/bin:$PATH"
export WINE="$TEST_WINE/bin/wine"
export WINELOADER="$TEST_WINE/bin/wine"
export WINESERVER="$TEST_WINE/bin/wineserver"
export SOURCE_PREFIX="$HOME/.wine-inventor2027"
export WINEPREFIX="$SOURCE_PREFIX"

printf 'Session=%s Wayland=%s\n' "$XDG_SESSION_TYPE" "$WAYLAND_DISPLAY"
ls -l "$TEST_WINE/lib/wine/x86_64-unix/winewayland.so" "$TEST_WINE/lib/wine/x86_64-windows/winewayland.drv"
lspci -nnk -s 23:00.0 | grep -i 'driver in use'
du -sh "$SOURCE_PREFIX"
df -h "$HOME/.cache"
```

Require `wayland`, a nonempty Wayland display, both driver files, amdgpu and
enough space. If a check fails, stop and report that check; do not replace DLLs.

```bash
"$WINESERVER" -k
"$WINESERVER" -w
export WAYLAND_PREFIX="$HOME/.cache/winum-inventor-wayland-$(date +%Y%m%d-%H%M%S)"
mkdir "$WAYLAND_PREFIX" && cp -a --reflink=auto "$SOURCE_PREFIX/." "$WAYLAND_PREFIX/"
```

Wait for a successful copy before continuing. Repoint only the copy's C:
link, including when the original link was absolute:

```bash
ln -sfn ../drive_c "$WAYLAND_PREFIX/dosdevices/c:"
export INV_PREFIX="$WAYLAND_PREFIX"
export WINEPREFIX="$WAYLAND_PREFIX"
export INV_VDESKTOP=
export INV_GPU=6400
export DRI_PRIME=pci-0000_23_00_0
export INV_LOGDIR="$HOME/sb-snap/native-wayland-$(date +%Y%m%d-%H%M%S)"
export WINEDEBUG=err+all,fixme-all
mkdir -p "$INV_LOGDIR"
printf '%s\n' "$WAYLAND_PREFIX" > "$INV_LOGDIR/prefix.txt"
printf '%s\n' "$INV_LOGDIR" > "$HOME/sb-snap/wayland-test-logdir.txt"

printf 'Test prefix: %s\n' "$WINEPREFIX"
"$WINE" reg add 'HKCU\Software\Wine\Drivers' /v Graphics /t REG_SZ /d wayland /f
"$WINE" reg query 'HKCU\Software\Wine\Drivers' /v Graphics
"$WINESERVER" -k
"$WINESERVER" -w
cd ~/inventor-on-linux
bash scripts/launch-inventor.sh
```

The registry command initially starts the copy using its old X11 setting;
stopping its server afterwards is necessary before the native-Wayland launch.
Keep `DISPLAY` unchanged. Selection is explicit in the copied registry and
the surrounding Linux desktop/browser need not lose their X11 environment.

## Verify the active driver and test

Terminal 2, while Inventor is visible:

```bash
export WL_LOGDIR="$(cat "$HOME/sb-snap/wayland-test-logdir.txt")"
export WINEPREFIX="$(cat "$WL_LOGDIR/prefix.txt")"
export WINE="$HOME/.local/share/inventor-gpurt-v3/wine-inventor-11.20-6-x86_64/bin/wine"
export WINELOADER="$WINE"
export WINEDEBUG=-all
"$WINE" reg query 'HKCU\Software\Wine\Drivers' /v Graphics
"$WINE" reg query 'HKLM\System\CurrentControlSet\Control\Video' /s /v GraphicsDriver
```

Require `Graphics REG_SZ wayland` and active `GraphicsDriver` value
`winewayland.drv`. Report the filtered registry values and whether Inventor
is visible. If selection is ambiguous, do not classify a visual pass yet.

1. Check menu opening/clicking, keyboard input and opening/closing Options.
   Stop here if basic interaction is broken; the old setup comment makes
   this a real compatibility question, not a new status-bar conclusion.
2. Open a test part without saving. Before starting ray tracing, repeat
   Restore Down/maximise twice and observe viewport/ribbon/status text.
3. Enable ray tracing and confirm progressive rendering actually works.
   Observe the same transitions, both status text and the white frame edge.
4. If the frame artefact occurs, test actual minimise-to-taskbar/restore.
5. Close Inventor normally. Return the results, not a full original log.

If launch fails, provide only a bounded diagnostic excerpt:

```bash
grep -hE 'err:(waylanddrv|winediag|module):' "$WL_LOGDIR"/*.log | tail -n 25
```

## Stop and return

After closing Inventor, in Terminal 1:

```bash
"$WINESERVER" -k
"$WINESERVER" -w
export INV_PREFIX="$SOURCE_PREFIX"
export WINEPREFIX="$SOURCE_PREFIX"
```

Close the test terminals. The original prefix still has its previous X11
driver and XRender workaround; no registry rollback in it is necessary.
Keep the copied prefix and logs until the result is assessed. The next normal
launch uses the existing original-prefix gpurt-v3 procedure. This control
does not copy new DLLs, change GPU binding, or establish general native-Wayland
CAD compatibility from a single successful launch.

## Maintainer result and decision (2026-11-07)

The installed original gpurt-v3 build contains both native-Wayland driver
files, the session is Wayland, and RX 6400 is bound to amdgpu. The maintainer
reported after following the native-control procedure:

| Check | Reported result |
| --- | --- |
| WebView content | Does not work |
| Dialogs | Abnormal appearance/behaviour |
| Dimension value entry | Cannot type values |
| GPU ray tracing | Works |
| Restore Down/maximise | Abnormal, not satisfactory |

The active-driver registry output and a filtered diagnostic log have not been
returned. Record these as the maintainer's native-test observations, not a
symbol-level attribution of each fault or an instrument-verified backend
comparison. In particular, working ray tracing is not a general UI pass.

The practical working direction for this investigation remains Wine's X11
driver through XWayland, with per-application `ClientSideWithRender=N` and
the previously tested GLX/gpurt-v3 configuration. That path already passed
ordinary Inventor menus, part/assembly/drawing/PDF, dimension entry and
save/restart/reopen tests, with the post-ray-tracing frame artefact still
open. The native control introduces multiple basic UI failures, so expanding
this task into focus/input/popup/WebView compatibility work would substantially
increase scope without an established fix for the original fault.

Likely areas of compatibility work include Windows focus/text input, hosted
child windows and popup/dialog placement in the Wayland backend, but no exact
failed API or implementation defect is identified by this visual report.
Do not label the Linux Wayland desktop, WebView itself or Autodesk code as
the proven cause. The native-Wayland control is not recommended for ordinary
Inventor use in the tested configuration. Keep its copied prefix for later
comparison and return to the original X11 prefix; no production patch or
desktop/session change is required by this decision.
