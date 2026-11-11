# msi-transform-null-stream.patch

<!-- winum-test: l1 -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `msi`
- **Origin:** Autodesk-Inventor-for-Linux, commit `d835bb3`, first shipped in `wine-11.19-1`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Inventor 2026.4 update: `Inventor.exe` died at start (0xc06d007f) because 570 patched files stayed at 2026.0.

## Cause and fix

Patch rows whose header lives in `MsiPatchHeaders` (NULL `Header` stream) are no longer dropped when a transform is applied.

## Wine test

None in the production patch. The separate
[v2 candidate](../../patches/candidates/msi-transform-null-stream-v2/README.md)
adds a Windows-derived test in dlls/msi/tests/db.c. Its original transform
contains a NULL OBJECT row and a populated four-byte binary control; Windows
retains both rows. The exact-body standalone assertion test passed native
L1 on both architectures. Cross review and paired MSI L2 remain pending;
the candidate is not yet in patches/series, so the production marker remains
missing.

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.19-1` (2026-10-19, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06); winum-11.20-1 series (promoted 2026-11-11): every entry applies with `--fuzz=0` on the official 11.20 archive; the tree is identical to that of `test-run/all-6` (run 42997560598) |
| L1 | 43/0, both arch (2026-11-09) (promoted file: `msi-transform-null-stream-v2`) |
| L2 | not recorded per patch; winum-11.20-1: ok (`msi:db`) |
| L3 | not recorded |
| L4 | 2026.4 update installs and starts. (Inventor, maintainer); winum-11.20-1: Inventor 2026.4 update (v1 code, unchanged) |
| L5 | in Inventor's runtime since `wine-11.19-1` (2026-10-19); a week of use not formally recorded - maintainer to confirm |
| Cross review | Claude, approve |

## Open points

Promote the native-tested v2 candidate only after cross review, paired MSI L2
and the remaining consumer gates. Observed vanilla/patched runtime results
have not yet been recorded.

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).

## History

- **2026-11-11, promoted** into `patches/series` for winum-11.20-1 from `msi-transform-null-stream-v2` (maintainer approval of `docs/release/winum-11.20-1.md` at `ef64451`). Open: workaround wider than measured (any failed `OpenStream`).
