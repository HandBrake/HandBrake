#!/usr/bin/env python3
"""Where a frame's time goes, by stage."""
import pathlib, sys, time
import numpy as np
ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "gpu")); sys.path.insert(0, str(ROOT / "src" / "ref"))
import xmxres, nr_model, nr_frame_resident as F

H, W = (int(sys.argv[1]), int(sys.argv[2])) if len(sys.argv) > 2 else (768, 1280)
model = nr_model.NeuralRenderingModel.from_safetensors(ROOT / "work" / "mlxw" / "dlssnr-logical.safetensors")
rt = xmxres.Runtime()
frame = F.ResidentFrame(rt, model.weights, H, W)
features = (np.random.default_rng(11).standard_normal((H, W, 16)) * 0.3).astype(np.float32)
frame.run(features)                                    # warm the clocks
timing, submits = {}, []
started = time.perf_counter()
frame.run(features, timing=timing, submits=submits)
total = time.perf_counter() - started
print("  %-38s %9s %7s %7s" % ("stage", "ms", "share", "submits"))
for name, (seconds, count) in sorted(timing.items(), key=lambda kv: -kv[1][0]):
    print("  %-38s %9.1f %6.1f%% %7d" % (name, 1000 * seconds, 100 * seconds / total, count))
print("  %-38s %9.1f %6.1f%% %7d passes" % ("TOTAL", 1000 * total, 100.0, submits[0]))
