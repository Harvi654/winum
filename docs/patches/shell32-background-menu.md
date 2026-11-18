# shell32-background-menu.patch

- **DLL:** `shell32`
- **Status:** [candidate](../../patches/candidates/shell32-background-menu/README.md)
- **Target:** winum-11.22-2 (Wine 11.22), pending validation

## Symptom

The Open dialog in Inventor has no New submenu when right-clicking the
empty area of a filesystem folder.

## Cause and fix

Inventor's shell view queries its background menu with first command ID
0, last ID 0x7fff and `CMF_NORMAL`. Wine rebases command IDs by
subtracting 0x7000. The submenu ID 0x42 wraps out of range and Wine skips
the submenu before merging its children. Offline Windows 10 build
19045.6466 showed an enabled New submenu at the observed command range
on both x86_64 and i686.

Put the New popup ID in the resource's command ID range. Only show it
when the parent provides the folder creation helper. If folder creation
fails or produces no PIDL, skip selection and label editing. The
general merge algorithm and the other background popups are unchanged.

## Test

`dlls/shell32/tests/shlfolder.c` checks for an enabled submenu at each
measured first command ID. It checks the New caption in an English UI.
The [MEASURE results](../wine-repro/shell32-background-menu/README.md)
capture the initial menu structure. Actual folder creation is checked
in Inventor.

## Validation

| Level | Result |
| --- | --- |
| L0 | Candidate build queued on `test-run/shell32-background-menu-1` |
| L1 | MEASURE passed on x86_64 and i686. Windows regression test pending. |
| L2 | Paired shell32 tests pending |
| L4 | Inventor checks pending |
| L5 | Pending |
| Cross review | Pending |
