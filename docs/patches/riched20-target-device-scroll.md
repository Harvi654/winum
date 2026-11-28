# riched20-target-device-scroll

<!-- winum-test: l1 -->

Keep the horizontal scroll range large enough for the target device.

Promoted from the [reviewed candidate](https://github.com/Harvi654/winum/blob/04717ba/patches/candidates/riched20-target-device-scroll/README.md).
The range and scroll position checks remain strict.

| Level | Result |
| --- | --- |
| L0 on 11.23 | The full 84-patch series applied with `--fuzz=0`; both architectures [built before the test-only v3](https://github.com/Harvi654/winum/actions/runs/46081050369). |
| L1 | Offline Windows 10, 2026-11-26: all 83 checks passed per architecture. The visibility assertion remains active on Windows. |
| L2 on 11.22 | Both variants in [run 45865705690](https://github.com/Harvi654/winum/actions/runs/45865705690) already failed `editor.c:9715` six times per architecture, so the paired comparison hid the shared failure. It reported that `9699` (range) and `9711` (scroll position) were fixed. |
| L2 on 11.23 | [Run 46081052433](https://github.com/Harvi654/winum/actions/runs/46081052433) reported the visibility failures because the vanilla variant did not contain this test. The v3 paired rerun is pending. |
| L4 | The [combined 11.22 overlay](https://github.com/Harvi654/winum/actions/runs/45880891853) passed Inventor and AutoCAD text editing and Revit New Project. In Inventor, the letter starts past the edge after focus, and a small scroll reveals it. This is the same open point as `editor.c:9715`. The 11.23 smoke is pending. |
| Cross review | Approved on the candidate branch. |

## Open point

Hypothesis: `sizeWindow.cx` includes the vertical scroll bar with
`ES_DISABLENOSCROLL`. For a 610 px control, the frame leaves 608 px and
a 17 px scroll bar leaves a 591 px client area. If
`editor_ensure_visible` scrolls the caret to `sizeWindow.cx - 1`
(607 px), a roughly 7 px glyph starts at x=600, matching the CI output.
A separate measurement should compare `GetClientRect` with the
post-focus glyph position with and without `WS_VSCROLL` and
`ES_DISABLENOSCROLL`.

## History

v3 (2026-11-28): Add `todo_wine_if` to the visibility assertion only,
for the cases that scroll on focus: 42211 twips centered and right
aligned, and 100000 twips centered, in both RichEdit classes. This
covers six expected todos per architecture. The range and scroll
position assertions, Windows expectations and implementation are unchanged.
