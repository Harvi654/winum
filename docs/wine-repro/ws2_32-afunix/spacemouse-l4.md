# Inventor 2027 SpaceMouse L4 on Arch

This trial uses [Inventor PR #25](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/pull/25)
at `54f83ec61329a4fcbaa00c0f991a846b6058277a` and
the winum 11.22-1 AF_UNIX v4 candidate. The PR's default transport
is loopback TCP through socat. The test patch adds an opt-in direct mode
to the same SIAPP DLL and skips the relay in that mode. The Windows DLL
uses `\\?\unix\run\spnav.sock`, so the test works with the normal
isolated drive mapping.

## Get the runtime

Use the `afunix-candidate` artifact once it appears in
[run 44406951061](https://github.com/Harvi654/winum/actions/runs/44406951061).
For an initial direct-transport smoke test, run 44398256194 already
has a candidate built from `c3fc029`. Its synchronous connect path
is the same as the revised candidate for this test. Set the run and
commit below to `44398256194` and `c3fc029` for that trial. Repeat on
`36b088f` to record L4 for the candidate that includes the ConnectEx
review fix.

```bash
export WINUM_CI_RUN=44406951061
export WINUM_CANDIDATE_COMMIT=36b088f
export WINUM_ARTIFACT="$(mktemp -d "$HOME/.winum-afunix.XXXXXXXX")"
export WINUM_NAME=winum-11.22-1-afunix-v4-x86_64
gh run download "$WINUM_CI_RUN" --repo Harvi654/winum -n afunix-candidate -D "$WINUM_ARTIFACT"
(
    set -e
    cd "$WINUM_ARTIFACT"
    sha256sum -c "$WINUM_NAME.tar.xz.sha256"
    tar -xJf "$WINUM_NAME.tar.xz"
    grep -F "($WINUM_CANDIDATE_COMMIT)" "$WINUM_NAME/CANDIDATE.txt"
)
export WINUM_ROOT="$WINUM_ARTIFACT/$WINUM_NAME"
```

## Prepare the Inventor checkout

Use the existing Inventor repo and a new worktree. Adjust the two
checkout locations if necessary.

```bash
export WINUM_REPO="$HOME/winum"
export INVENTOR_REPO="$HOME/inventor-on-linux"
export INVENTOR_TEST="$HOME/inventor-spacemouse-afunix"
export INVENTOR_PR_COMMIT=54f83ec61329a4fcbaa00c0f991a846b6058277a
git -C "$WINUM_REPO" fetch origin topic/ws2_32-af-unix-v4
git -C "$WINUM_REPO" show FETCH_HEAD:docs/wine-repro/ws2_32-afunix/spacemouse-pr25-direct.patch > "$WINUM_ARTIFACT/spacemouse-pr25-direct.patch"
git -C "$INVENTOR_REPO" fetch https://github.com/Harvi654/Autodesk-Inventor-for-Linux.git refs/pull/25/head
git -C "$INVENTOR_REPO" worktree add --detach "$INVENTOR_TEST" "$INVENTOR_PR_COMMIT"
git -C "$INVENTOR_TEST" apply --check "$WINUM_ARTIFACT/spacemouse-pr25-direct.patch" &&
    git -C "$INVENTOR_TEST" apply "$WINUM_ARTIFACT/spacemouse-pr25-direct.patch"
```

## Start the test

Connect the SpaceMouse and close Inventor. Start the local `spacenavd`
service if it is stopped (`sudo systemctl start spacenavd` on Arch).
The installer uses the Arch MinGW compiler. Use the existing 2027
trace copy if it still exists. A substitute copy should live beside
the original prefix on the same filesystem. Check space with
`df -h "$HOME"` before making a copy.

The launcher's migration can update a copy initially made with the
pinned 11.20-2 runtime. The first start can take longer. If sign-in
is needed, the browser callback must use the same candidate Wine for
this session. After testing, restore the usual browser handler with
`xdg-mime default adskidmgr-handler.desktop x-scheme-handler/adskidmgr`.
The copy contains Autodesk sign-in data.

```bash
(
    set -eu
    umask 077
    export INV_YEAR=2027
    export INV_PREFIX="$HOME/.wine-inventor2027.trace-A"
    export INV_LOGDIR="$(mktemp -d "$HOME/.inventor-spacemouse.XXXXXXXX")"
    export INV_DATA_DIR="$INV_LOGDIR/cad"
    export INV_WINE=system
    export PATH="$WINUM_ROOT/bin:$PATH"
    export WINE="$WINUM_ROOT/bin/wine"
    export WINELOADER="$WINUM_ROOT/bin/wine"
    export WINESERVER="$WINUM_ROOT/bin/wineserver"
    export SIAPPDLL_TRANSPORT=afunix
    export SIAPPDLL_CONFIG="C:\\${INV_LOGDIR##*/}.ini"
    export SIAPPDLL_LOG="C:\\${INV_LOGDIR##*/}.log"
    export SPACEMOUSE_LOG="$INV_PREFIX/drive_c/${INV_LOGDIR##*/}.log"

    test -S /run/spnav.sock
    test -f "$INV_PREFIX/drive_c/Program Files/Autodesk/Inventor 2027/Bin/Inventor.exe"
    test -x "$WINE"
    grep -F "($WINUM_CANDIDATE_COMMIT)" "$WINUM_ROOT/CANDIDATE.txt"
    "$WINE" --version
    mkdir -- "$INV_DATA_DIR"
    install -m 600 "$INVENTOR_TEST/spacenav/spacemouse.ini" "$INV_PREFIX/drive_c/${INV_LOGDIR##*/}.ini"
    cd "$INVENTOR_TEST"
    bash scripts/install-spacemouse.sh
    LAUNCH_STATUS=0
    bash scripts/launch-inventor.sh > "$INV_LOGDIR/launcher.log" 2>&1 || LAUNCH_STATUS=$?
    printf 'Launcher exit=%s\n' "$LAUNCH_STATUS"
    if [ -f "$SPACEMOUSE_LOG" ]; then
        grep -aE 'connected directly to spacenavd \(AF_UNIX\)|AF_UNIX (motion received|connection failed, error [0-9]+)|button [0-9]+ (press|release)' "$SPACEMOUSE_LOG" | tail -n 20 || true
    else
        grep -aoE 'SpaceMouse direct transport: AF_UNIX /run/spnav.sock|Inventor is already running|updating the prefix stopped|version mismatch' "$INV_LOGDIR/launcher.log" | sort | uniq -c || true
        printf '%s\n' 'No siappdll log was created.'
    fi
)
```

Create or open a disposable part. Pan, rotate and zoom with the
SpaceMouse, then press a button. Verify that Inventor actually moves
the viewport. The trace should contain both
`connected directly to spacenavd (AF_UNIX)` and
`AF_UNIX motion received`. Return the filtered lines and what the
viewport and button did. For L4, also check startup, part, assembly,
drawing, save/reopen and restart in the copy. Repeat the viewport
check after restarting. If a comparison is needed, rerun the same
checkout and candidate with `SIAPPDLL_TRANSPORT=tcp` and socat
installed.

The patch applied cleanly to the pinned PR. The x64 DLL compiled with
the PR's MinGW flags. A launcher check with a disposable Linux socket
confirmed AF_UNIX mode skips the relay and TCP mode starts it, both
with the supplied configuration. The hardware check is pending. The
hardware-free Inventor result is recorded below.

## Trial with no 3D mouse

Use the same checked-out PR, direct transport patch, verified winum
runtime and 2027 trace prefix. The supplemental patch accepts a private
socket. The Python fixture binds it before starting Inventor and sends
motion packets on request. It needs neither the system spacenavd service
nor a device. It exercises the same AF_UNIX connection and Inventor
message delivery as the direct trial. Hardware input and the real
spacenavd endpoint still need a hardware test.

In the Bash session that prepared the build and checkout, close Inventor
and run:

```bash
(
    set -eu
    umask 077
    export INV_YEAR=2027
    export INV_PREFIX="$HOME/.wine-inventor2027.trace-A"
    export INV_LOGDIR="$(mktemp -d "$HOME/.inventor-spacemouse-mock.XXXXXXXX")"
    export INV_DATA_DIR="$INV_LOGDIR/cad"
    export INV_WINE=system
    export PATH="$WINUM_ROOT/bin:$PATH"
    export WINE="$WINUM_ROOT/bin/wine"
    export WINELOADER="$WINUM_ROOT/bin/wine"
    export WINESERVER="$WINUM_ROOT/bin/wineserver"
    export SIAPPDLL_CONFIG="C:\\${INV_LOGDIR##*/}.ini"
    export SIAPPDLL_LOG="C:\\${INV_LOGDIR##*/}.log"
    export SPACEMOUSE_LOG="$INV_PREFIX/drive_c/${INV_LOGDIR##*/}.log"

    test -f "$INV_PREFIX/drive_c/Program Files/Autodesk/Inventor 2027/Bin/Inventor.exe"
    "$WINE" --version
    git -C "$WINUM_REPO" fetch origin topic/ws2_32-af-unix-v4
    for file in spacemouse-pr25-mock.patch spacemouse-motion.py; do
        git -C "$WINUM_REPO" show "FETCH_HEAD:docs/wine-repro/ws2_32-afunix/$file" > "$INV_LOGDIR/$file"
    done
    cd "$INVENTOR_TEST"
    if git apply --check "$INV_LOGDIR/spacemouse-pr25-mock.patch"; then
        git apply "$INV_LOGDIR/spacemouse-pr25-mock.patch"
    else
        git apply --reverse --check "$INV_LOGDIR/spacemouse-pr25-mock.patch"
    fi
    mkdir -- "$INV_DATA_DIR"
    install -m 600 spacenav/spacemouse.ini "$INV_PREFIX/drive_c/${INV_LOGDIR##*/}.ini"

    trial_status=0
    python3 -I "$INV_LOGDIR/spacemouse-motion.py" "$INV_LOGDIR/launcher.log" \
        bash -c 'bash scripts/install-spacemouse.sh && exec bash scripts/launch-inventor.sh' || trial_status=$?
    printf 'Trial exit=%s\n' "$trial_status"
    if [ -f "$SPACEMOUSE_LOG" ]; then
        grep -aoE 'connected directly to spacenavd \(AF_UNIX\)|AF_UNIX (motion received|connection failed, error [0-9]+)|invalid SIAPPDLL_SOCKET' \
            "$SPACEMOUSE_LOG" | sort | uniq -c || true
    else
        grep -aoE 'AF_UNIX test socket is ready|SpaceMouse direct transport: AF_UNIX|Inventor is already running|updating the prefix stopped|version mismatch' \
            "$INV_LOGDIR/launcher.log" | sort | uniq -c || true
    fi
)
```

Open or create a solid in Inventor, return to the terminal, press Enter,
and switch focus back to Inventor within five seconds. Watch for a change
in the view. After Inventor exits, report the filtered output and whether
the view moved. The fixture sends three groups of motion with a neutral
frame after each one. It sends no button commands.

Built the amended DLL as x86_64 PE with the PR's flags and `-Werror`.
A native test client received all 72 nonzero motion frames and the three
neutral frames over the private socket.

### Inventor result (2026-11-18)

The maintainer ran the hardware-free trial in Inventor 2027 on Arch with
winum 11.22-1 AF_UNIX v4 `c3fc029`, Inventor PR #25 `54f83ec` and the
synthetic motion patch from winum `be7c13f`. The sender connected, wrote
all 72 motion frames, and exited 0 after Inventor closed. The DLL trace
confirmed direct AF_UNIX connection and reception of motion. The visible
viewport performed the three programmed movements.

This is an application-level pass for synthetic motion on `c3fc029`.
Physical input, buttons, the running spacenavd service and the complete
Inventor L4 checklist remain open. The ConnectEx follow-up `36b088f`
has a separate build and validation run.
