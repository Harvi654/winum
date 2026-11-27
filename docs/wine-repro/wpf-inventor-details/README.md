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

Use the normal Inventor launcher and existing Wine prefix. The runner stages
the probe and keeps the logs in a new private directory on the prefix C: drive.
Open the file dialog, select a part, wait for the preview with Options selected,
then click Details and wait two seconds. Close Inventor and keep the
short measurement printed by the runner. Keep application logs local.

The checked-in assembly is built from StartupHook.cs with Roslyn from the
.NET SDK 10.0.100 targeting .NET 10.0. It is an AnyCPU library. The
measurement logic was exercised against a synthetic tree. The first
Inventor run is pending.

## Investigation status (2026-11-27)

The Open dialog in Inventor 2027 on winum-11.22-3 shows the preview, but
the metadata block under Details is blank. The Windows reference displays
that block. Live Visual Tree identified the WPF path as
`FileDialogUI > MoreInfoTabControl > PART_SelectedContentHost > FileProperties`.
The TextBlock is bound in the `FileDialogData` context. The evaluated Text
contains the complete formatted block, including labels and values, and
the control measures 169 by 187 device-independent pixels.

Wine creates and shows the WPF Open window. Forcing WPF software rendering
did not restore the text. The probe wrote no sample. The .NET host trace
confirmed .NET 10.0.9 initialized through a runtime delegate and received
the probe path in STARTUP_HOOKS; this did not confirm that Initialize ran.

In the timed COM trace, selecting a part read structured storage in an
earlier capture and retried a local-server activation for CLSID
`{daa52b27-8897-50af-ada5-c6c71bb64e17}` in the timed run. The same CLSID
was absent from both Windows registry views, where Details worked. The
click window showed no COM errors. Two earlier MSCOREE errors coincided
with loading other Inventor add-ins, about 21 seconds before selection.
These traces do not establish a connection between those errors and Details.

Next: compare the same part's Part Number in iProperties on Wine, then
measure the WPF TextBlock's evaluated text, visibility and arranged size
to distinguish a missing data update from layout or rendering. The
source property of the binding and the cause remain open.
