# Batch 3: native observations before Wine assertions

Original measurements, AI-assisted (Codex), LGPL-2.1-or-later. Programs are
prepared sequentially, one patch per commit, and added to both measure.list
here and the main docs/wine-repro/measure-bundle.list. Native expected values
remain unknown until the maintainer returns the offline Windows outputs.
No production implementation, validation marker or patches/series changes
are made by preparing a measurement.

Current entries:

- msi-null-stream: original compound-storage transform with a NULL OBJECT
  row and populated binary control; [scope](../msi-transform-null-stream/README.md).
- msi-tree-property: own MSI dialog with direct-directory/indirect browse
  bindings and own-process tree automation; [scope](../msi-selection-tree-directory-property/README.md).

Build one final bundle after all batch entries are prepared:

```sh
export BATCH3_MEASURE="$HOME/.cache/winum-batch3-$(date +%Y%m%d-%H%M%S)"
bash scripts/build-measure-bundle.sh "$BATCH3_MEASURE" docs/wine-repro/batch3-measure/measure.list
```

Copy the complete output directory to a fresh directory in offline Windows
10 and run run-measure.bat once. Return all result-*.txt and exitcodes.txt,
including failed/partial processes. A ZIP with only these text files is fine.
No vendor MSI, patch, DLL or application is used. Do not copy Windows files
back. Unknown API results are observations, not guessed assertions or L1.

The shared measure-support.h bounds each process with an independent
watchdog, reports setup/cleanup errors, and confines file writes to unique
own temporary directories. A timeout exits 2 without requiring stdio or
DLL shutdown on a possibly stuck thread; it need not print SUMMARY.
Successful observations print SUMMARY complete=1 expectations=none. Each
topic page also specifies which positive controls are necessary before
treating that completed output as usable measurement coverage.
