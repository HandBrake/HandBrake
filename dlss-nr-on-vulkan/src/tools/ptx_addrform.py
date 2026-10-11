"""Symbolic linear form of a PTX address: const + sum(coeff * special_register)."""
import sys, re, bisect
sys.path.insert(0,'src/tools')
from ptx_trace import body, parse
from collections import defaultdict
REG=re.compile(r'%\w+'); MEM=re.compile(r'\[(%\w+)(?:\+(-?\d+))?\]')
SPECIAL=re.compile(r'%(ctaid|tid|ntid|nctaid|laneid|warpid)(\.[xyz])?')

class Ctx:
    def __init__(self, kernel):
        self.p, src = body(kernel)
        self.ins = parse(src)
        self.ds = defaultdict(list)
        for i,op,d,s,_ in self.ins:
            for r in d: self.ds[r].append(i)
        for r in self.ds: self.ds[r].sort()
        self.memo={}
    def reach(self, reg, j):
        L=self.ds.get(reg)
        if not L: return None
        k=bisect.bisect_left(L,j)-1
        return L[k] if k>=0 else None
    def form(self, reg, at, depth=0):
        """-> dict {'': const, '%tid.y': coeff, ...} or None if non-linear."""
        d=self.reach(reg, at)
        if d is None or depth>45: return None
        if d in self.memo: return self.memo[d]
        self.memo[d]=None
        _,op,dd,s,l = self.ins[d]
        toks=l.split(None,1)[1] if ' ' in l else ''
        parts=[p.strip() for p in toks.split(',')]
        def sub(k):
            if k>=len(parts): return None
            t=parts[k]
            if t.startswith('%'):
                m=SPECIAL.fullmatch(t)
                if m: return {t:1}
                return self.form(t, d, depth+1)
            try: return {'':int(t)}
            except ValueError: return None
        def add(a,b,sign=1):
            if a is None or b is None: return None
            o=dict(a)
            for k,v in b.items(): o[k]=o.get(k,0)+sign*v
            return o
        def scale(a,k):
            if a is None: return None
            return {kk:vv*k for kk,vv in a.items()}
        base=op.split('.')[0]
        r=None
        if base in ('mov','cvt','cvta'): r=sub(1)
        elif base=='add': r=add(sub(1),sub(2))
        elif base=='sub': r=add(sub(1),sub(2),-1)
        elif base=='mul':
            a,b=sub(1),sub(2)
            if a is not None and set(a)=={''}: r=scale(b,a[''])
            elif b is not None and set(b)=={''}: r=scale(a,b[''])
        elif base=='shl':
            a,b=sub(1),sub(2)
            if b is not None and set(b)=={''}: r=scale(a,1<<b[''])
        elif base=='mad':
            a,b,c=sub(1),sub(2),sub(3)
            if b is not None and set(b)=={''}: r=add(scale(a,b['']),c)
            elif a is not None and set(a)=={''}: r=add(scale(b,a['']),c)
        elif base=='ld' and op.startswith('ld.param'):
            r={'':0}          # the buffer pointer IS the origin; offsets are relative to it
        elif base in ('ld','tex','shfl'): r=None
        self.memo[d]=r
        return r
    def load_addr(self, d):
        """linear form of the address of the load at instruction index d"""
        m=MEM.search(self.ins[d][4])
        if not m: return None
        f=self.form(m.group(1), d)
        if f is None: return None
        f=dict(f); f['']=f.get('',0)+int(m.group(2) or 0)
        return f

def show(f):
    if f is None: return "non-linear"
    items=sorted(((k,v) for k,v in f.items() if v and k), key=lambda kv:-abs(kv[1]))
    s=" + ".join("%d*%s"%(v,k) for k,v in items)
    c=f.get('',0)
    return (("%d"%c) if not s else (s + (" + %d"%c if c else "")))
