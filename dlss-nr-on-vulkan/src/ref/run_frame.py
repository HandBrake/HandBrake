#!/usr/bin/env python3
"""
run_frame — put an image in, get an image out.

The visual acceptance loop. A correctly implemented network returns a recognisable
frame; a wrong one returns noise. That is a test that needs no reference activations,
which is the one thing this project cannot obtain, so it is the honest way to tell
how close the reconstruction actually is.

Stand-ins are marked. The stem (`input_adapter_weight`) and the output head are not
yet role-assigned, so RGB is tiled up to the stage width on the way in and the first
three channels are taken on the way out. Those two will be replaced as their layouts
are recovered; everything between them is the recovered network.
"""
import sys
import time
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent / "gpu"))
from hnet_model import Model
from hnet_ops import (softmax, ptx_softmax, l2_normalize, cos_skip, HEAD_DIM, TOKENS,
                      GQA_RATIO, window_partition, window_reverse)
from image_io import load, save, test_pattern, smooth_pattern, clean_and_noisy, denoise_score

GEOMETRY = {32: (512, 256), 64: (256, 128), 128: (128, 64), 256: (64, 32), 512: (32, 16), 1024: (16, 8)}

# Encoder stage -> the last block of that stage, and the decoder block where its
# output rejoins. A U-Net carries detail across these; without them the four
# downsamples destroy everything and the output is a flat mosaic. The kernel names
# BSUpsampleSkip / SubtiledSkipBlend / cc_tinlayout_upsample_skip_block confirm the
# connections exist; the exact blend is unknown, so a plain add is used.
SKIP = {4: 66, 8: 62, 14: 56, 22: 48, 30: 40}


use_pre_proj = True
use_attn_bias = True
use_true_softmax = False
use_exp_scale = False
use_scale_f32 = True
branch_gain = 1.0


def attend(blk, x, mm, shift=0):
    """
    `shift` rolls the feature map by half a window before partitioning and back
    afterwards -- Swin's SW-MSA. Without it every window is attended in isolation and
    the boundaries show as hard 8x8 seams. The kernels carry explicit `_shifted_`
    variants, so the alternation is real; which blocks are shifted is inferred here
    as "every other one in a stage", the canonical arrangement.
    """
    if shift:
        x = np.roll(x, (-shift, -shift), axis=(0, 1))
    H, W, C = x.shape
    h = C // HEAD_DIM
    kv = h // GQA_RATIO
    if "wq" not in blk.t or kv < 1:
        return None
    win = window_partition(x.astype(np.float16))
    n = win.shape[0]
    flat = win.reshape(-1, C)
    q = mm(flat, blk.t["wq"]).reshape(n, TOKENS, h, HEAD_DIM).transpose(0, 2, 1, 3)
    k = mm(flat, blk.t["wk"]).reshape(n, TOKENS, kv, HEAD_DIM).transpose(0, 2, 1, 3)
    v = mm(flat, blk.t["wv"]).reshape(n, TOKENS, kv, HEAD_DIM).transpose(0, 2, 1, 3)
    q = l2_normalize(q); k = l2_normalize(k)
    k = np.repeat(k, GQA_RATIO, axis=1); v = np.repeat(v, GQA_RATIO, axis=1)
    lg = q @ k.transpose(0, 1, 3, 2)
    if blk.scale_f32 is not None and use_scale_f32:
        # The `2H` run is H FP32 scalars, not 2H FP16 ones -- the "even slots" are the
        # low halves, not parameters. notes/phase5-attn-scale-fp32.md.
        lg = lg * blk.scale_f32.reshape(1, -1, 1, 1)
    elif "attn_scale" in blk.t:
        sc = blk.t["attn_scale"].astype(np.float32).reshape(1, -1, 1, 1)
        # HYPOTHESIS: `attn_scale` is stored as a **log**, the way Swin-V2 stores its
        # cosine-attention temperature (`logit_scale`, initialised to log 10 = 2.303;
        # our values are 1.11-2.77). Evidence, not proof: used raw, the logits reach
        # only +-1.0 to +-1.6 and every attention row comes out uniform to within
        # 0.01 nats of the 4.159 maximum -- the attention would be doing nothing at
        # all. Exponentiated they reach +-3.2 to +-8.8, which is the range the
        # kernels' hard +-6 clamp exists to bound, and entropy falls to 3.84.
        lg = lg * (np.exp(sc) if use_exp_scale else sc)
    if use_attn_bias and "attn_bias" in blk.t:
        lg = lg + blk.t["attn_bias"].astype(np.float32)[None]
    # The kernels clamp the logits and exponentiate with a bit trick; there is no
    # max subtraction, so this is NOT shift-invariant (notes/phase5-softmax-found.md).
    a = (softmax(lg) if use_true_softmax else ptx_softmax(lg, variant="swin")) @ v
    a = a.transpose(0, 2, 1, 3).reshape(n, TOKENS, C)
    pj = blk.t.get("proj")
    if pj is None and use_pre_proj:
        pj = blk.t.get("pre_proj")          # HYPOTHESIS, see hnet_model._fused_swin
    if pj is not None:
        a = mm(a.reshape(-1, C).astype(np.float16), pj).reshape(n, TOKENS, C)
    out = window_reverse(a, H, W)
    return np.roll(out, (shift, shift), axis=(0, 1)) if shift else out


def resize(x, hw, mode="bilinear"):
    """
    STAND-IN for the learned down/upsample layers (`cc_dec_input_upsample`, the
    `_upsample` kernel variants, `BSUpsampleSkip`). Nearest-neighbour replication
    through four levels is what produces the blocky mosaic in the output, so this
    defaults to bilinear -- still a stand-in, but one that does not manufacture
    artefacts of its own.
    """
    H, W, C = x.shape
    th, tw = hw
    if mode == "nearest":
        yi = (np.arange(th) * H // th).clip(0, H - 1)
        xi = (np.arange(tw) * W // tw).clip(0, W - 1)
        return x[np.ix_(yi, xi, np.arange(C))]
    fy = (np.arange(th) + 0.5) * H / th - 0.5
    fx = (np.arange(tw) + 0.5) * W / tw - 0.5
    y0 = np.floor(fy).astype(int).clip(0, H - 1); y1 = (y0 + 1).clip(0, H - 1)
    x0 = np.floor(fx).astype(int).clip(0, W - 1); x1 = (x0 + 1).clip(0, W - 1)
    wy = (fy - y0).clip(0, 1)[:, None, None]; wx = (fx - x0).clip(0, 1)[None, :, None]
    top = x[y0][:, x0] * (1 - wx) + x[y0][:, x1] * wx
    bot = x[y1][:, x0] * (1 - wx) + x[y1][:, x1] * wx
    return (top * (1 - wy) + bot * wy).astype(np.float32)


def refit(x, C):
    cur = x.shape[-1]
    if cur == C:
        return x
    return np.tile(x, (1, 1, -(-C // cur)))[:, :, :C] if cur < C else x[:, :, :C]


def main():
    use_gpu = "--gpu" in sys.argv
    mm = (__import__("xmx").gemm if use_gpu
          else (lambda A, B: A.astype(np.float32) @ B.astype(np.float32)))
    root = HERE.parents[1]
    src = sys.argv[1] if len(sys.argv) > 1 and not sys.argv[1].startswith("-") else None
    if src:
        # A real frame is the CLEAN reference; the noise is added here, exactly as for
        # the synthetic patterns. Setting clean == img (as this did before) leaves the
        # metric with no noise to remove and makes it degenerate.
        sigma = float(sys.argv[sys.argv.index("--sigma") + 1]) if "--sigma" in sys.argv else 0.06
        clean = load(src, (512, 256))
        rng = np.random.default_rng(1000)
        img = np.clip(clean + rng.standard_normal(clean.shape).astype(np.float32) * sigma, 0, 1)
    else:
        seed = int(sys.argv[sys.argv.index("--seed") + 1]) if "--seed" in sys.argv else 0
        sigma = float(sys.argv[sys.argv.index("--sigma") + 1]) if "--sigma" in sys.argv else 0.06
        if "--smooth" in sys.argv:
            clean = smooth_pattern(512, 256, seed)
            rng = np.random.default_rng(seed + 1000)
            img = np.clip(clean + rng.standard_normal(clean.shape).astype(np.float32) * sigma, 0, 1)
        else:
            clean, img = clean_and_noisy(512, 256, seed=seed, sigma=sigma)
    save(img, root / "work" / "frame_in.png")

    m = Model(str(root / "work" / "weights_ht.bin"))
    stem = m.blocks[0].t.get("input_adapter")
    if stem is not None and "--stem" in sys.argv:
        # The real input is not RGB. The documented contract is colour(3) + motion
        # vectors RG16F(2) + raw depth R32F(1) + a trust mask R8(1), evaluated as
        # DLAA 1:1, plus temporal state the network carries itself. For one still
        # frame the honest synthetic input is zero motion, constant depth and a
        # fully-trusted mask -- which is a *valid* input, unlike tiling RGB up to 32
        # channels. Channel ORDER inside the 16 is not established; RGB first is a
        # guess and the remaining slots are left at zero.
        # OPT-IN (--stem): wiring it this way scores 41.96x against 1.00x for the
        # stand-in, so the role, the orientation or the channel order is wrong.
        # See notes/phase5-stem.md -- the localisation holds, the reading does not.
        # Channel order read out of the pre-block's shared store
        # (notes/phase5-channel-order.md), not guessed:
        #   ch4,5,6 = colour RGB, ch7,8,9 = history RGB (same affine normalisation),
        #   ch12,13,14 = +-1 validity flags, the rest uniforms.
        # A still frame has no motion, so history == colour and the flags are valid.
        H_, W_ = img.shape[:2]
        inp = np.zeros((H_, W_, 16), np.float32)
        inp[:, :, 4:7] = img[:, :, :3]      # colour
        inp[:, :, 7:10] = img[:, :, :3]     # history: a still frame reprojects to itself
        inp[:, :, 12:15] = 1.0              # validity flags, selp picks +1
        x = (inp.reshape(-1, 16) @ stem.astype(np.float32)).reshape(H_, W_, 32)
        stem_note = "input_adapter 16->32"
    else:
        x = refit(img.astype(np.float32), 32)      # STAND-IN
        stem_note = "STAND-IN (RGB tiled to 32)"
    # `x0` runs the identical scaffolding -- the same resamples, the same skip
    # blends -- with every network branch omitted. Scoring against it separates
    # "the network helps" from "the stand-ins hurt". Without this the acceptance
    # test cannot see the network at all: every figure recorded in
    # notes/phase5-visual-loop.md (5.13x, 1.45x, 1.43x) reproduces exactly with
    # --no-attend, because the branches move the output by 3e-5.
    x0 = x.copy()
    t0 = time.time()
    skips = {}
    skips0 = {}
    stage_pos = {}
    use_skips = "--no-skips" not in sys.argv
    use_shift = "--no-shift" not in sys.argv
    globals()["use_pre_proj"] = "--no-pre-proj" not in sys.argv
    globals()["use_attn_bias"] = "--no-attn-bias" not in sys.argv
    globals()["use_true_softmax"] = "--true-softmax" in sys.argv
    globals()["use_exp_scale"] = "--exp-scale" in sys.argv
    globals()["use_scale_f32"] = "--scale-f16" not in sys.argv
    globals()["branch_gain"] = (float(sys.argv[sys.argv.index("--branch-gain") + 1])
                                if "--branch-gain" in sys.argv else 1.0)
    use_attend = "--no-attend" not in sys.argv
    use_ffn = "--no-ffn" not in sys.argv
    trace = "--trace" in sys.argv
    # HARDWARE-CONFIRMED (notes/phase5-gate-on-skip.md): the kernels compute
    #     D = A.B + (gate (*) x)
    # -- the per-channel gate multiplies the SKIP and is placed in the mma's C
    # (accumulator) operand of a fresh chain, while the branch A.B is added
    # unscaled. Our previous default, `gated_branch`, had it exactly backwards.
    gate_form = "gated_skip"
    for gf in ("gated_branch", "convex", "convex_inv"):
        if "--gate-" + gf.replace("_", "-") in sys.argv:
            gate_form = gf
    skip_mode = "add"
    for mmode in ("mean", "rms"):
        if "--skip-" + mmode in sys.argv:
            skip_mode = mmode
    for blk in m.blocks:
        if use_skips and blk.index in SKIP:
            skips[SKIP[blk.index]] = x.copy()
            skips0[SKIP[blk.index]] = x0.copy()
        if blk.C in GEOMETRY:
            x = refit(resize(x, GEOMETRY[blk.C]), blk.C)
            x0 = refit(resize(x0, GEOMETRY[blk.C]), blk.C)
        if blk.index in skips:
            s_ = refit(resize(skips.pop(blk.index), x.shape[:2]), x.shape[-1])
            s0 = refit(resize(skips0.pop(blk.index), x0.shape[:2]), x0.shape[-1])
            x0 = (x0 + s0 if skip_mode == "add" else
                  0.5 * (x0 + s0) if skip_mode == "mean" else
                  (x0 + s0) * (np.sqrt((x0 * x0).mean())
                               / max(float(np.sqrt(((x0 + s0) ** 2).mean())), 1e-9)))
            # The kernel is named SubtiledSkipBlend -- "blend", not "add". A plain
            # sum doubles the magnitude at each of the five levels, which is a good
            # candidate for the scale drift in the output.
            if skip_mode == "add":
                x = x + s_
            elif skip_mode == "mean":
                x = 0.5 * (x + s_)
            elif skip_mode == "rms":
                y = x + s_
                x = y * (np.sqrt((x * x).mean()) / max(float(np.sqrt((y * y).mean())), 1e-9))
        sh = 0
        if use_shift and blk.kind in ("fused_swin", "split_swin_16h"):
            stage_pos[blk.C] = stage_pos.get(blk.C, 0) + 1
            sh = 4 if stage_pos[blk.C] % 2 == 0 else 0
        use_rs = "--resample" in sys.argv   # OPT-IN: degrades, see notes/phase5-resample.md
        if use_rs and blk.resample is not None and blk.resample_at == "front":
            # RECOVERED upsample: applied before the block does its work.
            H_, W_, C_ = x.shape
            x = mm(x.reshape(-1, C_).astype(np.float16), blk.resample).reshape(H_, W_, C_)
        if blk.kind == "dec_upsample" and "--no-dec-upsample" not in sys.argv:
            # RECOVERED: block39 is `512 x 512` plus a `512` bias -- the container
            # holds 262,656 = 512^2 + 512 exactly, and the kernel
            # cc_dec_input_upsample_1024_512 is one GEMM and nothing else (64 mma,
            # 16 chains, A from shared, B from the weight arena). The "1024_512" in
            # the name is the stage transition, not the matrix shape.
            # It is the only layer in the model carrying a bias rather than a gate.
            # The spatial half of the upsample is still the bilinear stand-in.
            H_, W_, _ = x.shape
            xs = refit(resize(x, GEOMETRY[512]), 512)
            wv = blk.t["weight"].astype(np.float32).reshape(512, 512)
            bv = blk.t["bias"].astype(np.float32)
            x = (mm(xs.reshape(-1, 512).astype(np.float16), wv) + bv).reshape(
                GEOMETRY[512][0], GEOMETRY[512][1], 512)
        br = (attend(blk, x, mm, sh)
              if (use_attend and blk.kind in ("fused_swin", "split_swin_16h")) else None)
        if br is not None and branch_gain != 1.0:
            br = br * branch_gain
        if br is not None:
            g = next((blk.t[n] for n in ("gate", "ffwd_gate", "ffwd_proj_gate")
                      if n in blk.t and blk.t[n].size == blk.C), None)
            if trace:
                gs = float(np.abs(g.astype(np.float32)).mean()) if g is not None else 1.0
                print("   blk%-3d C=%-5d trunk sd %.4g  branch sd %.4g  |gate| %.4g"
                      "  -> branch/trunk %.3g"
                      % (blk.index, blk.C, x.std(), br.std(), gs,
                         br.std() * gs / max(float(x.std()), 1e-30)))
            x = cos_skip(br, x, g.astype(np.float32), gate_form) if g is not None else x + br
            if use_rs and blk.resample is not None and blk.resample_at == "back":
                pass                                  # applied after the block, below
            if use_ffn and "pre_ffn0" in blk.t:
                # HYPOTHESIS: the leading region continues [C^2 proj][C^2/2 ffn0][C^2/2 ffn1]
                H_, W_, C_ = x.shape
                f = x.reshape(-1, C_).astype(np.float16)
                h_ = mm(f, blk.t["pre_ffn0"])
                h_ = h_ * (h_ > 0)
                x = x + mm(h_.astype(np.float16), blk.t["pre_ffn1"]).reshape(H_, W_, C_)
        if use_rs and blk.resample is not None and blk.resample_at == "back":
            # RECOVERED downsample: applied after the block's work.
            H_, W_, C_ = x.shape
            x = mm(x.reshape(-1, C_).astype(np.float16), blk.resample).reshape(H_, W_, C_)
    dt = time.time() - t0

    head = m.blocks[70].out_head
    if head is not None and "--slice-head" not in sys.argv:
        # The recovered head: 32 -> 4 channels (notes/phase5-output-head.md). The
        # first three are taken as RGB; what the fourth carries is not established,
        # and it has the largest column norm of the four.
        H_, W_ = x.shape[:2]
        out4 = (x.reshape(-1, 32) @ head).reshape(H_, W_, 4)
        out = out4[:, :, :3]
        head_note = "out_head 32->4 (RGB = first 3)"
    else:
        out = x[:, :, :3]                           # STAND-IN
        head_note = "STAND-IN (first 3 channels)"
    lo, hi = float(out.min()), float(out.max())
    norm = (out - lo) / max(hi - lo, 1e-9)
    save(norm, root / "work" / "frame_out.png")
    out0 = (x0.reshape(-1, 32) @ head).reshape(x0.shape[0], x0.shape[1], 4)[:, :, :3] \
        if (head is not None and "--slice-head" not in sys.argv) else x0[:, :, :3]
    lo0, hi0 = float(out0.min()), float(out0.max())
    norm0 = (out0 - lo0) / max(hi0 - lo0, 1e-9)

    print("=== frame through the recovered network (%s) ===" % ("XMX" if use_gpu else "CPU"))
    print("   input   %s  range [%.3f, %.3f]" % (img.shape, img.min(), img.max()))
    print("   output  %s  range [%.4g, %.4g]  sd %.4g" % (out.shape, lo, hi, out.std()))
    print("   wall    %.1f s" % dt)
    # how much of the input structure survived?
    a = img[:, :, :3].reshape(-1)
    b = np.repeat(norm.reshape(-1, 3), 1, axis=0).reshape(-1)
    c = float(np.corrcoef(a, b)[0, 1])
    print("   correlation of output with input: %+.4f   skips=%s(%s) shift=%s proj=%s ffn=%s gate=%s"
          % (c, use_skips, skip_mode, use_shift, use_pre_proj, use_ffn, gate_form))
    print("   stem:      %s" % stem_note)
    print("   head:      %s" % head_note)
    print("   attention: on=%s scale=%s exp_scale=%s branch_gain=%g"
          % (use_attend, "fp32-pairs" if use_scale_f32 else "f16-odd-slots",
             use_exp_scale, branch_gain))
    print("   attention: %s   bias=%s"
          % ("textbook max-subtracted softmax" if use_true_softmax
             else "PTX clamped softmax (logits +-6, bit-trick exp)", use_attn_bias))
    base, _ = denoise_score(clean, img)
    got, keep = denoise_score(clean, norm)
    # A pure pass-through scores exactly 1.00x, so a score near 1.0 is not a result --
    # it means the network was bypassed. Correlation with the input at 1.0000 is the
    # giveaway: the output is a linear function of the input and nothing else.
    if got == float("inf"):
        verdict = "COLLAPSED (kept %.3f of the spread)" % keep
    elif abs(c) > 0.9995:
        verdict = "PASS-THROUGH -- the network contributed nothing (corr %+.4f)" % c
    elif got < base:
        verdict = "IMPROVED %.2fx" % (base / got)
    else:
        verdict = "WORSE %.2fx" % (got / base)
    print("   denoise: input %.5f -> output %s   spread kept %.3f   %s"
          % (base, ("inf" if got == float("inf") else "%.5f" % got), keep, verdict))
    # What did the NETWORK do, as opposed to the resampling and skip scaffolding it
    # sits in? `scaf` is the same pipeline with every branch removed.
    scaf, _ = denoise_score(clean, norm0)
    moved = float(np.abs(norm - norm0).mean() / max(float(np.abs(norm0).mean()), 1e-30))
    print("   scaffolding alone (no branches): %.5f   network moved the frame by %.3g relative"
          % (scaf, moved))
    if moved < 1e-3:
        print("   NETWORK-INERT -- the branches contribute nothing; this score measures"
              " the stand-in resampling and skip blend, not the recovered model")
    else:
        print("   network vs its own scaffolding: %s %.3fx"
              % ("BETTER" if got < scaf else "worse", max(scaf, got) / max(min(scaf, got), 1e-30)))
    print("   output/input scale: sd %.3f vs %.3f   mean %+.3f vs %+.3f"
          % (out.std(), img.std(), out.mean(), img.mean()))
    # seam metric: energy at the 8-pixel window boundaries versus everywhere else
    g = norm.mean(axis=2)
    dv = np.abs(np.diff(g, axis=0)); dh = np.abs(np.diff(g, axis=1))
    bv = dv[7::8].mean(); ov = np.delete(dv, np.arange(7, dv.shape[0], 8), axis=0).mean()
    bh = dh[:, 7::8].mean(); oh = np.delete(dh, np.arange(7, dh.shape[1], 8), axis=1).mean()
    print("   window-seam ratio (boundary gradient / interior): %.3f vertical, %.3f horizontal"
          % (bv / max(ov, 1e-9), bh / max(oh, 1e-9)))
    print("   wrote work/frame_in.png and work/frame_out.png")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
