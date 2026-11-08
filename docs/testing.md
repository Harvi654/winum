# Testing: what we do and what else is worth doing

Written 2026-11-08 after reviewing a general overview of test methods
(regression, differential, GUI/E2E, fuzzing, matrix, performance/soak,
smoke). It records which of them fit this project, which we already do
under another name, and which do not fit. The validation levels L0-L5
themselves are defined in `AGENTS.md`; this file does not change them.

## Already in place

| Method (general name) | Here | Where |
| --- | --- | --- |
| Regression testing | **L2**: Wine's own test suites of every touched DLL, vanilla against the series, paired, in CI (`test-series`, five groups per variant); failures moved by added test lines are told apart from new ones | `scripts/test-series.sh`, `scripts/compare-tests.py`, `docs/wine-repro/l2-series-*.md` |
| Differential / oracle testing | **MEASURE programs and L1**: the same program on Windows 10 and on Wine; Windows' answer is the expected value, never documentation | `docs/wine-repro/*/`, `scripts/build-l1-*.sh` |
| Minimal reproducer (for Wine bug reports) | reproducers without vendor code, in words in the report | `docs/wine-repro/*/` |
| Fuzzing / sequence tests | **L3** for complex logic (MSI feature states, ole32 storage threads, input attachment) | per patch page |
| Smoke test in the real program | **L4**, by the maintainer, in Inventor (and SOLIDWORKS when the DLL is shared) | per patch page |
| Soak testing | **L5**: a week of normal use | per patch page |
| Rule that every change has a test | `scripts/check-series.sh`, enforced for release tags | `AGENTS.md` |

## Worth adding (in order of value for effort)

1. **CI smoke run of the built Wine.** After the build, without any vendor
   files: `wineboot`, our MEASURE programs and reproducers that do not need
   a GPU, a small MSI package built by our own test code (exercises the MSI
   patches end to end), a COM round trip. Catches a broken build or a
   prefix that does not start before anyone downloads it. Cheap; runs in
   the existing CI image under Xvfb.
2. **Installer matrix in containers (Inventor repository).** The install
   scripts without Autodesk files on Arch, Ubuntu, Fedora (and openSUSE
   when added): dependency checks, Wine download and SHA-256 check, prefix
   creation, DXVK/vkd3d-proton placement, and - once prefix migrations exist
   - **updating an old prefix** step by step versus a clean one. This is
   where updates for existing users can break unnoticed.
3. **Scripted L4 through Inventor's own automation API.** Instead of
   clicking with xdotool (fragile with themes, scaling and timing), a small
   script drives Inventor through its public COM API: create a part,
   sketch, extrude, save, reopen, export PDF/STEP, run a Stress Analysis
   study on a fixed model, check the results. Runs on the maintainer's
   machine (Autodesk software never runs in CI); makes L4 repeatable and
   quick for every new build. Uses only Inventor's documented API, nothing
   from inside its binaries.
4. **Reference outputs, Windows against Linux.** For a few own test models:
   mass properties, a STEP export, Stress Analysis results, a 3D PDF,
   produced on Windows 10 in the VM and on Linux, compared by value with
   tolerances (files are never bit-identical: time stamps, GUIDs). Shows
   silent wrong results, which a crash test does not. The models and the
   numbers are ours; no vendor files are copied out of the VM.
5. **Timings per build.** A handful of numbers recorded with every winum
   build in the maintainer's smoke run: Inventor start, opening a large
   assembly, 3D PDF export (7 min -> 15 s showed why this matters), Stress
   Analysis solve. A change of more than ~20 % is looked at before release.
6. **Hardware matrix by testers.** A short, fixed checklist that testers
   with other GPUs/drivers run and report (NVIDIA, Intel, hybrid laptops,
   PRIME offload - the GL + D3D12 hang on PRIME was found this way).

## Does not fit, or not now

- Sending patches or AI-written tests to Wine upstream: not done, by rule
  (`AGENTS.md`); only bug reports with public reproducers go upstream.
- Pixel-matching GUI automation of Inventor in CI: Autodesk software cannot
  run in public CI, and screenshot comparison breaks with every theme,
  scaling or driver change. Item 3 covers the need better.
- Bit-for-bit comparison of exported files: not meaningful (see item 4).
- Fuzzing Inventor itself with damaged files: finds Inventor's own bugs,
  not Wine's; out of scope.
- Generic test frameworks from web development (Selenium, Cypress, JUnit):
  not applicable to Wine's C test suites.
