"""Separate the Q/K/V accumulator chains without crossing between them."""
import sys, re
sys.path.insert(0,'src/tools')
from ptx_addrform import Ctx, show
REG=re.compile(r'%\w+')
def grp(l): return [REG.findall(g) for g in re.findall(r'\{([^}]*)\}', l)]

class Chains:
    def __init__(self, kernel):
        self.c = Ctx(kernel); self.ins = self.c.ins
        self.mmas = [i for i,op,d,s,l in self.ins if op.startswith('mma')]
        # 1. C-links: which mma produced this mma's accumulator
        self.cprod = {}
        for i in self.mmas:
            for r in grp(self.ins[i][4])[3]:
                d = self.c.reach(r, i)
                if d is not None and self.ins[d][1].startswith('mma'):
                    self.cprod[i] = d; break
        # 2. chain id, following C-links only
        self.cid = {}
        nxt = 0
        for i in self.mmas:
            j, path = i, []
            while j in self.cprod and j not in self.cid:
                path.append(j); j = self.cprod[j]
            root = self.cid.get(j)
            if root is None:
                root = nxt; nxt += 1; self.cid[j] = root
            for k in path: self.cid[k] = root
            self.cid.setdefault(i, root)
        self.nchains = nxt
        # 3. weight loads per chain (B operands that are global loads)
        self.wof = {}
        for i in self.mmas:
            for r in grp(self.ins[i][4])[2]:
                d = self.c.reach(r, i)
                if d is None or not self.ins[d][1].startswith(('ld.global','ld.weak.global')): continue
                f = self.c.load_addr(d)
                if f is None: continue
                self.wof.setdefault(self.cid[i], set()).add(
                    (f.get('',0), f.get('%tid.y',0), f.get('%ctaid.z',0)))

    def chains_behind(self, reg, at, maxdepth=30):
        """every mma chain reachable backwards from `reg`, stopping at each mma."""
        out=set(); seen=set(); stack=[(reg,at,0)]
        while stack:
            r, site, dep = stack.pop()
            d = self.c.reach(r, site)
            if d is None or d in seen or dep>maxdepth: continue
            seen.add(d)
            op = self.ins[d][1]
            if op.startswith('mma'):
                out.add(self.cid[d]); continue          # stop: do not cross into its inputs
            if op.startswith(('ld.','mov.u32')) and 'ld.global' in op: continue
            for rr in self.ins[d][3]:
                stack.append((rr, d, dep+1))
        return out

def footprint(tiles, Y, Z, tile=512):
    """union of covered byte ranges over the runtime index ranges"""
    seg=set()
    for cst, sy, sz in tiles:
        for z in range(Z):
            for y in range(Y):
                seg.add(cst + sy*y + sz*z)
    if not seg: return 0, 0
    return len(seg)*tile, min(seg)
