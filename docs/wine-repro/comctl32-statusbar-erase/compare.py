#!/usr/bin/env python3
# Compare a Wine run of the status bar MEASURE with the Windows run, row by
# row. Original, AI-assisted (Claude); LGPL-2.1-or-later.
# Usage: compare.py windows-result.txt wine-result.txt
# Font-dependent pixel counts (old_pixels, old_retained) and the exact
# button-face shade differ between Windows and Wine; read marker_after,
# erase counts and whether after_rgb is the old glyph colour.
import sys
def load(f):
    rows=[];cfg=None
    for l in open(f):
        w=l.split()
        if not w: continue
        if w[0]=='CONFIG':
            d=dict(x.split('=',1) for x in w[1:]); cfg=(d['classic'],d['custom_erase'],d['style']); th=d['theme']
        elif w[0]=='MEASURE':
            d=dict(x.split('=',1) for x in w[1:]); d['_cfg']=cfg; d['_theme']=th; rows.append(d)
    return rows
keys=['prep_diff','update_before','erase','nested_erase','outside_erase','send_erase','paint','drawitem','marker_after','old_retained','after_rgb']
win,wine=load(sys.argv[1]),load(sys.argv[2])
print(sys.argv[2], len(win), len(wine))
from collections import Counter
diffs=Counter()
for a,b in zip(win,wine):
    assert a['op']==b['op'] and a['blank']==b['blank'] and a['_cfg']==b['_cfg']
    dk=[k for k in keys if a[k]!=b[k]]
    if dk:
        key=(a['_cfg'][0],'themeW='+a['_theme'],'themeWine='+b['_theme'],a['op'])
        diffs[(key, ' '.join(f"{k}:{a[k]}->{b[k]}" for k in dk))]+=1
for (k,d),n in sorted(diffs.items()): print(n, 'classic=%s %s %s op=%s'%k, '|', d)
