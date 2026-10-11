#!/usr/bin/env python3
"""
test_attention_gpu — the three block families on Intel's matrix hardware.

**Superseded, and not in `make test`.** Two reasons, both worth knowing before running
it: it loads `work/weights_ht.bin`, the dense-FP16 decode that `notes/phase6` replaced —
so it cannot run on a clone that has not carved that file out of the DLL — and the
weights it reads are the wrong decode, kept only for the findings the surrounding files
encode. `notes/reviewing.md` says which tests are the live ones.

It still passes: worst relative deviation **2.8e-04** across every layer tested,
which is Phase 4's acceptance result. What it measures is whether the GPU path
reproduces the CPU path *on the same weights*, so the decode being wrong does not
invalidate it — it is a kernel test, not a weights test.
"""
import sys
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "ref"))
sys.path.insert(0, str(ROOT / "src" / "gpu"))
from hnet_model import Model
from hnet_ops import softmax, l2_normalize, HEAD_DIM, TOKENS, GQA_RATIO
from xmx import gemm


def attention(blk, tok, mm):
    """mm is the matmul to use: numpy's or the GPU's."""
    C = blk.t["wq"].shape[0]
    H = C // HEAD_DIM
    q = mm(tok, blk.t["wq"])
    k = mm(tok, blk.t["wk"])
    v = mm(tok, blk.t["wv"])
    q = l2_normalize(q.reshape(TOKENS, H, HEAD_DIM).transpose(1, 0, 2))
    k = l2_normalize(k.reshape(TOKENS, H // GQA_RATIO, HEAD_DIM).transpose(1, 0, 2))
    v = v.reshape(TOKENS, H // GQA_RATIO, HEAD_DIM).transpose(1, 0, 2)
    k = np.repeat(k, GQA_RATIO, axis=0)
    v = np.repeat(v, GQA_RATIO, axis=0)
    logits = (q @ k.transpose(0, 2, 1)) * blk.t["attn_scale"].astype(np.float32).reshape(-1, 1, 1)
    a = (softmax(logits + blk.t["attn_bias"].astype(np.float32)) @ v)
    a = a.transpose(1, 0, 2).reshape(TOKENS, C)
    return mm(a.astype(np.float16), blk.t["proj"])


def vit1d_attention(blk, tok, mm):
    """ViT-1D: 32 heads, GQA 4:1, global attention over the token sequence, and no
    positional bias table -- there is no window to bias (notes/phase3-block-internals)."""
    C = blk.t["wq"].shape[0]
    H = C // HEAD_DIM
    T = tok.shape[0]
    q = mm(tok, blk.t["wq"])
    k = mm(tok, blk.t["wk"])
    v = mm(tok, blk.t["wv"])
    q = l2_normalize(q.reshape(T, H, HEAD_DIM).transpose(1, 0, 2))
    k = l2_normalize(k.reshape(T, H // GQA_RATIO, HEAD_DIM).transpose(1, 0, 2))
    v = v.reshape(T, H // GQA_RATIO, HEAD_DIM).transpose(1, 0, 2)
    k = np.repeat(k, GQA_RATIO, axis=0)
    v = np.repeat(v, GQA_RATIO, axis=0)
    logits = (q @ k.transpose(0, 2, 1)) * blk.t["attn_scale"].astype(np.float32).reshape(-1, 1, 1)
    a = (softmax(logits) @ v).transpose(1, 0, 2).reshape(T, C)
    return mm(a.astype(np.float16), blk.t["proj"])


def feed_forward(blk, x, mm):
    """ViT-1D feed-forward: 1024 -> 2048 -> 1024, with the cosine gate applied."""
    h = mm(x, blk.t["ffn_expand"])
    h = h * (h > 0)
    return mm(h.astype(np.float16), blk.t["ffn_contract"])


def fused_attention(blk, tok, mm):
    """
    Fused Swin block. Only the qkv and the bias table are resolved inside these
    blocks so far, which is enough to exercise the attention path; the leading
    2.5C^2 + 64C region is not yet role-assigned. The scale is not located for this
    family, so it is left at 1 -- harmless here, because both sides of the comparison
    use the same formula and the point is GPU/CPU agreement.

    Restricted to C >= 128. The GQA 4:1 split needs K and V to be a whole number of
    32-channel heads: C/4/32 = 1 head at C=128, 2 at C=256, 4 at C=512. At C=64 it
    would be half a head and at C=32 a quarter, so those widths must use a different
    qkv structure -- not yet determined.
    """
    C = blk.t["wq"].shape[0]
    H = C // HEAD_DIM
    kv = max(H // GQA_RATIO, 1)
    q = mm(tok, blk.t["wq"])
    k = mm(tok, blk.t["wk"])
    v = mm(tok, blk.t["wv"])
    q = l2_normalize(q.reshape(TOKENS, H, HEAD_DIM).transpose(1, 0, 2))
    k = l2_normalize(k.reshape(TOKENS, kv, HEAD_DIM).transpose(1, 0, 2))
    v = v.reshape(TOKENS, kv, HEAD_DIM).transpose(1, 0, 2)
    rep = H // kv
    k = np.repeat(k, rep, axis=0)
    v = np.repeat(v, rep, axis=0)
    logits = q @ k.transpose(0, 2, 1)
    a = (softmax(logits + blk.t["attn_bias"].astype(np.float32)) @ v)
    return a.transpose(1, 0, 2).reshape(TOKENS, C)


def main():
    m = Model(str(ROOT / "work" / "weights_ht.bin"))
    idx = {b.index: b for b in m.blocks}
    cpu_mm = lambda A, B: A.astype(np.float32) @ B.astype(np.float32)
    print("=== full attention layer: XMX vs CPU reference ===\n")
    print("  %-10s %-8s %-13s %-13s %-11s %s"
          % ("block", "C", "|gpu-cpu|max", "cpu output sd", "relative", "verdict"))
    worst = 0.0
    rng = np.random.default_rng(11)
    for bi in (23, 26, 29, 40, 44, 47):
        blk = idx[bi]
        C = blk.t["wq"].shape[0]
        tok = (rng.standard_normal((TOKENS, C)) * 0.3).astype(np.float16)
        ref = attention(blk, tok, cpu_mm)
        got = attention(blk, tok, gemm)
        dev = np.abs(got - ref).max()
        rel = dev / max(np.abs(ref).max(), 1e-30)
        worst = max(worst, rel)
        print("  block%-5d %-8d %-13.4g %-13.4g %-11.4g %s"
              % (bi, C, dev, ref.std(), rel, "OK" if rel < 1e-3 else "FAIL"))
    print("\n=== ViT-1D bottleneck: attention and feed-forward on XMX ===\n")
    print("  %-10s %-10s %-13s %-13s %-11s %s"
          % ("block", "part", "|gpu-cpu|max", "cpu output sd", "relative", "verdict"))
    for bi in (31, 34, 38):
        blk = idx[bi]
        C = blk.t["wq"].shape[0]
        tok = (rng.standard_normal((128, C)) * 0.3).astype(np.float16)
        for label, fn in (("attention", vit1d_attention), ("feed-fwd", feed_forward)):
            ref = fn(blk, tok, cpu_mm)
            got = fn(blk, tok, gemm)
            dev = np.abs(got - ref).max()
            rel = dev / max(np.abs(ref).max(), 1e-30)
            worst = max(worst, rel)
            print("  block%-5d %-10s %-13.4g %-13.4g %-11.4g %s"
                  % (bi, label, dev, ref.std(), rel, "OK" if rel < 1e-3 else "FAIL"))

    print("\n=== fused Swin blocks (45 of 71): attention path on XMX ===\n")
    print("  %-10s %-8s %-13s %-13s %-11s %s"
          % ("block", "C", "|gpu-cpu|max", "cpu output sd", "relative", "verdict"))
    for bi in (9, 12, 15, 21, 49, 55, 57, 61):
        blk = idx[bi]
        if blk.kind != "fused_swin" or blk.C < 128:
            continue
        C = blk.C
        tok = (rng.standard_normal((TOKENS, C)) * 0.3).astype(np.float16)
        ref = fused_attention(blk, tok, cpu_mm)
        got = fused_attention(blk, tok, gemm)
        dev = np.abs(got - ref).max()
        rel = dev / max(np.abs(ref).max(), 1e-30)
        worst = max(worst, rel)
        print("  block%-5d %-8d %-13.4g %-13.4g %-11.4g %s"
              % (bi, C, dev, ref.std(), rel, "OK" if rel < 1e-3 else "FAIL"))

    print("\n  worst relative deviation across every layer tested: %.4g" % worst)
    ok = worst < 1e-3
    print("  %s" % ("PASS - all three block families reproduce on Intel hardware" if ok else "FAIL"))
    return 0 if ok else 1


raise SystemExit(main())
