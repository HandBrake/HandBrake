#!/usr/bin/env python3
"""Would the graph survive integer weights?

The XMX units expose `sint8 x sint8 -> sint32` at M=8 N=16 **K=32** — twice the K depth
of the fp16 config, so twice the arithmetic per instruction, and half the weight bytes.
Whether that is reachable at all depends first on whether the *representation* holds,
which is cheaper to answer than an int8 GEMM: quantise every GEMM weight, dequantise it
straight back to fp16, and run the real graph on the result.

Only the GEMM operands are touched. The gates, `attn_scale`, the attention bias and the
transition sines are uploaded as float32 and are left alone — they are a few thousand
values and structural.
"""
import pathlib, sys
import numpy as np
ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "gpu")); sys.path.insert(0, str(ROOT / "src" / "ref"))
import xmxres, nr_model, nr_frame_resident as F

real_buffer_from = xmxres.Runtime.buffer_from
STATE = {"bits": 0, "per_row": True, "error": [], "count": 0}


def quantise(w, bits, per_row):
    """Symmetric integer quantisation, dequantised back. Weights are (in, out), so a
    per-row scale here means one scale per output channel — the finest granularity a
    GEMM can apply without touching the inner loop."""
    limit = float(2 ** (bits - 1) - 1)
    axis = 0 if per_row else None
    scale = np.max(np.abs(w), axis=axis, keepdims=axis is not None) / limit
    scale = np.where(scale == 0, 1.0, scale)
    return np.round(w / scale).clip(-limit - 1, limit) * scale


def hooked(self, array, dtype=np.float32, pad=0):
    if STATE["bits"] and dtype == np.float16 and np.ndim(array) == 2:
        w = np.asarray(array, np.float32)
        q = quantise(w, STATE["bits"], STATE["per_row"])
        norm = np.linalg.norm(w)
        if norm:
            STATE["error"].append(float(np.linalg.norm(q - w) / norm))
        STATE["count"] += 1
        array = q
    return real_buffer_from(self, array, dtype, pad)


if __name__ == "__main__":
    xmxres.Runtime.buffer_from = hooked

    model = nr_model.NeuralRenderingModel.from_safetensors(ROOT / "work" / "mlxw" / "dlssnr-logical.safetensors")
    rt = xmxres.Runtime()
    H, W = 384, 384
    features = (np.random.default_rng(11).standard_normal((H, W, 16)) * 0.3).astype(np.float32)


    def run(bits, per_row):
        STATE.update(bits=bits, per_row=per_row, error=[], count=0)
        frame = F.ResidentFrame(rt, model.weights, H, W)
        head = frame.run(features)
        err = float(np.mean(STATE["error"])) if STATE["error"] else 0.0
        return head, err, STATE["count"]


    base, _, _ = run(0, True)
    print("  baseline: %d GEMM weights in float16, head sd %.4f\n" % (0, base.std()))
    print("  %-22s %10s %12s %12s %12s"
          % ("weights", "rel error", "corr vs fp16", "head sd", "max |d|"))
    for bits, per_row in ((8, True), (8, False), (6, True), (5, True), (4, True)):
        head, err, count = run(bits, per_row)
        a, b = base.reshape(-1), head.reshape(-1)
        corr = float(np.corrcoef(a, b)[0, 1])
        print("  int%-2d %-17s %10.2e %12.6f %12.4f %12.4f"
              % (bits, "per output channel" if per_row else "per tensor", err, corr,
                 head.std(), np.abs(head - base).max()))
    print("\n  %d GEMM weight matrices quantised; gates, attn_scale, the attention bias and"
          "\n  the transition sines stay float32." % count)
