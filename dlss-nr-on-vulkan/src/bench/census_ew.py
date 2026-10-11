#!/usr/bin/env python3
"""Tally the elementwise / row passes one frame records, by kind."""
import collections, pathlib, sys
import numpy as np
ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "gpu")); sys.path.insert(0, str(ROOT / "src" / "ref"))
import xmxres, nr_model, nr_frame_resident as F

NAME = {0: "e4m3", 1: "gate", 2: "half", 3: "to_half", 4: "scale", 5: "residual",
        6: "from_half", 7: "partition", 8: "reverse", 9: "add_bias", 10: "split_heads",
        11: "merge_heads", 12: "pool2", 13: "upsample2", 14: "scale_channel", 15: "add",
        16: "pad_end", 17: "gate_e4m3_half", 18: "e4m3_half", 19: "gate_half"}
ELEM, CALLS = collections.Counter(), collections.Counter()
real_unary, real_cos, real_soft = xmxres.Runtime.unary, xmxres.Runtime.cosine_publish, xmxres.Runtime.softmax

def unary(self, kind, source, target, count, **kw):
    ELEM[NAME.get(kind, kind)] += count; CALLS[NAME.get(kind, kind)] += 1
    return real_unary(self, kind, source, target, count, **kw)

def cosine(self, source, target, rows, **kw):
    ELEM["cosine_publish"] += rows * 32; CALLS["cosine_publish"] += 1
    return real_cos(self, source, target, rows, **kw)

def softmax(self, source, target, rows, width, **kw):
    ELEM["softmax"] += rows * width; CALLS["softmax"] += 1
    return real_soft(self, source, target, rows, width, **kw)

xmxres.Runtime.unary, xmxres.Runtime.cosine_publish, xmxres.Runtime.softmax = unary, cosine, softmax

H, W = 768, 1280
model = nr_model.NeuralRenderingModel.from_safetensors(ROOT / "work" / "mlxw" / "dlssnr-logical.safetensors")
rt = xmxres.Runtime()
F.ResidentFrame(rt, model.weights, H, W).run(np.zeros((H, W, 16), dtype=np.float32))

print("  %-18s %8s %14s %12s" % ("pass", "calls", "elements", "M elem"))
for name, count in ELEM.most_common():
    print("  %-18s %8d %14d %12.1f" % (name, CALLS[name], count, count / 1e6))
print("  %-18s %8d %14d %12.1f" % ("TOTAL", sum(CALLS.values()), sum(ELEM.values()),
                                   sum(ELEM.values()) / 1e6))
