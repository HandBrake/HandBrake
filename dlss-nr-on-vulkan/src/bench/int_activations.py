#!/usr/bin/env python3
"""The other half of the integer question: the activations.

Cooperative-matrix config 4 is `sint8 x sint8 -> sint32` — **both** operands integer.
There is no mixed mode on this hardware, so integer weights force integer activations,
and that is the side where a graph built on E4M3 publishes has something to lose: E4M3
spends four bits on the exponent and keeps 6.25 % *relative* precision across seventeen
binades, while int8 lays an absolute grid over one.
"""
import pathlib, sys
import numpy as np
ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "gpu")); sys.path.insert(0, str(ROOT / "src" / "ref"))
import xmxres, nr_model, nr_frame_resident as F

model = nr_model.NeuralRenderingModel.from_safetensors(ROOT / "work" / "mlxw" / "dlssnr-logical.safetensors")
rt = xmxres.Runtime()
capture = {}
F.ResidentFrame(rt, model.weights, 384, 384).run(
    (np.random.default_rng(11).standard_normal((384, 384, 16)) * 0.3).astype(np.float32),
    capture=capture)

print("  %-12s %10s %11s %11s %11s %11s"
      % ("published", "values", "|max|", "dynamic", "int8 -> 0", "int8 rel err"))
for name, data in capture.items():
    v = np.abs(np.asarray(data, np.float32).reshape(-1))
    v = v[np.isfinite(v)]
    nz = v[v > 0]
    if nz.size < 1000:
        continue
    step = float(v.max()) / 127.0
    lost = float((nz < step / 2).mean())
    q = np.round(nz / step) * step
    rel = float(np.mean(np.abs(q - nz) / nz))
    print("  %-12s %10d %11.4g %10.0fx %10.1f%% %11.2f%%"
          % (name, v.size, v.max(), v.max() / max(nz.min(), 1e-30), 100 * lost, 100 * rel))
print("\n  `int8 -> 0` is the share of non-zero values that a per-tensor int8 grid rounds"
      "\n  away entirely; `rel err` is the mean relative error on the survivors. E4M3 keeps"
      "\n  6.25 % on all of them, which is the step the graph is built around.")
