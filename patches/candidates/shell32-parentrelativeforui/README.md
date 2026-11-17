# shell32-parentrelativeforui candidate

See [the patch page](../../../docs/patches/shell32-parentrelativeforui.md)
and [the Windows measurements](../../../docs/wine-repro/shell32-parentrelativeforui/README.md).
The first patch adds the test with `todo_wine` on the unsupported
requests. The second patch implements the behavior and removes those
markers. Apply the patches in the order in `series`.

The `test-run/shell32-parentrelativeforui-1` branch appends both patches
to the winum-11.22-1 series and uses the Build Wine workflow to produce a
candidate binary, corresponding source and SHA-256 files. The first
consumer check is Inventor 2027 Open at the drive root and one subfolder.
If names are still missing, return to the dialog trace. If names appear,
run the offline Windows test, paired shell32 suites and Inventor smoke
tests in both versions before cross review.
