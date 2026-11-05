#!/usr/bin/env python3
"""Builds the Winum logo files from the two OFL fonts, as plain SVG paths.

GitHub shows SVG files without web fonts, so the letters are converted to
outlines here. Run from the repository root:

    python3 tools/brand/make-logo.py <Fraunces[...].ttf> <IBMPlexMono-Regular.ttf>

The fonts come from https://github.com/google/fonts (ofl/fraunces and
ofl/ibmplexmono), SIL Open Font License 1.1; they are not stored here.
Writes docs/brand/*.svg; PNG copies are made with rsvg-convert if present.
Original code, AI-assisted (Claude); LGPL-2.1-or-later like the repository.
"""
import shutil
import subprocess
import sys
from pathlib import Path

from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.transformPen import TransformPen
from fontTools.ttLib import TTFont
from fontTools.varLib import instancer

BG = '#0D1117'          # GitHub dark background
INK = '#F0F6FC'         # GitHub dark text
MUTED = '#8B949E'       # GitHub dark secondary text
WINE = '#A3263C'        # the wine in the u
TAGLINE = 'Fortified Wine for engineering software'


def load(path, **axes):
    font = TTFont(path)
    if 'fvar' in font:
        font = instancer.instantiateVariableFont(font, axes)
    return font


def text_paths(font, text, size, x, baseline, tracking=0.0):
    """Returns (svg path data, advance in px) for text set at x/baseline."""
    upem = font['head'].unitsPerEm
    scale = size / upem
    cmap = font.getBestCmap()
    glyphs = font.getGlyphSet()
    hmtx = font['hmtx']
    parts = []
    for ch in text:
        name = cmap[ord(ch)]
        pen = SVGPathPen(glyphs)
        glyphs[name].draw(TransformPen(pen, (scale, 0, 0, -scale, x, baseline)))
        parts.append(pen.getCommands())
        x += hmtx[name][0] * scale + tracking
    return ' '.join(p for p in parts if p), x


def advance(font, text, size, tracking=0.0):
    scale = size / font['head'].unitsPerEm
    cmap = font.getBestCmap()
    return sum(font['hmtx'][cmap[ord(c)]][0] * scale + tracking for c in text)


def cup(x, baseline, height, ink, wine):
    """The u as a cup: 64x60 design units, bottom on the baseline."""
    s = height / 60.0
    ox, oy = x, baseline - 60 * s
    tf = f'translate({ox:.2f} {oy:.2f}) scale({s:.4f})'
    return (f'<g transform="{tf}">'
            f'<path d="M17 22 H47 V34 Q47 45 32 45 Q17 45 17 34 Z" fill="{wine}"/>'
            f'<path d="M10 0 V34 Q10 52 32 52 Q54 52 54 34 V0" fill="none" stroke="{ink}" stroke-width="14"/>'
            f'</g>'), 64 * s


def wordmark(serif, size, cx, baseline, ink, wine):
    """'win' + cup + 'm', centred on cx. Returns svg elements."""
    track = -2.0 * size / 170.0
    xh = serif['OS/2'].sxHeight * size / serif['head'].unitsPerEm
    gap_l, gap_r = 3 * size / 170.0, 2 * size / 170.0
    _, cup_w = cup(0, 0, xh, ink, wine)
    total = advance(serif, 'win', size, track) + gap_l + cup_w + gap_r + advance(serif, 'm', size, track)
    x = cx - total / 2
    d1, x = text_paths(serif, 'win', size, x, baseline, track)
    g, w = cup(x + gap_l, baseline, xh, ink, wine)
    x += gap_l + w + gap_r
    d2, _ = text_paths(serif, 'm', size, x, baseline, track)
    return f'<path d="{d1} {d2}" fill="{ink}"/>{g}'


def tagline(mono, size, cx, baseline, color):
    track = 0.5
    x = cx - advance(mono, TAGLINE, size, track) / 2
    d, _ = text_paths(mono, TAGLINE, size, x, baseline, track)
    return f'<path d="{d}" fill="{color}"/>'


def svg(w, h, body, label, bg=BG):
    rect = f'<rect width="{w}" height="{h}" fill="{bg}"/>' if bg else ''
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}" '
            f'role="img" aria-label="{label}">{rect}{body}</svg>\n')


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    # Browsers set opsz from the font size (capped at 144); WONK/SOFT stay at the font defaults.
    serif = load(sys.argv[1], wght=600, opsz=144, SOFT=0, WONK=1)
    mono = load(sys.argv[2])
    out = Path('docs/brand')
    out.mkdir(parents=True, exist_ok=True)
    label = 'winum - Fortified Wine for engineering software'
    files = {
        'winum-banner.svg': svg(1280, 320, wordmark(serif, 110, 640, 182, INK, WINE)
                                + tagline(mono, 20, 640, 236, MUTED), label),
        'winum-social.svg': svg(1280, 640, wordmark(serif, 170, 640, 340, INK, WINE)
                                + tagline(mono, 28, 640, 420, MUTED), label),
        'winum-wordmark-dark.svg': svg(760, 200, wordmark(serif, 110, 380, 120, INK, WINE), 'winum', bg=None),
        'winum-wordmark-light.svg': svg(760, 200, wordmark(serif, 110, 380, 120, '#1F2328', WINE), 'winum', bg=None),
    }
    files['winum-icon.svg'] = svg(512, 512, cup(106, 400, 281, INK, WINE)[0], 'winum icon: the u as a cup of wine')
    for name, text in files.items():
        (out / name).write_text(text)
    if shutil.which('rsvg-convert'):
        for name in ('winum-social.svg', 'winum-icon.svg', 'winum-banner.svg'):
            subprocess.run(['rsvg-convert', '-o', str(out / name.replace('.svg', '.png')), str(out / name)], check=True)
    print('written:', ', '.join(sorted(p.name for p in out.iterdir())))


if __name__ == '__main__':
    main()
