#!/usr/bin/env python3
"""Frame time against network extent — what resolution reaches a given frame rate.

Extents are multiples of 64, which is what `NetworkGeometry.vendor_aligned` produces.
Each is the best of five consecutive frames after a warm-up, so the GPU is at its
1950 MHz ceiling throughout.
"""
import pathlib, sys, time
import numpy as np
ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "gpu")); sys.path.insert(0, str(ROOT / "src" / "ref"))
import xmxres, nr_model, nr_frame_resident as F

model = nr_model.NeuralRenderingModel.from_safetensors(ROOT / "work" / "mlxw" / "dlssnr-logical.safetensors")
rt = xmxres.Runtime()
rng = np.random.default_rng(11)

EXTENTS = [(256, 448), (320, 512), (384, 640), (384, 704), (448, 768), (512, 896),
           (576, 1024), (640, 1152), (768, 1280)]
print("  %-16s %10s %9s %9s %12s" % ("network extent", "pixels", "ms", "fps", "ms/Mpixel"))
base = None
for h, w in EXTENTS:
    frame = F.ResidentFrame(rt, model.weights, h, w)
    features = (rng.standard_normal((h, w, 16)) * 0.3).astype(np.float32)
    frame.run(features)
    best = min((lambda t=time.perf_counter(): (frame.run(features), time.perf_counter() - t)[1])()
               for _ in range(5))
    ms = 1000 * best
    px = h * w
    if base is None:
        base = ms / px * 1e6
    print("  %-16s %10d %9.0f %9.1f %12.1f" % (f"{w}x{h}", px, ms, 1000 / ms, ms / px * 1e6))
print("\n  30 fps is 33 ms; 60 fps is 17 ms.")
