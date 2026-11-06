# Patches

`series` lists the patches of a normal winum build, in the order they are
applied to vanilla Wine (`wine/version.env`), without fuzz. A `*.patch` file
that `series` does not name is not applied. Subdirectories are never
applied:

- `candidates/<topic>/` - work in progress, with its own `series` and a
  `README.md` (what works, what is open, how to build it, how to resume);
  a promoted candidate's README becomes its page in `docs/patches/` and its
  `series` goes to `docs/archive/candidates/`;
- `debug/` - trace patches for finding a problem;
- `drafts/` - unfinished patches.

How a patch gets into `series` (test on Windows, levels L0-L5, cross review)
is in [AGENTS.md](../AGENTS.md); each patch gets a page in `docs/patches/`.

## Where the patches come from

| Patches | From | Validated there |
| --- | --- | --- |
| the first 27 (`combase-…` to `winex11-xinput-bad-device`) | Autodesk-Inventor-for-Linux, `patches/*.patch` at commit `a41b275`, = its Wine release `wine-11.20-6` | Inventor 2026/2027 since October 2026 (release `wine-11.20-6`) |
| the next 13 (`server-input-attachment-components` to `win32u-fixed-negative-mat2`) | Solidworks-for-Linux (private for now), its `patches/series` at `work` 2026-11-06 | SOLIDWORKS 2026 since late October 2026 |
| A, B, C (`win32u-child-topmost-ignore`, `comctl32-themed-checkbox-buffered-paint`, `winex11-stale-map-activation`) | Solidworks-for-Linux, validated candidates (2026-11-02 to 11-05) | SOLIDWORKS 2026 |
| the 17 `msi-*` feature-selection patches | Solidworks-for-Linux, written by Codex, cross-reviewed by Claude (2026-11-05) | SOLIDWORKS and Inventor installations |
| `oleaut32-typelib-keep-released`, `opengl32-window-format-color-bits` | winum candidates (2026-11-06), cross-reviewed by Codex | Inventor 2026/2027 |

Changed here since: `oleaut32-typedesc-pointee.patch` v2 (2026-11-06, no
leak; see its page in `docs/patches/`).

Their history (why each was made, how it was tested) is in the source
project's `git log` and documents; every patch has a status page in
`docs/patches/` that summarises it and links there, and
[docs/patches/README.md](../docs/patches/README.md) is the overview of all
levels. The dates in `PATCHES.txt` of a winum build are the dates of
the last change in this repository.

`wbemprox-timezone.patch` comes from the original project
[cakelesscoder/inventor-on-linux](https://github.com/cakelesscoder/inventor-on-linux).
`mspatcha-pa19-32bit-and-large-files.patch` contains Wine merge request
3870 by Aidan Khoury (2023, not merged), ported to 11.19.
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
