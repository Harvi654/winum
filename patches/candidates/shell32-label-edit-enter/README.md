# Shell view inline name confirmation

Apply the patches in `series` order. The first adds the measured Wine
test with `todo_wine`. The second implements the behavior and removes
those markers.

The test-run branch includes the previously tested breadcrumb fix and
both New submenu candidate patches. Its baseline for paired shell32
tests includes the Enter test-only patch. The patched variant also
includes the Enter fix. Build Wine produces a candidate tarball.
The validation workflow supplies `shell32-label-edit-enter-l1` with
both Windows test executables and `run-tests.bat`.

See [the patch page](../../../docs/patches/shell32-label-edit-enter.md)
and [the trace and native measurements](../../../docs/wine-repro/shell32-label-edit-enter/README.md).
