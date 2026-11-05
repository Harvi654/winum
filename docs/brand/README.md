# winum logo

The word *winum* in Fraunces SemiBold, with the **u** drawn as a cup filled
with wine. Colours follow GitHub's dark theme.

| File | Use |
| --- | --- |
| `winum-banner.svg` / `.png` | top of the README (1280×320) |
| `winum-social.png` | GitHub social preview (1280×640, Settings → Social preview) |
| `winum-icon.svg` / `.png` | avatar, favicon (512×512) |
| `winum-wordmark-dark.svg` | the word alone, for dark backgrounds (transparent) |
| `winum-wordmark-light.svg` | the word alone, for light backgrounds (transparent) |

| Colour | Hex |
| --- | --- |
| background (GitHub dark) | `#0D1117` |
| text | `#F0F6FC` |
| tagline | `#8B949E` |
| wine | `#A3263C` |

All files are made by `tools/brand/make-logo.py` (letters converted to
outlines, so no font is needed to show them):

```bash
git clone --depth 1 --filter=blob:none --sparse https://github.com/google/fonts /tmp/gfonts
git -C /tmp/gfonts sparse-checkout set ofl/fraunces ofl/ibmplexmono
python3 tools/brand/make-logo.py "/tmp/gfonts/ofl/fraunces/Fraunces[SOFT,WONK,opsz,wght].ttf" \
    /tmp/gfonts/ofl/ibmplexmono/IBMPlexMono-Regular.ttf
```

Made 2026-11-05 from google/fonts commit
`bbda7253fc363238b05d97515ca71f8ae49c4fa9` (Fraunces at wght 600, opsz 144,
SOFT 0, WONK 1; needs fontTools, PNGs need `rsvg-convert`).

## Fonts

- **Fraunces** - Copyright 2018 The Fraunces Project Authors
  (https://github.com/undercasetype/Fraunces), SIL Open Font License 1.1.
- **IBM Plex Mono** - Copyright © 2017 IBM Corp. with Reserved Font Name
  "Plex", SIL Open Font License 1.1.

Only outlines of a few letters are used in the images; the fonts themselves
are not part of this repository.
