#!/usr/bin/env python3
"""
ptx_trace — def/use dataflow over a single PTX kernel.

Reading PTX by eye has repeatedly produced wrong answers in this project: the
"missing 2^8.5 factor", the shifted-window mask, the leading-region projection. All
three were assumptions that a mechanical trace would have refused. This walks the
actual chains.

Usage:
  ptx_trace.py <entry> --param <byte-offset>     what does this launch argument feed?
  ptx_trace.py <entry> --reg %rd7 [--depth N]    what consumes this register?
  ptx_trace.py <entry> --consumers mma           which registers reach an mma operand?
"""
import glob
import re
import sys
from collections import defaultdict

REG = re.compile(r'%\w+')


def body(entry, root="work/modules/*.ptx"):
    for p in sorted(glob.glob(root)):
        txt = open(p, encoding="utf-8", errors="replace").read()
        m = re.search(r'^\.visible \.entry ' + re.escape(entry) + r'\(', txt, re.M)
        if not m:
            continue
        s = m.start()
        nxt = re.search(r'^\.(visible \.entry|func |visible \.func)', txt[s + 40:], re.M)
        return p, (txt[s: s + 40 + nxt.start()] if nxt else txt[s:])
    return None, None


def statements(src):
    """
    Split a kernel body into PTX statements, **descending into `{ ... }` scopes**.

    This is not a detail. NVCC wraps hand-written f16x2 arithmetic in brace scopes
    (`{fma.rn.f16x2 %r647,%r644,%r645,%r646;}`), and a line-oriented parser that
    skips anything starting with `{` drops every one of them. That is what produced
    the "DLSS-NR does not use softmax" conclusion: the exp is built from
    max/min/fma.f16x2 plus a shift-and-add on the bit pattern, and all of it lived
    inside braces. See notes/phase5-softmax-found.md.

    A `{` opens a scope when the previous non-space character is `;`, `{`, `}` or
    nothing; otherwise it is an operand group (`mma ... {%r1,%r2}, ...`) and stays
    inside the statement. Registers declared `.reg` inside a scope are renamed per
    scope so two scopes reusing the name `low` do not alias.
    """
    txt = "\n".join(l.split("//")[0] for l in src.split("\n"))
    out, buf, stack, prev, sid = [], [], [], "", 0
    for ch in txt:
        if ch == "{" and (prev in ";{}" or prev == ""):
            sid += 1
            stack.append(sid)
            out.append(("".join(buf), tuple(stack)))
            buf = []
        elif ch == "}" and stack and not "".join(buf).count("{") > "".join(buf).count("}"):
            out.append(("".join(buf), tuple(stack)))
            buf = []
            stack.pop()
        elif ch == ";":
            out.append(("".join(buf), tuple(stack)))
            buf = []
        else:
            buf.append(ch)
        if not ch.isspace():
            prev = ch
    out.append(("".join(buf), tuple(stack)))
    return [(t.strip(), sc) for t, sc in out if t.strip()]


LOCAL = re.compile(r'(?<![%\w.$])([a-zA-Z_]\w*)(?![\w.(])')
DECL = re.compile(r'^\.reg\s*\.\w+\s+(.*)$')     # `.reg .f32 x` and `.reg.b16 x` both occur
LABEL = re.compile(r'^[\$\w]+:\s*')
STORE = ("st", "red", "atom", "bar", "mbarrier", "cp.", "sust", "ret", "bra", "prefetch")


def split_operands(rest):
    """Split an operand list on top-level commas, keeping {..} and [..] intact."""
    ops, depth, cur = [], 0, []
    for ch in rest:
        if ch in "{[":
            depth += 1
        elif ch in "}]":
            depth -= 1
        if ch == "," and depth == 0:
            ops.append("".join(cur)); cur = []
        else:
            cur.append(ch)
    if "".join(cur).strip():
        ops.append("".join(cur))
    return [o.strip() for o in ops]


def parse(src):
    """-> list of (index, opcode, dests, srcs, raw). Descends into brace scopes."""
    out, locals_by_scope = [], {}
    for text, scope in statements(src):
        text = LABEL.sub("", text).strip()
        if not text:
            continue
        m = DECL.match(text)
        if m:
            for nm in re.split(r'[,\s]+', m.group(1)):
                nm = nm.split("<")[0].strip()
                if nm:
                    locals_by_scope.setdefault(scope, {})[nm] = "%%__s%d_%s" % (scope[-1] if scope else 0, nm)
            continue
        if text.startswith("."):
            continue
        text = re.sub(r'^@!?%\w+\s+', '', text).strip()
        m = re.match(r'([a-z][\w.:]*)\s*(.*)$', text, re.S)
        if not m:
            continue
        op, rest = m.group(1), m.group(2)
        # substitute scope-local register names so they join the dataflow graph
        names = {}
        for k in range(len(scope), -1, -1):
            names.update(locals_by_scope.get(scope[:k], {}))
        if names:
            rest = LOCAL.sub(lambda mo: names.get(mo.group(1), mo.group(1)), rest)
        ops = split_operands(rest)
        if not ops:
            out.append((len(out), op, [], [], text))
            continue
        if op.startswith(STORE):
            dests, srcs = [], REG.findall(rest)
        else:
            dests = REG.findall(ops[0])
            srcs = REG.findall(",".join(ops[1:]))
            if ops[0].strip().startswith("["):     # dest is a memory ref: address is a source
                srcs, dests = dests + srcs, []
        out.append((len(out), op, dests, srcs, text))
    return out


def chains(instrs):
    defs = defaultdict(list); uses = defaultdict(list)
    for i, op, d, s, _ in instrs:
        for r in d: defs[r].append(i)
        for r in s: uses[r].append(i)
    return defs, uses


def forward(instrs, defs, uses, seeds, depth=6, limit=400):
    """Which opcodes does a value reach, and by what route."""
    seen, frontier, hit = set(), set(seeds), defaultdict(int)
    routes = []
    for lvl in range(depth):
        nxt = set()
        for r in frontier:
            for i in uses.get(r, [])[:limit]:
                if i in seen: continue
                seen.add(i)
                _, op, d, s, line = instrs[i]
                hit[(lvl, op)] += 1
                if lvl <= 2: routes.append((lvl, line))
                nxt.update(d)
        frontier = nxt
        if not frontier: break
    return hit, routes


def coverage(src, instrs):
    """
    Report how much of the body was actually parsed. A silent skip here is the most
    expensive bug this tool can have: dropping brace scopes hid the entire attention
    non-linearity and produced a documented, wrong conclusion. Print it every run.
    """
    stmts = [t for t, _ in statements(src)]
    kept = len(instrs)
    directives = sum(1 for t in stmts if t.startswith("."))
    labels = sum(1 for t in stmts if LABEL.match(t) and not t.split(":", 1)[1].strip())
    unaccounted = len(stmts) - kept - directives - labels
    return ("coverage: %d statements, %d parsed, %d directives, %d labels, %d unaccounted%s"
            % (len(stmts), kept, directives, labels, unaccounted,
               "  <-- CHECK THIS" if unaccounted > len(stmts) // 50 else ""))


def main():
    entry = sys.argv[1]
    path, src = body(entry)
    if src is None:
        sys.exit("entry %s not found" % entry)
    instrs = parse(src)
    defs, uses = chains(instrs)
    print("kernel %s   [%s]   %d instructions parsed" % (entry, path.split("/")[-1], len(instrs)))
    print("  %s" % coverage(src, instrs))

    seeds = []
    if "--param" in sys.argv:
        off = int(sys.argv[sys.argv.index("--param") + 1])
        pat = re.compile(re.escape(entry) + r'_param_0(?:\+(\d+))?')
        for i, op, d, s, line in instrs:
            m = pat.search(line)
            if m and int(m.group(1) or 0) == off:
                seeds += d
        print("  seeds from param+%d: %s" % (off, seeds or "none"))
    elif "--reg" in sys.argv:
        seeds = [sys.argv[sys.argv.index("--reg") + 1]]
    if not seeds:
        sys.exit("no seed registers")

    depth = int(sys.argv[sys.argv.index("--depth") + 1]) if "--depth" in sys.argv else 6
    hit, routes = forward(instrs, defs, uses, seeds, depth)
    print("\n  opcodes reached, by distance from the seed:")
    for (lvl, op), n in sorted(hit.items()):
        print("     level %d  %-28s x%d" % (lvl, op, n))
    print("\n  first steps:")
    for lvl, line in routes[:14]:
        print("     [%d] %s" % (lvl, line[:110]))


if __name__ == "__main__":
    main()
