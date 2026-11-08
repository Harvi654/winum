# shell32-automation-zip-copy.patch

<!-- winum-test: l1 -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `shell32`
- **Origin:** Solidworks-for-Linux (private for now), `patches/`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

SOLIDWORKS installation: ZIP payloads were not extracted (`Folder::CopyHere` from a ZIP).

## Cause and fix

Shell automation `CopyHere` copies from stored and deflated ZIP archives.

## Wine test

`dlls/shell32/tests/shelldispatch.c`

## Validation

| Level | Result |
| --- | --- |
| L0 | full builds full-1/full-2 |
| L1 | Windows 10 VM (Mac host), L1 bundle of run 42483162074 (2026-11-08): shell32 shelldispatch x86_64 480 / i386 466 checks, 0 failures |
| L2 | not recorded |
| L3 | x64 matrix of 16 positive/negative cases |
| L4 | installation complete; Windows comparison: all 19,632 Design files identical (SHA-256) |
| L5 | in use since 2026-10-28 (maintainer to confirm) |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

Large implementation; ZIP64, encryption, other methods unsupported.

## Evidence

Solidworks-for-Linux `docs/wine-repro/shell-zip/implementation.md`; overview in `docs/patch-status.md`.

