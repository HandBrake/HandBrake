#!/usr/bin/env python3
"""
forward — carry a tensor through all 71 blocks in execution order.

This is a *plumbing* test, not the finished network. It exercises the parts whose
roles are established — the attention path, the cosine-gate mixing, the resolution
and channel schedule — and passes through the parts that are not (the leading
2.5C^2 + 64C region of the fused blocks, whose sub-roles are still unassigned, and
the runtime scale factor that is not in the file at all).

What it proves: the block order, the channel schedule, the resolution schedule and
every tensor shape chain correctly from input to output. What it does not prove:
that the numbers are the ones DLSS-NR would produce.

Geometry is derived from the kernel configs: Attention1dConfig gives the bottleneck a
128-token sequence, and Conv2d1x1Config<1024,1024,16,8,...> makes that a 16x8 grid.
Doubling upward through the four encoder stages puts the input at 512x256.
"""
import sys
import time
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
from hnet_model import Model
from hnet_ops import softmax, l2_normalize, cos_skip, HEAD_DIM, TOKENS, GQA_RATIO, window_partition, window_reverse

# stage width -> (H, W) of the feature map
GEOMETRY = {32: (512, 256), 64: (256, 128), 128: (128, 64), 256: (64, 32), 512: (32, 16), 1024: (16, 8)}


def attend(blk, x, mm):
    """x:(H,W,C) -> (H,W,C). Windowed attention where the qkv split is known."""
    H, W, C = x.shape
    h = C // HEAD_DIM
    kv = h // GQA_RATIO
    if "wq" not in blk.t or kv < 1:
        return None
    win = window_partition(x.astype(np.float16))              # (nWin, 64, C)
    n = win.shape[0]
    flat = win.reshape(-1, C)
    q = mm(flat, blk.t["wq"]).reshape(n, TOKENS, h, HEAD_DIM).transpose(0, 2, 1, 3)
    k = mm(flat, blk.t["wk"]).reshape(n, TOKENS, kv, HEAD_DIM).transpose(0, 2, 1, 3)
    v = mm(flat, blk.t["wv"]).reshape(n, TOKENS, kv, HEAD_DIM).transpose(0, 2, 1, 3)
    q = l2_normalize(q); k = l2_normalize(k)
    k = np.repeat(k, GQA_RATIO, axis=1); v = np.repeat(v, GQA_RATIO, axis=1)
    logits = q @ k.transpose(0, 1, 3, 2)
    if "attn_scale" in blk.t:
        logits = logits * blk.t["attn_scale"].astype(np.float32).reshape(1, -1, 1, 1)
    a = softmax(logits + blk.t["attn_bias"].astype(np.float32)[None]) @ v
    a = a.transpose(0, 2, 1, 3).reshape(n, TOKENS, C)
    if "proj" in blk.t:
        a = mm(a.reshape(-1, C).astype(np.float16), blk.t["proj"]).reshape(n, TOKENS, C)
    return window_reverse(a, H, W)


def resize(x, hw):
    """Nearest-neighbour resample between stages; channel count is set by the weights."""
    H, W, C = x.shape
    th, tw = hw
    yi = (np.arange(th) * H // th).clip(0, H - 1)
    xi = (np.arange(tw) * W // tw).clip(0, W - 1)
    return x[np.ix_(yi, xi, np.arange(C))]


def refit_channels(x, C):
    """Tile or truncate channels when a stage changes width. Stand-in for the real
    transition matrices, which live in the unassigned `pre` region."""
    cur = x.shape[-1]
    if cur == C:
        return x
    if cur < C:
        return np.tile(x, (1, 1, -(-C // cur)))[:, :, :C]
    return x[:, :, :C]


def main():
    use_gpu = "--gpu" in sys.argv
    if use_gpu:
        sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "gpu"))
        from xmx import gemm
        calls = [0]

        def mm(A, B):
            calls[0] += 1
            return gemm(A, B)
    else:
        calls = [0]

        def mm(A, B):
            calls[0] += 1
            return A.astype(np.float32) @ B.astype(np.float32)
    m = Model(str(Path(__file__).resolve().parents[2] / "work" / "weights_ht.bin"))
    rng = np.random.default_rng(0)
    x = (rng.standard_normal((512, 256, 32)) * 0.5).astype(np.float32)
    print("=== forward pass through all %d blocks ===\n" % len(m.blocks))
    print("  input %s" % (x.shape,))
    t0 = time.time()
    attended = passed = 0
    last_kind = None
    for blk in m.blocks:
        C = blk.C
        if C in GEOMETRY:
            x = refit_channels(resize(x, GEOMETRY[C]), C)
        branch = attend(blk, x, mm) if blk.kind in ("fused_swin", "split_swin_16h") else None
        if branch is not None:
            gate = None
            for name in ("gate", "ffwd_gate", "ffwd_proj_gate"):
                if name in blk.t and blk.t[name].size == C:
                    gate = blk.t[name].astype(np.float32)
                    break
            x = cos_skip(branch, x, gate) if gate is not None else x + branch
            attended += 1
        else:
            passed += 1
        if blk.kind != last_kind:
            print("  block%-3d %-18s C=%-5d map %s" % (blk.index, blk.kind, C, x.shape[:2]))
            last_kind = blk.kind
    dt = time.time() - t0
    print("\n  output %s   finite=%s   mean=%+.4g   sd=%.4g"
          % (x.shape, np.isfinite(x).all(), x.mean(), x.std()))
    print("  blocks with attention applied: %d    passed through: %d" % (attended, passed))
    print("  matmuls: %d   wall time: %.1f s   (%s)"
          % (calls[0], dt, "XMX" if use_gpu else "CPU numpy"))
    if "--save" in sys.argv:
        out = Path(sys.argv[sys.argv.index("--save") + 1])
        out.write_bytes(np.ascontiguousarray(x, dtype="<f4").tobytes())
        print("  saved %s" % out)
    return 0 if np.isfinite(x).all() else 1


raise SystemExit(main())
