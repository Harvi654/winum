# Windows 10 input component observations (2026-11-09)

Maintainer's offline Windows 10 VM, own MEASURE bundle from Actions run
42659893206, source `e0421f4`. This directory contains only structured
public-API measurements and the batch exit codes, not a Wine/application
debug trace. Both architectures completed all 44 scenarios with zero
harness errors and exit 0. After normalizing the single bits=32/64 field,
the returned files are identical.

Before committing, every line was parsed against the known record/field
schema. Scenario and stage names, A/B/C/D and child window roles, numeric
results/errors and the fixed LEGEND were checked. No machine/user names,
e-mails, paths or other identifiers were found. The full returned bytes
are retained; SHA256SUMS records them. No vendor window text was captured.

There are 592 distinct scenario/stage checkpoints, 2352 successful
GetGUIThreadInfo observations and 52 explicit detach calls per
architecture. Expected test values are copied from those observations,
not from an attachment graph model or Wine's implementation.

These are MEASURE results, **not** an assertion L1 pass. The standalone
and Wine assertion bodies need their own native x86_64/i686 run.
