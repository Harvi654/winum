# mspatcha-pa19-32bit-and-large-files.patch

<!-- winum-test: missing -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `mspatcha`
- **Origin:** Autodesk-Inventor-for-Linux, commit `4a1a803`, first shipped in `wine-11.19-1`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Inventor 2026.4 update: PatchFiles failed with 1603 on .NET assemblies and on old files larger than 8 MB.

## Cause and fix

Wine MR 3870 (Aidan Khoury, 2023, never merged) ported to 11.19: 32-bit PE patching; plus old files larger than the LZXD window. Not original code of this project.

## Wine test

None in the production patch. The separate
[v2 signature candidate](../../patches/candidates/mspatcha-signature-v2/README.md)
adds an original Wine test for the Windows-measured signature/normalization
API subset. Its exact-body standalone test passed 50 checks on Windows 10
x86_64/i686. It does not apply or decode PA19 data, so this is not full-patch
L1 and does not close the missing producer/decoder fixture gate.

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.19-1` (2026-10-19, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06) |
| L1 | 2026-11-09: candidate signature API subset passes on Windows 10 x86_64/i686, 50 checks / zero failures / exit 0 each (c50734d sources). PA19 decode/reference-window coverage is absent; full-patch L1 remains open. |
| L2 | not recorded per patch |
| L3 | not recorded |
| L4 | 2026.4 update installs. (Inventor, maintainer) |
| L5 | in Inventor's runtime since `wine-11.19-1` (2026-10-19); a week of use not formally recorded - maintainer to confirm |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

Third-party origin (Wine MR 3870), LGPL like Wine. The original reproducible
PA19 producer and decoder/reference-window fixture remain missing. A large
raw-buffer signature is not a large-reference-window test. No l3-only
exception is claimed. The native-tested signature candidate still needs
formal cross review and observed paired Wine runtime; it is not promoted.

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).

