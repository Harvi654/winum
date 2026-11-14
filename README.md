<p align="center">
  <img src="docs/brand/winum-wordmark.png" alt="winum - Fortified Wine for engineering software" width="100%">
</p>

**winum** is a build of [Wine](https://www.winehq.org/) with fixes for
engineering software: CAD, CAE and the installers and add-ins around them.
It is vanilla Wine plus a small, documented set of patches, built in public
and published as versioned releases.

*Winum* is how the Romans pronounced *vinum*, wine.

> **Status (14 November 2026).** Current release:
> [`winum-11.20-2`](docs/release/winum-11.20-2.md) - Wine 11.20 with 66
> patches: the Inventor project's former `wine-11.20-6` series, the
> SOLIDWORKS fixes, 17 MSI feature-selection fixes, and four patches for
> Inventor's Import CAD (contributed by @krokosik). Every patch in
> `patches/series` has a Wine test that passed on Windows 10 (L1) or a
> recorded reason why not; `scripts/check-series.sh` checks it.
> On `main` since that release: v6 of `ole32-createfromfile-package`
> (winum PR #2), for the next build. Next: the move to Wine 11.22 - the
> series applies to it with `--fuzz=0` except one test hunk - and the
> candidate `wldap32-winsock-cleanup` (L0 and L2 done, L1 and cross review
> open).

## Who uses it

- [Autodesk Inventor on Linux](https://github.com/Harvi654/Autodesk-Inventor-for-Linux) (2026, 2027)
- Autodesk AutoCAD (2027) (not public yet, currently in development, public release planned)
- Autodesk Revit (2027) (not public yet, currently in development, public release planned)
- SOLIDWORKS 2026 on Linux (not public yet, currently in development, public release planned)

The projects download a pinned winum release and check its SHA-256. One
Wine for all of them: a patch that changes a DLL several of them use is
tested in each before it is released (or its test there is recorded as
deferred, see AGENTS.md).

winum is not affiliated with WineHQ, Autodesk or Dassault Systèmes. Wine is
a trademark of the Wine project; Autodesk, Inventor and SOLIDWORKS are
trademarks of their owners and are named here only to say what the build
was tested with. No program of theirs is part of this repository.

## What is in a release

- vanilla Wine `WINE_VERSION` (see `wine/version.env`) with the patches in
  `patches/series`, applied in that order;
- the complete source of exactly that build (`winum-<version>-<build>-source.tar.xz`);
- SHA-256 sums.

Release names are `winum-<Wine version>-<build>`, for example
`winum-11.20-1`. The build number goes up whenever the patches change; a
published release is never replaced.

Every push to `main` builds the series in public (GitHub Actions, "Build
Wine") as a check. A release is made only when the maintainer pushes a
signed tag `winum-<version>-<build>` that matches `wine/version.env`; the
workflow then builds from the tag and publishes the binary, the source and
a signed provenance attestation. To build it yourself:
`bash scripts/build-wine.sh` (Wine's build dependencies and mingw-w64,
about 10 GB and 1-2 hours).

## How a patch gets in

Every patch:

1. fixes one problem a real program shows, in one DLL, in the style of
   the surrounding Wine code;
2. comes with a test in Wine's own test suite that fails on vanilla Wine
   and passes with the patch, and whose expected values were measured on
   real Windows;
3. is reviewed by a second reviewer before it goes into `patches/series`;
4. has a short plain-language write-up in `docs/patches/`, with the
   validation level it reached (L0 build … L5 a week of real use).

Rules for contributors and AI assistants are in [AGENTS.md](AGENTS.md).

## About AI

Most patches here are written with AI assistance (Claude, Codex) and are
marked so. Wine does not accept AI-generated code, so these patches are
**not** sent to Wine. What goes to Wine is bug reports written in words,
with a public test anyone can run.

## License

The patches change Wine, which is LGPL, so they are
**LGPL-2.1-or-later** ([LICENSE](LICENSE)), as is everything else in this
repository unless a file says otherwise. The logo uses outlines of the
fonts Fraunces and IBM Plex Mono (SIL Open Font License 1.1), see
[docs/brand](docs/brand/README.md).
