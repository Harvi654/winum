# comctl32-header-null-layout.patch

<!-- winum-test: l1 -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `comctl32`
- **Origin:** Solidworks-for-Linux (private for now), `patches/`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

SOLIDWORKS crashed at start (`HDM_LAYOUT` with a NULL layout).

## Cause and fix

`HDM_LAYOUT` with a NULL pointer fails instead of dereferencing it.

## Wine test

None in the patch.

## Validation

| Level | Result |
| --- | --- |
| L0 | full builds full-1/full-2; winum-11.20-1 series (promoted 2026-11-11): every entry applies with `--fuzz=0` on the official 11.20 archive; the tree is identical to that of `test-run/all-6` (run 42997560598) |
| L1 | batch1, both arch (promoted file: `comctl32-header-null-layout-v2`) |
| L2 | not recorded; winum-11.20-1: ok |
| L3 | v5/v6 null/valid layout checks (x64) |
| L4 | SOLIDWORKS starts; combined smoke; winum-11.20-1: Inventor smoke; SOLIDWORKS deferred |
| L5 | in use since 2026-10-28 (maintainer to confirm) |
| Cross review | Codex, approve |

## Open points

None recorded.

## Evidence

Solidworks-for-Linux `docs/wine-repro/header-layout/README.md`; overview in `docs/patch-status.md`.

## History

- **2026-11-11, promoted** into `patches/series` for winum-11.20-1 from `comctl32-header-null-layout-v2` (maintainer approval of `docs/release/winum-11.20-1.md` at `ef64451`).
