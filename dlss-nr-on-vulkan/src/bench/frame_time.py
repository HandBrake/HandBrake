#!/usr/bin/env python3
"""Wall time of the resident graph at a given network extent, best of N."""
import pathlib, sys, time
import numpy as np
ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "gpu")); sys.path.insert(0, str(ROOT / "src" / "ref"))
import xmxres, nr_model, nr_frame_resident as F

H = int(sys.argv[1]) if len(sys.argv) > 1 else 768
W = int(sys.argv[2]) if len(sys.argv) > 2 else 1280
repeats = int(sys.argv[3]) if len(sys.argv) > 3 else 3
model = nr_model.NeuralRenderingModel.from_safetensors(ROOT / "work" / "mlxw" / "dlssnr-logical.safetensors")
rt = xmxres.Runtime()
frame = F.ResidentFrame(rt, model.weights, H, W)
rng = np.random.default_rng(11)
features = (rng.standard_normal((H, W, 16)) * 0.3).astype(np.float32)
times, head = [], None
for _ in range(repeats):
    started = time.perf_counter()
    head = frame.run(features)
    times.append(time.perf_counter() - started)
print("  %dx%d: %s ms   best %.0f ms" % (W, H, " ".join("%.0f" % (1000 * t) for t in times),
                                         1000 * min(times)))
print("  head sd %.4f  finite %s" % (head.std(), np.isfinite(head).all()))
