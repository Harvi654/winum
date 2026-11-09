# HDM_LAYOUT with NULL: Windows MEASURE

Original, AI-assisted (Claude), LGPL-2.1-or-later. Patch:
`patches/comctl32-header-null-layout.patch` (`HEADER_Layout` returns FALSE
for a NULL `HDLAYOUT` instead of dereferencing it; SOLIDWORKS crashed at
start). Read before writing: the patch and Wine 11.20
`dlls/comctl32/header.c` (`HEADER_Layout`) and `tests/header.c`.

Measured, comctl32 v5 and v6, header styles `HDS_BUTTONS` and
`HDS_BUTTONS | HDS_HIDDEN`, `SendMessageA` and `SendMessageW`:

- `valid`: a normal `HDLAYOUT` (control: return value, `WINDOWPOS` and the
  changed `RECT`);
- `null_layout`: `lParam` NULL - the case the patch changes;
- `null_prc`, `null_pwpos`, `null_both`: an `HDLAYOUT` with NULL members,
  neighbours the patch does not touch (they may crash on Windows too).

Every case runs in its own child process (`../batch1-measure/comctl-support.h`):
`CALL`, then `RETURN ret=... error=...` (last error, preset to `deadbeef`),
`STATE` and `AFTER count=...` (the control still answers), or
`EXCEPTION code=...` and a `CHILD_END ... exit=` with that code. A
case that crashes on Windows cannot become a Wine test; only the measured
non-crashing ones are tested.

Local check (Wine 9.0, not an oracle): all four NULL variants crash
(`c0000005`), the valid layout returns 1.

## Windows observations (2026-11-09)

Offline Windows 10 VM, `windows-2026-11-09/` (both architectures complete,
`api_errors=0`, no exceptions; identical apart from pointer width).
comctl32 5.82 and 6.16, A and W, `HDS_BUTTONS` with and without
`HDS_HIDDEN` behave the same:

- `valid`: returns 1; `WINDOWPOS` x=5, y=7, cx=200, cy = header height
  (20 in v5, 24 in v6; 0 with `HDS_HIDDEN`), `hwndInsertAfter` NULL,
  flags `SWP_NOZORDER | SWP_NOACTIVATE` (0x14); `prc->top` moves down by
  the height (not with `HDS_HIDDEN`).
- `null_layout`, `null_prc`, `null_pwpos`, `null_both`: no crash, returns
  0, last error unchanged (`deadbeef`), the given `RECT` and `WINDOWPOS`
  untouched (0xcc fill kept); the control still answers.

So Windows checks both members as well, before writing anything; patch v1
checked only the `HDLAYOUT` pointer. Wine also sets only `SWP_NOZORDER`
(0x4) in the valid case - a separate difference, not part of the patch.
