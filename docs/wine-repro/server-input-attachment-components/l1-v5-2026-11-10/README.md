# V5 native L1, Windows 10 (2026-11-10)

Maintainer-returned native unit results from the series L1 bundle of
[test-run/batch4-4, run 42772336963](https://github.com/Harvi654/winum/actions/runs/42772336963),
commit ee0773582e02308f8368a444b4f909de2c2eab15. This runs the same v5
user32:attachment assertion body as the paired Wine unit, with Wine todos
ordinary on Windows. SHA256SUMS preserves the three uploaded files byte
for byte. All contents were read and checked for personal machine/user
names, home paths, email addresses and secrets; none were found. The
hexadecimal thread prefixes and fixture source lines are not user names.

Both x86_64 and i386: 16719 checks, zero todos/flaky checks/failures/skips,
exit 0. Each completes 44 scenarios with zero scenario skips, 592 component
and 592 absolute snapshots, absolute_state=1, and 2180 positive checks.
Thus both observation scopes execute natively, including the exact
first-child C-window choices that remain narrow todos on Wine. This is
v5 L1, not a transfer from v2/v4.

The earlier batch1 unit attachments in this conversation are unrelated;
the corrected three files above are the evidence used here. No vendor
binary, registry export or personal path is stored.
