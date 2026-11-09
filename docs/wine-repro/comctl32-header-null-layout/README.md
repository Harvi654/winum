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
