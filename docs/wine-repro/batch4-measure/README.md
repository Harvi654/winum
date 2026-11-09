# Batch 4 input component measurement

`measure.list` deliberately contains only input-components. Build and run
instructions, scope and output interpretation are in
`../server-input-attachment-components/README.md`.

The Windows 10 x86_64/i686 MEASURE output is now recorded in the topic's
windows-2026-11-09 directory after privacy/schema checking. The v2 candidate
adds its native-derived Wine test; l1.list builds only its unchanged
standalone assertion body for the next native run. Use that list as the
input to the main Build MEASURE bundle workflow, or the dedicated L1 build
script (which also checks candidate/body parity). The older batch-specific
workflow is left unchanged.
The earlier server mousemove candidate is a separate task and is not
included in this bundle.
