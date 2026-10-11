#!/usr/bin/env python3
"""Tally the GEMM shapes one frame records, and dump them for the replay bench."""
import collections, json, pathlib, sys
import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "gpu"))
sys.path.insert(0, str(ROOT / "src" / "ref"))

import xmxres, nr_model, nr_frame_resident as F

TALLY, CALLS = collections.Counter(), collections.Counter()
real_gemm = xmxres.Runtime.gemm


def gemm(self, a, b, c, rows, cols, inner, *, batch=1, transpose_b=False, **kw):
    key = (rows, cols, inner, batch, bool(transpose_b))
    TALLY[key] += 2.0 * rows * cols * inner * batch
    CALLS[key] += 1
    return real_gemm(self, a, b, c, rows, cols, inner, batch=batch, transpose_b=transpose_b, **kw)


xmxres.Runtime.gemm = gemm

H = int(sys.argv[1]) if len(sys.argv) > 1 else 768
W = int(sys.argv[2]) if len(sys.argv) > 2 else 1280
model = nr_model.NeuralRenderingModel.from_safetensors(ROOT / "work" / "mlxw" / "dlssnr-logical.safetensors")
rt = xmxres.Runtime()
F.ResidentFrame(rt, model.weights, H, W).run(np.zeros((H, W, 16), dtype=np.float32))

total = sum(TALLY.values())
print(f"network extent {H}x{W}: {sum(CALLS.values())} GEMM dispatches, "
      f"{total/1e9:.1f} GFLOP over {len(TALLY)} distinct shapes\n")
print("  %-30s %8s %10s %7s" % ("M x N x K (batch)", "calls", "GFLOP", "share"))
for key, flops in TALLY.most_common(20):
    m, n, k, b, t = key
    label = f"{m} x {n} x {k}" + (f" (b={b})" if b > 1 else "") + (" T" if t else "")
    print("  %-30s %8d %10.2f %6.1f%%" % (label, CALLS[key], flops / 1e9, 100 * flops / total))

out = ROOT / "work" / f"shapes-{H}x{W}.json"
out.write_text(json.dumps([{"m": k[0], "n": k[1], "k": k[2], "batch": k[3],
                            "transpose_b": k[4], "calls": CALLS[k]} for k in TALLY]))
print(f"\nwrote {out}")
