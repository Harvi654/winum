# Inventor Open dialog Details measurement

This diagnostic branch provides an original .NET 10 probe for the Inventor
2027 Open dialog. It uses a .NET startup hook to observe the WPF visual
tree through public APIs on the WPF dispatcher. It samples the Details tab
and the FileProperties text block. It records state changes, text length,
height, visibility and whether the text includes the English Part Number
label. It does not log document names or metadata values.

Verify and run the bundle with Inventor closed:

```bash
sha256sum -c SHA256SUMS
INV_PREFIX=/path/to/prefix bash run-measure.sh /path/to/inventor-repo
```

Use the normal Inventor launcher and existing Wine prefix. The runner
requires the standard Z: mapping to the Linux filesystem root. Open the
file dialog, select a part, wait for the preview with Options selected,
then click Details and wait two seconds. Close Inventor and keep the
short measurement printed by the runner. Keep application logs local.

The checked-in assembly is built from StartupHook.cs with Roslyn from the
.NET SDK 10.0.100 targeting .NET 10.0. It is an AnyCPU library. The
measurement logic was exercised against a synthetic tree. The first
Inventor run is pending.
