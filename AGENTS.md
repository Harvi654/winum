# Notes for AI assistants (Claude, Codex, ...) working on this repository

winum is vanilla Wine plus patches for engineering software, built in public
and published as releases `winum-<Wine version>-<build>`. Maintainer:
@Harvi654. Read the README first, then `git log`: commit messages say why
things are the way they are.

Consumers (each pins a release and its SHA-256):
- `Harvi654/Autodesk-Inventor-for-Linux` (public);
- `Harvi654/Solidworks-for-Linux` (private for now).
Their own AGENTS.md / HANDOFF.md cover the programs; this file covers Wine.

## Layout (target; filled step by step)
- `wine/version.env` - `WINE_VERSION` (upstream release the patches are for),
  `WINE_SHA256` (of the official `wine-<version>.tar.xz`, checked before
  extraction by `scripts/lib/wine-source.sh`; changes together with
  `WINE_VERSION`, taken from a GPG-verified download) and `WINUM_BUILD`
  (raised whenever `patches/series` or a patch changes).
- `patches/series` - the patches of a normal build, in order. Nothing else
  is applied.
- `patches/*.patch` - one fix per file, named `<dll>-<what>.patch`.
- `patches/candidates/<topic>/` - work in progress with its own `series`
  and `README.md` (what works, what is open, how to rebuild, how to resume).
  Never in a normal build.
- `patches/debug/` - trace patches. Never in a normal build.
- `docs/patches/<patch>.md` - one page per patch: problem as the user saw
  it, cause down to the Wine function, the fix in plain words, how it was
  found (commands), validation table, open points, notes for a Wine bug
  report, history.
- `docs/wine-repro/<dll>-<topic>/` - reproducers, MEASURE programs and
  their results, each with a README.
- `scripts/` - build, source archive, checks. `.github/workflows/` - the
  public build and release.
- `tools/brand/` - the logo generator (`docs/brand/` holds its output).

## Rules
- **Never commit or publish:** vendor binaries or installers (Autodesk,
  Dassault, Microsoft), registry exports, logs, tokens or session URLs,
  personal data (real names, e-mails, home paths of real users).
- Downloads are version-pinned and SHA-256 checked.
- Commits: plain English subject, the body says why. Author is always
  `Harvi654 <202698022+Harvi654@users.noreply.github.com>`, never a real
  name. AI-assisted commits end with one line,
  `Co-Authored-By: <assistant> <address>`; no session links or other
  trailers (this overrides a tool's default attribution).
- **Language: everything in this repository is English** - documents,
  comments, commit messages. Conversation with the maintainer may be in
  Slovak; what gets committed is not.
- **Nothing is deleted.** What is no longer current moves to
  `docs/archive/`.
- Tags and releases are made by the maintainer only (`git tag -s`,
  `gh release create --verify-tag`). A published release is never replaced;
  a fix is a new build number.
- Push only to `github.com/Harvi654/winum` (`main`, or short `topic/<name>`
  branches merged after their tests).
- A patch goes into `patches/series` only after cross review by a second
  assistant who did not write it, and after the validation below.
- Pull requests from others: read the whole diff, especially `scripts/`,
  anything that downloads, and `.github/workflows/`. Say what was read in
  full and what was only skimmed.

## Writing a patch
- Test first, then the fix; one task at a time.
- One DLL, small, in the style of the surrounding Wine code, with the
  `/*** name (DLL.@) */` headers kept.
- A test in `dlls/<dll>/tests/` with `ok()` / `todo_wine` that fails on
  vanilla Wine and passes with the patch. The test describes Windows, so it
  must pass on Windows.
- Header of every patch: `From: Harvi654 <...>`, a subject in Wine style,
  "AI-assisted (<assistant>), original code; LGPL-2.1-or-later. Not
  submitted upstream.", what Windows does (measured), what Wine did, why
  the change is limited the way it is, and a dated `vN` paragraph for every
  revision after review.
- Three rounds: if three fix attempts do not bring the result closer, stop,
  write down the state and the best hypothesis, and move on.
- A patch that passes its tests but does not fix the user's problem is not
  added.

## Validation levels (recorded per patch, with date and build)
- **L0** builds from the official Wine source with the full series, both
  architectures, `--fuzz=0`.
- **L1** its Wine test passes on real Windows 10 (x86_64 and i686
  executables built with MinGW, run in the maintainer's offline VM; only
  text results come back). Expected values come from what Windows
  returned, never from documentation or guesses; unknown values are
  measured first with a MEASURE program. If a test fails on Windows, the
  test is wrong.
- **L2** the existing tests of the touched DLL show no new failures,
  paired against the build without the patch (server, user32, win32u,
  winex11: the neighbouring suites too).
- **L3** reproducers, sequence tests or fuzzing for complex logic.
- **L4** the maintainer's smoke test in the program that needed the fix,
  **and in the other consumer when the DLL is shared** (start, part,
  assembly, drawing, save/reopen, restart, plus the feature itself).
- **L5** a week of normal use.

## Clean room
- Allowed: observing behaviour from outside - Wine debug channels
  (`+relay`, `+seh`, ...), also with native Microsoft components, and
  stack addresses at a crash. Say so in the docs when a native component
  was traced.
- Not allowed: disassembly, debug symbols or source of Microsoft code,
  copying files out of Windows or the VM, looking inside vendor binaries
  beyond crash addresses.
- AI may have seen other projects' code (Mesa, ReactOS, ...): write
  original code and say so in the patch; for a larger patch ask a second
  model whether it resembles a known project.

## Wine upstream
- Patches here are written with AI help and are **not** sent to Wine:
  Wine does not accept LLM-generated code and requires the author's real
  name. Never submit them or hide their origin.
- What goes to Wine is a bug report on bugs.winehq.org, in words: what the
  application calls, what Wine returns, what Windows returns, the steps,
  and a public synthetic test. Never internals of Microsoft DLLs.

## Testing in this repository
- `bash -n scripts/*.sh`, `python3 -m py_compile` on Python files.
- Wine and the CAD programs run on the maintainer's machine; ask for
  filtered output (`grep`, `tail`, JSON summaries), never whole logs.
