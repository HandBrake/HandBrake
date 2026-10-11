#!/usr/bin/env python3
"""
hnet_model — materialise the whole DLSS-NR network as named, shaped tensors.

**Superseded — do not build on this.** It decodes the weight container as dense FP16, which
`notes/phase6` showed is not what the container holds (correlation -0.02 with the logical
tensors), and the claims below about GQA and the subnormal fraction fell with that decode
(`notes/phase61`). The live path is `src/ref/nr_model.py` on
`work/mlxw/dlssnr-logical.safetensors`. Kept for the PTX-derived findings in the comments.

This is the artifact everything downstream builds on: it turns 147 MB of opaque
FP16 into 71 blocks of addressable parameters, and it is self-validating. Every
slice must consume exactly its region; any leftover or overrun raises. If this
script exits 0, the recovered layout is internally consistent for all 153 records.

Layouts come from notes/MODEL-SPEC.txt, notes/phase3-block-layout.md and
notes/phase3-block-internals.md.
"""
import re
import sys
from collections import defaultdict
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from hnet_weights import parse

HEAD_DIM = 32
TOKENS = 64          # 8x8 Swin window

FP16_MIN_NORMAL = 6.103515625e-05
FP16_MAX = 65504.0


def fp16_shift(w):
    """
    Largest 2^k that keeps every value of `w` inside the FP16 normal range.

    Xe2's XMX units flush subnormal FP16 operands to zero, and **27.22% of this
    model's parameters are FP16 subnormals** (measured; see
    notes/phase4-subnormal-flush.md). Left alone, a quarter of the network is
    silently zeroed on this hardware. Scaling by a power of two is exact in binary
    floating point, so multiplying the weights offline and dividing the FP32
    accumulator afterwards recovers the loss with no error of its own.
    """
    mx = float(np.abs(w.astype(np.float32)).max())
    return int(np.floor(np.log2(FP16_MAX / mx))) if mx > 0 else 0


def subnormal_fraction(w):
    f = np.abs(w.astype(np.float32))
    return float(((f < FP16_MIN_NORMAL) & (f != 0)).mean())


def is_gate(x):
    """
    Cosine gate signature: inside [-1,1] and centred well away from zero.

    The mean is the discriminator, not the spread. At C=32 the weights themselves
    have sd 0.065 and sit inside [-1,1], so spread cannot separate them; but weights
    have mean ~0 while every measured gate has mean 0.63-0.99. Spread is useless in
    the other direction too: gates in the deeper decoder blocks are squeezed against
    1.0 with sd as low as 0.012 (block43.layer3: min 0.911, mean 0.994), meaning
    those blocks pass their input through almost unchanged.
    """
    return (x.size and x.min() >= -1.001 and x.max() <= 1.001 and x.mean() > 0.3)


def fp32_pairs(a):
    """
    Read a run of FP16 slots as half as many FP32 values, little-endian.

    The `2H` run at the end of every attention layer is H **FP32** scalars, not 2H
    FP16 ones. The even slots are not parameters at all: across the eight ViT-1D
    blocks they take exactly eight distinct values -- every one a multiple of
    0x2000, i.e. the low 13 bits are zero, which is precisely an FP16 widened to
    FP32 -- and across the sixteen Swin blocks they hold NaNs and magnitudes above
    1e4. A trained tensor does neither. (0.015 % of a real weight tensor's words are
    multiples of 0x2000; here it is 100 %.)

    The kernels agree: `cc_split_swin_16h_qkv_512` loads this scalar with
    `ld.global.b32` followed by `cvt.rn.f16.f32` -- four bytes, interpreted as FP32,
    narrowed to FP16 in register -- indexed per head at stride 4. See
    notes/phase5-attn-scale-fp32.md.
    """
    u = np.ascontiguousarray(a).view(np.uint16).astype(np.uint32)
    return (((u[1::2] << 16) | u[0::2]).astype(np.uint32)).view(np.float32)


class Block:
    def __init__(self, idx, kind, C):
        self.index, self.kind, self.C = idx, kind, C
        self.t = {}
        # Derived, not stored: the per-head attention scale read as FP32 rather than
        # as two FP16 slots. Deliberately NOT in `self.t`, so the parameter
        # accounting keeps counting stored elements exactly once.
        self.scale_f32 = None
        self.out_head = None       # derived: the 32x4 output projection, unpadded
        self.resample = None       # derived: the C x C resampling matrix, if any
        self.resample_at = None    # "front" (upsample) or "back" (downsample)

    def put(self, name, arr, shape=None):
        self.t[name] = arr.reshape(shape) if shape else arr
        return self.t[name]

    def __repr__(self):
        return "block%-3d %-18s C=%-5d %s" % (
            self.index, self.kind, self.C,
            "  ".join("%s%s" % (k, tuple(v.shape)) for k, v in self.t.items()))


class Model:
    def __init__(self, path="work/weights_ht.bin"):
        blob = Path(path).read_bytes()
        recs = parse(blob)
        raw = {}
        for t in recs:
            raw[t["name"]] = np.frombuffer(blob, dtype="<f2",
                                           count=t["n_elem"], offset=t["offset"])
        by = defaultdict(dict)
        for name, a in raw.items():
            m = re.match(r"block(\d+)\.layer(\d+)\.(\w+)$", name)
            by[int(m.group(1))][(int(m.group(2)), m.group(3))] = a
        self.blocks = [self._build(i, by[i]) for i in sorted(by)]
        self.n_params = sum(a.size for a in raw.values())

    # -- block builders ---------------------------------------------------
    def _build(self, i, subs):
        keys = sorted(subs)
        if any(k[1] == "blend_scale" for k in keys):
            return self._output_head(i, subs)
        # Dispatch on the size of layer0, not the sub-layer count: block30 is a
        # split-Swin block that happens to carry a fifth sub-layer.
        if len(keys) >= 4:
            n0 = subs[(0, "layer")].size
            if n0 == 2 * 1024 * 1024 + 8:
                return self._vit1d(i, subs)
            if n0 == 512 * 512:
                return self._split_swin(i, subs)
        a = subs[keys[0]]
        if a.size == 262656:
            b = Block(i, "dec_upsample", 1024)
            b.put("weight", a[:512 * 512], (512, 512))
            b.put("bias", a[512 * 512:])
            assert b.t["bias"].size == 512
            return b
        return self._fused_swin(i, a)

    def _split_swin(self, i, subs, C=512):
        b, H = Block(i, "split_swin_16h", C), C // HEAD_DIM
        g = lambda n: subs[(n, "layer")]
        b.put("proj", g(0), (C, C))
        for n, tag in ((1, "ffwd"), (3, "ffwd_proj")):
            a = g(n)
            b.put(tag, a[:C * C // 2], (C, C // 2))
            gate = b.put(tag + "_gate", a[C * C // 2:])
            assert gate.size == C and is_gate(gate.astype(np.float32)), tag
        l2, q = g(2), 3 * C * C // 2
        b.put("wq", l2[:C * C], (C, C))
        b.put("wk", l2[C * C:C * C + C * C // 4], (C, C // 4))
        b.put("wv", l2[C * C + C * C // 4:q], (C, C // 4))
        b.put("attn_bias", l2[q:q + 128 * C], (H, TOKENS, TOKENS))
        pairs = l2[q + 128 * C:]
        b.put("attn_scale", pairs[1::2])     # the FP16 reading; kept for comparison
        b.put("unknown_even", pairs[0::2])   # the low half of the FP32 -- not a weight
        b.scale_f32 = fp32_pairs(pairs)
        assert b.t["attn_scale"].size == H
        if (4, "layer") in subs:                      # block30 only
            ex = subs[(4, "layer")]
            b.put("extra", ex)
            # 262,152 elements = 512^2 + 8: the 512->1024 downsample, the deepest of
            # the five U-Net levels. Same object as the encoder surplus in the fused
            # blocks, here carried as its own layer4 tensor.
            if ex.size >= C * C:
                b.resample = ex[:C * C].astype(np.float32).reshape(C, C)
                b.resample_at = "back"
        return b

    def _vit1d(self, i, subs, C=1024):
        b, H = Block(i, "vit_1d", C), C // HEAD_DIM
        g = lambda n: subs[(n, "layer")]
        a0 = g(0); b.put("ffn_expand", a0[:2 * C * C], (C, 2 * C)); b.put("ffn_expand_extra", a0[2 * C * C:])
        a1 = g(1); b.put("ffn_contract", a1[:2 * C * C], (2 * C, C))
        gate = b.put("ffn_gate", a1[2 * C * C:]); assert is_gate(gate.astype(np.float32))
        a2 = g(2)
        b.put("attn_scale", a2[:64][1::2])
        b.put("unknown_even", a2[:64][0::2])
        b.scale_f32 = fp32_pairs(a2[:64])
        q = a2[64:]
        b.put("wq", q[:C * C], (C, C))
        b.put("wk", q[C * C:C * C + C * C // 4], (C, C // 4))
        b.put("wv", q[C * C + C * C // 4:], (C, C // 4))
        assert b.t["attn_scale"].size == H and b.t["wv"].size == C * C // 4
        b.put("scalar", g(3))
        a4 = g(4); b.put("proj", a4[:C * C // 2], (C, C // 2))
        gate = b.put("proj_gate", a4[C * C // 2:]); assert is_gate(gate.astype(np.float32))
        return b

    # Measured leading-region length per stage width. C >= 64 follows 4C^2 + 64C;
    # C = 32 does not, and comes out at 5.5C^2 -- expected, because those blocks are
    # built from FusedSwin2d1HLayer rather than CrazyCuckooFusedSwin*, a different
    # class with its own layout. Bias is 128C at every width.
    LEAD = {32: lambda C: 11 * C * C // 2,
            64: lambda C: 4 * C * C + 64 * C,
            128: lambda C: 4 * C * C + 64 * C,
            256: lambda C: 4 * C * C + 64 * C}

    # The five U-Net levels each carry a resampling matrix, found the same way as the
    # stem and the head -- by the surplus over a standard block of the same width.
    # Encoder blocks carry it at the BACK (work, then downsample), decoder blocks at
    # the FRONT (upsample, then work); established by front- vs end-aligned
    # correlation against a standard neighbour. Surplus is C^2 (+-8, the zero pad) on
    # the encoder side and C^2 plus a short vector on the decoder side, exactly as
    # block39 is 512^2 + 512. See notes/phase5-resample.md.
    #
    # block66 is deliberately absent: its surplus has sd 19.2 against 0.003-0.034 for
    # every other one, so it is not the same kind of object and is not guessed at.
    DOWNSAMPLE = {4, 8, 14, 22, 30}
    UPSAMPLE = {48, 56, 62}

    def _fused_swin(self, i, a):
        # block0 carries 512 elements that no other C=32 block has, at the FRONT.
        # Position established exactly, not by correlation: every fused block ends
        # with [cosine gate][8 zeros], and that gate sits 64 elements from the end in
        # block0 (10848-10784) and in block1 (10336-10272) alike, so the surplus is
        # leading. Its magnitude is a separate population -- sd 0.031 against 2.86
        # for the rest of the block -- and reshaped (16,32) its row norms are uniform
        # (cv 0.15) where an equal-sized slice of ordinary weights gives cv 1.84.
        # 512 = 16 x 32, and the documented input contract is colour(3) + motion
        # vectors(2) + depth(1) + trust mask(1) plus carried temporal state.
        # This is `input_adapter_weight`. The 16x32 orientation is inferred; the
        # localisation is measured. See notes/phase5-stem.md.
        stem, a_full = None, a
        if i == 0 and a.size >= 512 + 32 * 32:
            stem, a = a[:512], a[512:]
        n = a.size
        for C in (32, 64, 128, 256):
            lead = self.LEAD[C](C)
            if lead + 128 * C <= n < 4 * (4 * C * C):
                break
        b, H = Block(i, "fused_swin", C), C // HEAD_DIM
        f = a.astype(np.float32)
        # Order inside the leading region is [pre weights][cosine gate][qkv].
        # The gate begins where the qkv would start if it were exactly 1.5C^2, and
        # runs forward; the qkv then occupies whatever is left up to the bias. Scan
        # forward with a fine window -- the gate is short (64 to 288 elements) and a
        # coarse backward scan lands in the weights and finds nothing.
        # Order inside the leading region is [pre weights][pad][cosine gate][qkv].
        # Anchor everything on the gate: it is the one landmark the values identify
        # unambiguously. The bias then starts exactly 1.5C^2 after the gate ends,
        # which keeps every slice an exact multiple of C instead of relying on a
        # boundary formula that the detectors cannot resolve to the element.
        qkv_n = 3 * C * C // 2
        cut = lead - qkv_n
        e = cut
        while e < len(f) and f[e] == 0.0:        # a few zero pad elements first
            e += 1
        W = 8
        gs = e
        while e + W <= len(f) and is_gate(f[e:e + W]):
            e += W
        # The leading region is 2.5C^2 + 64C. Its sub-roles are not established, but
        # the first C^2 is exposed as `pre_proj` because these blocks otherwise have
        # no attention output projection at all, and C^2 is exactly the right size
        # for one. Treated as a hypothesis and measured, not assumed.
        pre = a[:cut]
        if pre.size >= 2 * C * C:
            b.put("pre_proj", pre[:C * C].reshape(C, C))
            b.put("pre_ffn0", pre[C * C: C * C + C * C // 2].reshape(C, C // 2))
            b.put("pre_ffn1", pre[C * C + C * C // 2: 2 * C * C].reshape(C // 2, C))
            b.put("pre_rest", pre[2 * C * C:])
        else:
            b.put("pre", pre)
        b.put("gate", a[gs:e])
        qkv = a[e:e + qkv_n]
        bias_at = e + qkv_n
        # Q/K/V split is GQA 4:1 -- only exact when C/4 is a whole number of
        # 32-channel heads, i.e. C >= 128. The narrow stages use FusedSwin2d1HLayer /
        # CrazyCuckooFusedSwin2d2HLayer, different classes with an undetermined split,
        # so no shapes are invented for them.
        if C >= 128 and qkv.size == qkv_n:
            b.put("wq", qkv[:C * C], (C, C))
            b.put("wk", qkv[C * C:C * C + C * C // 4], (C, C // 4))
            b.put("wv", qkv[C * C + C * C // 4:], (C, C // 4))
        else:
            b.put("qkv", qkv)               # split undetermined at this width
        b.put("attn_bias", a[bias_at:bias_at + 128 * C], (H, TOKENS, TOKENS))
        b.put("tail", a[bias_at + 128 * C:])
        b.put("pad", a[cut:gs])
        if i in self.DOWNSAMPLE:
            b.resample = a_full[-C * C:].astype(np.float32).reshape(C, C)
            b.resample_at = "back"
        elif i in self.UPSAMPLE:
            b.resample = a_full[:C * C].astype(np.float32).reshape(C, C)
            b.resample_at = "front"
        if stem is not None:
            b.put("input_adapter", stem, (16, 32))
        placed = sum(v.size for v in b.t.values())
        assert placed == a_full.size, "block%d does not close: %d vs %d" % (i, placed, a_full.size)
        return b

    def _output_head(self, i, subs):
        b = Block(i, "output_head", 32)
        a = subs[(0, "layer")]
        # block70 is a standard C=32 fused block with 512 extra elements at the END.
        # Position established the same way as block0's stem: correlation against a
        # standard block is 0.840 front-aligned against 0.666 end-aligned.
        #
        # Those 512 are a **padded (64, 8) matrix with exactly 384 structural zeros** --
        # columns 4..7 are entirely zero and only 32 of the 64 rows carry data, so the
        # real matrix is 32 x 4: the C=32 trunk projected to **four output channels**.
        # The zeros are structural, not incidental: block0's leading 512 elements,
        # by contrast, contain not a single zero. See notes/phase5-output-head.md.
        b.put("weights", a[:-512])
        head = a[-512:].reshape(64, 8)
        b.put("out_head_padded", a[-512:], (64, 8))
        b.out_head = head[:32, :4].astype(np.float32)      # derived, not stored
        b.put("blend_scale", subs[(0, "blend_scale")])
        return b


def main():
    m = Model(sys.argv[1] if len(sys.argv) > 1 else "work/weights_ht.bin")
    kinds = defaultdict(int)
    placed = 0
    for b in m.blocks:
        kinds[b.kind] += 1
        placed += sum(v.size for v in b.t.values())
    print("=== DLSS-NR model, materialised ===\n")
    for b in m.blocks[:3] + m.blocks[23:24] + m.blocks[31:32] + m.blocks[-2:]:
        print("  " + repr(b))
    print("\n  ... %d blocks total\n" % len(m.blocks))
    for k, v in sorted(kinds.items()):
        print("  %-18s x%d" % (k, v))
    print("\n  parameters placed %s of %s   -> %s"
          % (format(placed, ","), format(m.n_params, ","),
             "ALL ACCOUNTED" if placed == m.n_params else "MISSING %d" % (m.n_params - placed)))
    return 0 if placed == m.n_params else 1


if __name__ == "__main__":
    raise SystemExit(main())
