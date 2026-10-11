#!/usr/bin/env python3
"""Would bfloat16 storage be lossless for this graph's activations? float16 is.

The graph rounds to half at every vendor rounding point — the quadratic gate, the
softmax's affine map, the cosine tree — so its activations *are* float16 values by
construction. bfloat16 is the same two bytes but keeps 7 explicit mantissa bits
against float16's 10, so it cannot hold them.
"""
import pathlib, sys
import numpy as np
ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "gpu")); sys.path.insert(0, str(ROOT / "src" / "ref"))
import xmxres, nr_model, nr_frame_resident as F

def to_bf16(x):
    """Round float32 to bfloat16 and back, round-half-even at bit 16."""
    bits = x.astype(np.float32).view(np.uint32)
    bits = (bits + 0x7FFF + ((bits >> 16) & 1)) & 0xFFFF0000
    return bits.view(np.float32)

model = nr_model.NeuralRenderingModel.from_safetensors(ROOT / "work" / "mlxw" / "dlssnr-logical.safetensors")
rt = xmxres.Runtime()
frame = F.ResidentFrame(rt, model.weights, 384, 384)
rng = np.random.default_rng(5)
capture = {}
head = frame.run((rng.standard_normal((384, 384, 16)) * 0.3).astype(np.float32), capture=capture)

print("  %-14s %10s %14s %14s" % ("tensor", "values", "exact in fp16", "exact in bf16"))
for name, data in list(capture.items())[:8]:
    v = np.asarray(data, np.float32).reshape(-1)
    v = v[np.isfinite(v)]
    if not v.size:
        continue
    fp16 = float((v == v.astype(np.float16).astype(np.float32)).mean())
    bf16 = float((v == to_bf16(v)).mean())
    print("  %-14s %10d %13.2f%% %13.2f%%" % (name, v.size, 100 * fp16, 100 * bf16))

v = np.asarray(head, np.float32).reshape(-1)
print("  %-14s %10d %13.2f%% %13.2f%%" % ("head", v.size,
      100 * (v == v.astype(np.float16).astype(np.float32)).mean(), 100 * (v == to_bf16(v)).mean()))
print("\n  Both formats are two bytes, so bfloat16 saves no traffic that float16 does not.")
