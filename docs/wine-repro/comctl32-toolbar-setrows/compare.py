# Original, AI-assisted (Claude), LGPL-2.1-or-later.
# compare.py WINDOWS VANILLA CANDIDATE: per style, how many cases match
# Windows in TB_GETROWS and layout (pixel sizes ignored).
import sys,re
def load(p):
    d={}
    for l in open(p,errors='replace'):
        l=l.rstrip('\r\n')
        m=re.match(r'^(CASE|PALETTE) (.*?) (?:rect=\S+ )?getrows=(\d+) .*layout=\[(.*)\]',l)
        if m: d[('P ' if m.group(1)=='PALETTE' else '')+m.group(2)]=(m.group(3),m.group(4))
        m=re.match(r'^CHILD_TIMEOUT (.*)',l)
        if m: d[m.group(1)]='TIMEOUT'
    return d
w,van,v2=(load(p) for p in sys.argv[1:4])
from collections import Counter
cat=Counter(); ex={}
for k,x in w.items():
    if x=='TIMEOUT': continue
    st=re.search(r'style=(\w+)',k); key=st.group(1) if st else 'palette:'+k.split('step=')[1]
    a,b=van.get(k)==x, v2.get(k)==x
    c=(key, 'van=%s v2=%s'%('ok' if a else 'X','ok' if b else 'X'))
    cat[c]+=1; ex.setdefault(c,k)
for c,n in sorted(cat.items()): print(c,n,'e.g.',ex[c])
