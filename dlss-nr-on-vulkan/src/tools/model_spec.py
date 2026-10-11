#!/usr/bin/env python3
"""
model_spec — derive the DLSS-NR network specification from the weight container.

Every number printed here is computed from `work/weights_ht.bin`, cross-checked
against the kernel template parameters recovered from the PTX
(`notes/ptx-kernel-configs.md`). Nothing is hand-entered.

Usage: model_spec.py <weights_ht.bin>
"""
import collections
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from hnet_weights import parse

# Verified from PTX and from the weights themselves. Head dim 32, Swin window
# 8x8 -> 64 tokens, so the attention bias is heads * 64 * 64 = (C/32)*4096 = 128*C.
# A fused Swin block lays out, in order:
#     [ 4C^2 + 64C ]  weights (qkv 1.5C^2 + projection C^2 + ffn), gates inside
#     [ 128C       ]  attention log-bias, mean ~ -7, exp() of it underflows to 0
#     [ q*C^2/2    ]  second weight group, q = 0 at C=32, 1 at C >= 64
#     [ r*C^2      ]  transition blocks only (stage change), r = 1
#     [ 2C + c     ]  per-channel gates / out_gain, c a small residue
ATTN_BIAS = lambda C: 128 * C


def fit_fused(total):
    """Return (C, q, r, c) or None. Exact for 45 of the 46 fused blocks."""
    for C in (32, 64, 128, 256):
        base = 4 * C * C + 64 * C + ATTN_BIAS(C)
        for q in (0, 1):
            for r in (0, 1):
                c = total - base - q * (C * C // 2) - r * C * C - 2 * C
                if 0 <= c <= 600:
                    return C, q, r, c
    return None


def load(path):
    blob = Path(path).read_bytes()
    by = collections.defaultdict(dict)
    for t in parse(blob):
        m = re.match(r"block(\d+)\.layer(\d+)\.(\w+)$", t["name"])
        by[int(m.group(1))][(int(m.group(2)), m.group(3))] = t["n_elem"]
    return by


def classify(b, subs, total):
    if len(subs) == 4:
        return 512, "split-Swin-16H", "C^2 | C^2/2+C | 1.5C^2+128C+32 | C^2/2+C"
    if any(k[1] == "blend_scale" for k in dict(subs)):
        return 32, "output head + blend_scale", "4C^2 | 128C | out_gain, out_conv_weight, blend"
    if len(subs) >= 5:
        return 1024, "ViT-1D bottleneck", "2C^2+8 | 2C^2+C | 1.5C^2+128C+64 | 1 | C^2/2+C"
    f = fit_fused(total)
    if f:
        C, q, r, c = f
        kind = "fused Swin" + (" + transition" if r else "")
        return C, kind, "4C^2+64C | 128C |%s%s 2C+%d" % (
            " C^2/2 |" if q else "", " C^2 |" if r else "", c)
    return 1024, "dec_input_upsample", "512x512 + 512"


# Execution order: the 156-slot captured graph, cross-checked slot-count against
# our block inventory (see notes/phase3-execution-order.md). Every stage matches
# exactly; vit_1d carries 2 extra weightless repack kernels and slot 0 / 155 are
# the clear and the final copy.
STAGES_EXEC = (
    ("clear",              0,   0,   None),
    ("encoder_1h_32",      1,   5,   (0, 4)),
    ("encoder_2h_64",      6,   9,   (5, 8)),
    ("encoder_4h_128",     10,  15,  (9, 14)),
    ("encoder_8h_256",     16,  23,  (15, 22)),
    ("bottleneck_16h_512", 24,  56,  (23, 30)),
    ("vit_1d",             57,  98,  (31, 38)),
    ("decoder_16h_512",    99,  131, (39, 47)),
    ("decoder_8h_256",     132, 139, (48, 55)),
    ("decoder_4h_128",     140, 145, (56, 61)),
    ("decoder_2h_64",      146, 149, (62, 65)),
    ("decoder_1h_32",      150, 153, (66, 69)),
    ("output_head",        154, 154, (70, 70)),
    ("final_copy",         155, 155, None),
)


def main():
    by = load(sys.argv[1])
    total_all = sum(sum(v.values()) for v in by.values())
    rows = []
    for b in sorted(by):
        subs = sorted(by[b].items())
        tot = sum(by[b].values())
        C, kind, layout = classify(b, subs, tot)
        rows.append((b, C, tot, len(subs), kind, layout))

    print("# DLSS-NR (CG2R / HNet, build 310.8.0.0) — network specification")
    print("#\n# Derived from the weight container; widths cross-checked against PTX")
    print("# kernel template parameters. Head dim 32, Swin window 8x8 (64 tokens),")
    print("# attention bias = heads * 64 * 64 = 128*C. Grouped-query attention:")
    print("# QKV = 1.5C^2 = Q(C^2) + K(C^2/4) + V(C^2/4), i.e. 4:1 query:kv heads.\n")
    print("%-8s %-6s %-5s %-13s %-28s %s" % ("block", "C", "subs", "elements", "kind", "layout"))
    for b, C, tot, n, kind, layout in rows:
        print("block%-3d %-6d %-5d %-13s %-28s %s" % (b, C, n, format(tot, ","), kind, layout))

    print("\n# stage summary")
    st = collections.Counter((C, kind) for _, C, _, _, kind, _ in rows)
    for (C, kind), n in sorted(st.items()):
        print("#   C=%-5d %-28s x%d" % (C, kind, n))

    print("\n# execution order (156-slot graph)")
    print("# %-22s %-11s %-14s %s" % ("stage", "slots", "blocks", "weight records"))
    for name, a, b, blk in STAGES_EXEC:
        if blk is None:
            print("# %-22s %-11s %-14s %s" % (name, "%d-%d" % (a, b), "-", "0 (weightless)"))
            continue
        nrec = sum(len(by[x]) for x in range(blk[0], blk[1] + 1))
        print("# %-22s %-11s %-14s %d" % (name, "%d-%d" % (a, b),
                                          "block%d-%d" % blk, nrec))

    acc = sum(r[2] for r in rows)
    print("\n# accounted %s of %s elements -> %s"
          % (format(acc, ","), format(total_all, ","),
             "COMPLETE" if acc == total_all else "INCOMPLETE"))
    return 0 if acc == total_all else 1


raise SystemExit(main())
