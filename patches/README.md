# Patches

`series` lists the patches of a normal winum build, in the order they are
applied to vanilla Wine (`wine/version.env`), without fuzz. A `*.patch` file
that `series` does not name is not applied. Subdirectories are never
applied:

- `candidates/<topic>/` - work in progress, with its own `series` and a
  `README.md` (what works, what is open, how to build it, how to resume);
- `debug/` - trace patches for finding a problem;
- `drafts/` - unfinished patches.

How a patch gets into `series` (test on Windows, levels L0-L5, cross review)
is in [AGENTS.md](../AGENTS.md); each patch gets a page in `docs/patches/`.

## Where the patches come from

| Patches | From | Validated there |
| --- | --- | --- |
| the first 27 (`combase-…` to `winex11-xinput-bad-device`) | Autodesk-Inventor-for-Linux, `patches/*.patch` at commit `a41b275`, = its Wine release `wine-11.20-6` | Inventor 2026/2027 since October 2026 (release `wine-11.20-6`) |

Changed here since: `oleaut32-typedesc-pointee.patch` v2 (2026-11-06, no
leak; see its page in `docs/patches/`).

Their history (why each was made, how it was tested) is in that project's
`git log` and `docs/arch-hyprland.md`; the pages in `docs/patches/` will
link to it. The dates in `PATCHES.txt` of a winum build are the dates of
the last change in this repository.

`wbemprox-timezone.patch` comes from the original project
[cakelesscoder/inventor-on-linux](https://github.com/cakelesscoder/inventor-on-linux).
The other patches were written with AI help (Claude by Anthropic, Codex by
OpenAI) and tested by people on real machines. No Microsoft code,
disassembly or debug symbols were used.

## License

The patches change LGPL code, so they are licensed like that code: GNU
Lesser General Public License 2.1 or later ([LICENSE](../LICENSE), Wine's
`COPYING.LIB`).

Every release has its source next to the binary
(`winum-<version>-<build>-source.tar.xz`, made by
`scripts/make-wine-source.sh` from the release's tag): the patched tree with
`PATCHES.txt` and a short "Modified by winum" comment at the end of every
changed file (`scripts/lib/wine-patches.sh`, used by `build-wine.sh` too, so
the binary is built from exactly that tree).
