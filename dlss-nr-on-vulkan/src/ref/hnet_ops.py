#!/usr/bin/env python3
"""
hnet_ops — operators for the DLSS-NR CPU reference.

**Superseded — do not build on this.** Its slices come from the dense-FP16 reading of the
container that `notes/phase6` replaced, and the GQA split asserted below does not exist:
`qkv_weight` is `(C, 3C)`, full multi-head attention (`notes/phase61`). The live operators
are in `src/ref/nr_model.py`.

Design rule: every tensor slice is taken at an offset and length that the spec
(`notes/MODEL-SPEC.txt`) states exactly. Role assignments that are *inferred* rather
than measured are marked HYPOTHESIS in the code and are checked at runtime, so a
wrong guess fails loudly instead of producing plausible garbage.

Numerics follow the Phase 4 contract throughout: FP16 operands, FP32 accumulation
(cooperative matrix config 1, `notes/hw-coopmat.md`).
"""
import numpy as np

HEAD_DIM = 32          # verified: every config carries head dim 32
WINDOW = 8             # verified: Attention2dConfig <..., 8, 8, ...>
TOKENS = WINDOW * WINDOW
GQA_RATIO = 4          # verified: FusedSwin2dQKVAttnConfig<512, 8, 8, 16, 4, ...>


def linear(x, w, b=None):
    """FP16 operands, FP32 accumulation. x:(N,in) w:(in,out) -> (N,out) float32."""
    assert w.dtype == np.float16
    y = x.astype(np.float32) @ w.astype(np.float32)
    if b is not None:
        y = y + b.astype(np.float32)
    return y


def softmax(x, axis=-1):
    """Textbook, max-subtracted. Kept for comparison only -- the network does NOT
    use this; see ptx_softmax below."""
    m = x.max(axis=axis, keepdims=True)
    e = np.exp(x - m)
    return e / e.sum(axis=axis, keepdims=True)


# --- the attention non-linearity, read out of the PTX -------------------------
#
# There IS a softmax. `ex2` never appears because the exponential is hand-rolled in
# f16x2 so it runs two lanes at a time: an affine map into [1,2), a clamp, then a
# left shift of the f16 *bit pattern* with a compensating 32-bit add. Inside [1,2)
# the bit pattern is linear in the value, so `bits = (bits(t) - 0x3C00) << sh` is
# the standard mantissa-linear approximation of 2^x. Verified bit-exact against a
# full 32-bit two-lane emulation, and within [0.92, 1.04] of exp() over the whole
# clamped range. See notes/phase5-softmax-found.md.
#
# The consequence that matters is not the 4% ripple -- it is that **there is no
# max subtraction**. The clamp is what keeps the exponential in range, so the
# softmax is not shift-invariant and the absolute size of the logits decides how
# sharp the attention gets. A constant added to a row changes the answer.
#
# a, b, lo, hi are the f16 constants; `sh` the shift. Two variants exist across the
# 231 kernels and no others: Swin/post-block clamps the logits to +-6, ViT to +-3.
EXP_VARIANTS = {
    "swin": (np.float16(0.044921875), np.float16(1.30078125),
             np.float16(1.03125), np.float16(1.5693359375), 5),      # 114 kernels
    "vit":  (np.float16(0.08953857421875), np.float16(1.708984375),
             np.float16(1.439453125), np.float16(1.9775390625), 4),  # 12 kernels
}
LOGIT_CLAMP = {"swin": 6.0, "vit": 3.0}


def ptx_exp(x, variant="swin"):
    """exp(x) as the kernels compute it, including the clamp. Returns float32."""
    a, b, lo, hi, sh = EXP_VARIANTS[variant]
    t = np.minimum(np.maximum((np.asarray(x, np.float16) * a + b).astype(np.float16), lo), hi)
    bits = ((t.view(np.uint16).astype(np.uint32) - 0x3C00) << sh) & 0xFFFF
    return bits.astype(np.uint16).view(np.float16).astype(np.float32)


def ptx_softmax(x, axis=-1, variant="swin", eps=None):
    """
    The attention weights as the hardware produces them: clamped logits, bit-trick
    exp, and a reciprocal of the row sum floored at eps (`max.f16x2` then a
    hand-rolled `rcp.f16x2` -- the kernels do exactly this).
    """
    e = ptx_exp(x, variant)
    if eps is None:
        eps = 6.1e-5                       # smallest normal f16; the kernels floor here
    return e / np.maximum(e.sum(axis=axis, keepdims=True), eps)


def window_partition(x, w=WINDOW):
    """(H,W,C) -> (nWin, w*w, C). Swin's non-overlapping windows."""
    H, W, C = x.shape
    assert H % w == 0 and W % w == 0, "H,W must be multiples of the window"
    x = x.reshape(H // w, w, W // w, w, C).transpose(0, 2, 1, 3, 4)
    return x.reshape(-1, w * w, C)


def window_reverse(win, H, W, w=WINDOW):
    C = win.shape[-1]
    x = win.reshape(H // w, W // w, w, w, C).transpose(0, 2, 1, 3, 4)
    return x.reshape(H, W, C)


def cos_skip(branch, skip, gate, form="gated_branch"):
    """
    Gated skip mix. `gate` holds values in [-1, 1] with the maximum at exactly 1.0
    (parameter names `attn_cos_skip` / `ffn_cos_skip`).

    A norm-preserving `cos*skip + sqrt(1-cos^2)*branch` was assumed at first and is
    **ruled out**: no kernel anywhere computes a sqrt, an fma against 1.0f or a
    subtraction from 1.0f (see notes/phase3-first-operator.md). The gate is applied
    as a plain multiply.

    Which operand it multiplies is not settled. `gated_branch` leaves the trunk
    undecayed and is the default; `gated_skip` matches the parameter name more
    literally. Both are one line apart so the choice stays visible.
    """
    c = gate.astype(np.float32)
    if form == "gated_skip":
        return c * skip + branch
    if form == "convex":
        return (1.0 - c) * skip + c * branch
    if form == "convex_inv":
        return c * skip + (1.0 - c) * branch
    return skip + c * branch


def l2_normalize(x, axis=-1, eps=1e-6):
    """The `rsqrt` the QKV kernel performs 2x per head, and the surviving ATen op
    name `linalg_vector_norm`. Q and K are unit-normalised before the dot product."""
    return x / np.sqrt(np.maximum((x * x).sum(axis=axis, keepdims=True), eps))


def gqa_attention(q, k, v, bias, n_qheads, n_kvheads, scale=None):
    """
    Grouped-query attention over one window, with QK-normalisation.

      q:(T, n_qheads*HEAD_DIM)  k,v:(T, n_kvheads*HEAD_DIM)
      bias:(n_qheads, T, T) additive, log space
      scale:(n_qheads,) learned per-head temperature (`attn_scale`)

    `cc_split_swin_16h_qkv_512` issues exactly 32 `rsqrt.approx.ftz.f32` for a
    16-head block: two per head, i.e. Q and K each normalised. Without a learned
    scale, unit-vector logits would be confined to [-1, 1]; the scale restores range.
    """
    T = q.shape[0]
    q = l2_normalize(q.reshape(T, n_qheads, HEAD_DIM).transpose(1, 0, 2))
    k = l2_normalize(k.reshape(T, n_kvheads, HEAD_DIM).transpose(1, 0, 2))
    v = v.reshape(T, n_kvheads, HEAD_DIM).transpose(1, 0, 2)
    rep = n_qheads // n_kvheads
    k = np.repeat(k, rep, axis=0)
    v = np.repeat(v, rep, axis=0)
    logits = q @ k.transpose(0, 2, 1)
    if scale is not None:
        logits = logits * scale.reshape(-1, 1, 1)
    return (softmax(logits + bias) @ v).transpose(1, 0, 2).reshape(T, n_qheads * HEAD_DIM)


class SplitSwin16H:
    """
    One block of the 512-wide stage (our block23..30, 40..47; slots 24..56, 99..131).

    Sub-tensor roles, by exact size match against the kernel configs:
      layer0  C^2                 projection_weight       VERIFIED (Conv2d1x1<512,512>)
      layer1  C^2/2 + C           feed-forward            HYPOTHESIS
      layer2  1.5C^2 | 128C | 32  qkv | attention bias    VERIFIED by size + value class
      layer3  C^2/2 + C           feed-forward projection HYPOTHESIS
    """

    def __init__(self, net, block, C=512):
        self.C, self.n = C, block
        g = lambda i: net.raw("block%d.layer%d.layer" % (block, i))
        self.proj = g(0).reshape(C, C)
        # layer1 / layer3: [C^2/2 matrix][C cosine gate]. The trailing C values are
        # NOT biases -- they are cos-skip gates: range [-1,1], max exactly 1.0.
        self.w_ffwd = g(1)[:C * C // 2].reshape(C, C // 2)
        self.gate_a = g(1)[C * C // 2:]
        l2 = g(2)
        qkv_n = 3 * C * C // 2
        self.qkv = l2[:qkv_n]
        H = C // HEAD_DIM
        self.attn_bias = l2[qkv_n:qkv_n + 128 * C].astype(np.float32).reshape(H, TOKENS, TOKENS)
        # trailing 2*H values: odd slots are the learned per-head attention scale
        # (16 values at C=512, 32 at C=1024, all in a tight 1.1-2.2 band).
        self.attn_scale = l2[qkv_n + 128 * C:].astype(np.float32)[1::2]
        assert self.attn_scale.size == H, "attn_scale length must equal the head count"
        self.w_ffwd_proj = g(3)[:C * C // 2].reshape(C, C // 2)
        self.gate_b = g(3)[C * C // 2:]
        # GQA split: Q is C x C, K and V are each C x C/4
        self.wq = self.qkv[:C * C].reshape(C, C)
        self.wk = self.qkv[C * C: C * C + C * C // 4].reshape(C, C // 4)
        self.wv = self.qkv[C * C + C * C // 4:].reshape(C, C // 4)
        assert self.wv.size == C * C // 4, "GQA split does not consume the qkv slice exactly"

    def feed_forward(self, x):
        """x:(T,C) float16 -> (T,C) float32.  HYPOTHESIS: C -> C/2 -> C."""
        h = linear(x, self.w_ffwd)                     # (T, C/2)
        h = h * (h > 0)                                # ReLU-family gate, placeholder
        return linear(h.astype(np.float16), self.w_ffwd_proj.T.copy())

    def attend(self, tokens):
        """tokens: (T, C) float16 for one window -> (T, C) float32."""
        C = self.C
        q = linear(tokens, self.wq)
        k = linear(tokens, self.wk)
        v = linear(tokens, self.wv)
        a = gqa_attention(q, k, v, self.attn_bias,
                          C // HEAD_DIM, (C // HEAD_DIM) // GQA_RATIO, self.attn_scale)
        return linear(a.astype(np.float16), self.proj)


def _selftest():
    import sys
    from pathlib import Path
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from hnet_ref import HNet

    net = HNet("work/weights_ht.bin")
    blk = SplitSwin16H(net, 23)
    C = blk.C
    print("== block23 (split-Swin-16H, C=%d) ==" % C)
    print("   wq %s  wk %s  wv %s  proj %s" % (blk.wq.shape, blk.wk.shape, blk.wv.shape, blk.proj.shape))
    print("   attention bias %s  heads=%d  window=%dx%d (%d tokens)"
          % (blk.attn_bias.shape, C // HEAD_DIM, WINDOW, WINDOW, TOKENS))

    print("\n== window partition round-trip ==")
    x = np.random.default_rng(0).standard_normal((16, 16, 8)).astype(np.float32)
    assert np.array_equal(window_reverse(window_partition(x), 16, 16), x)
    print("   (16,16,8) -> %s -> back: exact" % (window_partition(x).shape,))

    print("\n== one attention window on real weights ==")
    rng = np.random.default_rng(1)
    tok = (rng.standard_normal((TOKENS, C)) * 0.1).astype(np.float16)
    out = blk.attend(tok)
    print("   input  %s  finite=%s" % (tok.shape, np.isfinite(tok).all()))
    print("   output %s  finite=%s  mean=%+.4g  sd=%.4g  range [%+.3g, %+.3g]"
          % (out.shape, np.isfinite(out).all(), out.mean(), out.std(), out.min(), out.max()))

    print("\n== attention weights are a proper distribution ==")
    q = linear(tok, blk.wq); k = linear(tok, blk.wk)
    qq = q.reshape(TOKENS, C // HEAD_DIM, HEAD_DIM).transpose(1, 0, 2)
    kk = np.repeat(k.reshape(TOKENS, (C // HEAD_DIM) // GQA_RATIO, HEAD_DIM).transpose(1, 0, 2),
                   GQA_RATIO, axis=0)
    p = softmax(qq @ kk.transpose(0, 2, 1) / np.sqrt(HEAD_DIM) + blk.attn_bias)
    print("   rows sum to 1: max deviation %.3e" % np.abs(p.sum(-1) - 1).max())
    print("   min prob %.3e   max prob %.4f   entropy/head %s"
          % (p.min(), p.max(), np.round(-(p * np.log(p + 1e-30)).sum(-1).mean(-1)[:4], 3).tolist()))
    return 0


if __name__ == "__main__":
    raise SystemExit(_selftest())
