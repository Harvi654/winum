# MEASURE: which child pixels survive SetWindowPos?

Inventor 2027's Frame Analysis panes can display pixels from the old top-left
part of the main window. Minimising and restoring clears them. The first trace
and the observed copy are recorded in the
[Inventor notes](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/c7b1ae3/docs/arch-hyprland.md#inventor-2027).
In the decisive trace a visible, direct child moves from under the ribbon with
NOZORDER and NOACTIVATE. Whether Windows preserves only the visible, valid
pixels or handles this differently is an open measurement.

## Cases

Each case starts with a new visible window set. The program measures G first.
Coordinates are scaled down for smaller desktops. The container is 180 x 150
and moves from client (0,0) to (320,70), into a clear destination with a green
background. The container paints blue. In G the parent is green and a red
ribbon is explicitly above the container in Z order. In A–F the parent paints
the source area red.

| Case | Sequence |
|---|---|
| G | A visible container with both clipping styles moves from under a sibling ribbon with WS_CLIPSIBLINGS. Cross full, left-half and zero overlap with same-thread and other-thread ownership. Run each with and without an initial paint drain. The initial snapshot shows how much actually painted, including a fully occluded container. |
| A | Move a visible container before pumping its initial paint. |
| B | Move a hidden container, then show it with SWP_SHOWWINDOW. |
| C | Pump the initial paint, send WM_SETREDRAW(FALSE), move the container, send TRUE and call SetWindowPos with FRAMECHANGED, NOMOVE, NOSIZE, NOZORDER and NOACTIVATE. |
| D | Use the C sequence and move the intermediate parent. Apply FRAMECHANGED to the container. |
| E | Repeat C and D with the container owned by a worker thread. The main thread calls SetWindowPos and sends WM_SETREDRAW. |
| F | Repeat A once with NOREDRAW and once with NOCOPYBITS. |

The G, A and ordinary B–E moves pass NOZORDER and NOACTIVATE with the original
size. The trace also shows the internal NOCLIENTMOVE and NOCLIENTSIZE flags.
Snapshots precede posted-message dispatch. The worker handles sent messages
even between snapshots so cross-thread calls can complete. It drains posted
messages at the same explicit checkpoints as the main thread. Initial warm
cases drain before the first snapshot. Every case drains after its last
operation. This ordering distinguishes synchronous paint from deferred paint.

The two sampled points are at one-quarter and three-quarters of the pane's
width, halfway down. Each line gives the source and destination pixels from
the top-level window DC, the container's screen rectangle, update rectangle,
region type and bounds, and membership of the two container-local points in
that region. Counts are cumulative. The program logs the most recent
WM_NCCALCSIZE with wParam TRUE, including all three input and output
rectangles and the return flags. When the return flags omit WVR_VALIDRECTS,
the last two output slots are just raw fields. The header gives architecture
and DPI. Colors are RGB hex: green `00008000`, red `00ff0000`, blue
`000000ff`. `ffffffff` is an invalid pixel sample.

Use the initial source samples to validate the painted setup and the initial
destination as a color baseline. Compare `after_move` against both, before
looking at `after_pump`. An invalid DC sample cannot establish what was
copied. Keep errors and partial output if a run ends early. A full
run prints `measure_complete cases=20`.

## Build and run

From the Winum checkout with MinGW for both architectures:

```bash
bash scripts/build-measure-bundle.sh /tmp/stale-bits-measure \
  docs/wine-repro/user32-setwindowpos-stale-bits/measure.list
(cd /tmp/stale-bits-measure && sha256sum -c SHA256SUMS)
```

For Windows, download the `user32-stale-bits-measure` artifact from the
workflow, verify `SHA256SUMS`, and copy the extracted folder to a new
directory in the offline Windows 10 VM. Run `run-measure.bat` from the
interactive desktop, with the test window unobstructed and the mouse and
keyboard idle. Return only the two `result-stale-bits-*.txt` files and
`exitcodes.txt`.

CI runs the same executables in fresh WoW64 prefixes using a 1280 x 800
Xvfb screen at 96 DPI. The vanilla Wine 11.22 source is hash-verified before
building. The Winum test build comes from
[CI run 45064416572](https://github.com/Harvi654/winum/actions/runs/45064416572).
For a local Wine run with Xvfb and xauth installed:

```bash
xvfb-run -a -s '-screen 0 1280x800x24 -dpi 96 -nolisten tcp' bash \
  docs/wine-repro/user32-setwindowpos-stale-bits/run-wine.sh \
  /path/to/wine /path/to/wineserver /tmp/stale-bits-measure /tmp/stale-bits-results
```

## Results

The native Windows measurements determine the expected behavior for any
later Wine test.

| Cases | Vanilla Wine 11.22 | Winum 11.22-2 test build | Windows 10 |
|---|---|---|---|
| G, initially drained, full cover (both thread layouts) | CI pending | Green/green. Both samples invalidated. Then blue/blue. | Pending |
| G, initially drained, half cover (both thread layouts) | CI pending | Green/blue. Left sample invalidated. Then blue/blue. | Pending |
| G, initially drained, no cover (both thread layouts) | CI pending | Blue/blue. No sampled point invalidated. Remains blue/blue. | Pending |
| G, initially unpumped (all overlaps and thread layouts) | CI pending | Green/green. Both samples invalidated. Then blue/blue. | Pending |
| A | CI pending | Green until the final drain, then blue. | Pending |
| B | CI pending | Hidden move leaves green with no update. Show invalidates both samples, then drain paints blue. | Pending |
| C, E_direct | CI pending | Green through move, FRAMECHANGED and drain. No update or new paint at destination. | Pending |
| D, E_parent | CI pending | Parent move transfers blue. FRAMECHANGED and drain leave it blue without new container paint. | Pending |
| F_noredraw, F_nocopybits | CI pending | Green before final drain, then blue. | Pending |

For G, each cell gives the two destination pixels immediately after the move,
update membership at those points and the colors after the final drain.
[Winum results for both architectures](results/winum-11.22-2/) from
[CI](https://github.com/Harvi654/winum/actions/runs/45097935771/job/137441980528)
agree on these observations. No tested G variant copied red from the ribbon
into either destination sample.
