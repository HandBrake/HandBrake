#!/usr/bin/env python3
"""How a frame's time divides between the GEMMs and everything else.

Each half is skipped in turn. The output is meaningless; the timing is not — the
buffers keep their sizes and the dispatch pattern is otherwise identical.
Force fresh recording: cached replay would otherwise ignore the monkeypatches.
These ablations change intermediate values and are only an approximate cost split.
"""
import pathlib, sys, time
import numpy as np
ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "gpu")); sys.path.insert(0, str(ROOT / "src" / "ref"))
import xmxres, nr_model, nr_frame_resident as F

model = nr_model.NeuralRenderingModel.from_safetensors(ROOT / "work" / "mlxw" / "dlssnr-logical.safetensors")
rt = xmxres.Runtime()
frame = F.ResidentFrame(rt, model.weights, 768, 1280)
features = (np.random.default_rng(11).standard_normal((768, 1280, 16)) * 0.3).astype(np.float32)
real = {n: getattr(xmxres.Runtime, n) for n in
        ("gemm", "unary", "cosine_publish", "softmax")}

def timed(label, skip):
    for name in skip:
        setattr(xmxres.Runtime, name, lambda self, *a, **k: self)
    try:
        best = float("inf")
        for _ in range(4):
            started = time.perf_counter()
            frame.run(features, execution="single")
            best = min(best, time.perf_counter() - started)
    finally:
        for name, fn in real.items():
            setattr(xmxres.Runtime, name, fn)
    print("  %-28s %7.0f ms" % (label, 1000 * best))
    return best

whole = timed("whole frame", ())
gemms = timed("GEMMs only", ("unary", "cosine_publish", "softmax"))
rest = timed("everything but the GEMMs", ("gemm",))
print("  %-28s %7.0f ms  (%.0f + %.0f = %.0f)" % ("", 0, 1000 * gemms, 1000 * rest,
                                                  1000 * (gemms + rest)))
