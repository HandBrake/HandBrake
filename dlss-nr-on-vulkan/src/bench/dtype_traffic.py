#!/usr/bin/env python3
"""How much of a frame's traffic is still float32, and in which passes.

A buffer's dtype is recoverable from its size against the element count the pass is
given, so this needs no bookkeeping in the graph itself.
"""
import collections, pathlib, sys
import numpy as np
ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "gpu")); sys.path.insert(0, str(ROOT / "src" / "ref"))
import xmxres, nr_model, nr_frame_resident as F

NAME = {0: "e4m3", 1: "gate", 2: "half", 3: "to_half", 4: "scale", 5: "residual",
        6: "from_half", 7: "partition", 8: "reverse", 9: "add_bias", 10: "split_heads",
        11: "merge_heads", 12: "pool2", 13: "upsample2", 14: "scale_channel", 15: "add",
        16: "pad_end", 17: "gate_e4m3_half", 18: "e4m3_half", 19: "gate_half"}
WIDE, NARROW = collections.Counter(), collections.Counter()
real = {n: getattr(xmxres.Runtime, n) for n in ("unary", "gemm", "cosine_publish", "softmax")}

def width(buffer, count):
    """4 if the buffer holds float32 at this element count, else 2."""
    return 4 if buffer.nbytes >= count * 4 else 2

def note(label, buffer, count, reading):
    w = width(buffer, count)
    (WIDE if w == 4 else NARROW)[label + (" read" if reading else " write")] += count * w

def unary(self, kind, source, target, count, **kw):
    label = NAME.get(kind & 0xFF, kind)
    note(label, source, count, True); note(label, target, count, False)
    return real["unary"](self, kind, source, target, count, **kw)

def gemm(self, a, b, c, rows, cols, inner, *, batch=1, **kw):
    note("gemm A", a, rows * inner * batch, True)
    note("gemm C", c, rows * cols * batch, False)
    return real["gemm"](self, a, b, c, rows, cols, inner, batch=batch, **kw)

def cosine(self, source, target, rows, **kw):
    note("cosine_publish", source, rows * 32, True); note("cosine_publish", target, rows * 32, False)
    return real["cosine_publish"](self, source, target, rows, **kw)

def softmax(self, source, target, rows, width_, **kw):
    note("softmax", source, rows * width_, True); note("softmax", target, rows * width_, False)
    return real["softmax"](self, source, target, rows, width_, **kw)

for name, fn in (("unary", unary), ("gemm", gemm), ("cosine_publish", cosine), ("softmax", softmax)):
    setattr(xmxres.Runtime, name, fn)

model = nr_model.NeuralRenderingModel.from_safetensors(ROOT / "work" / "mlxw" / "dlssnr-logical.safetensors")
rt = xmxres.Runtime()
F.ResidentFrame(rt, model.weights, 768, 1280).run(np.zeros((768, 1280, 16), np.float32))

wide, narrow = sum(WIDE.values()), sum(NARROW.values())
print("  720p frame, activation traffic by storage width\n")
print("  %-26s %10s" % ("float32 (4 bytes)", "GB"))
for label, total in WIDE.most_common(12):
    print("  %-26s %10.2f" % (label, total / 1e9))
print("  %-26s %10.2f  (%.0f%%)\n" % ("-- total float32", wide / 1e9, 100 * wide / (wide + narrow)))
print("  %-26s %10.2f  (%.0f%%)" % ("float16 (2 bytes)", narrow / 1e9, 100 * narrow / (wide + narrow)))
print("  %-26s %10.2f" % ("everything", (wide + narrow) / 1e9))
print("\n  halving every float32 buffer would save %.2f GB of %.2f — %.0f%% of the traffic"
      % (wide / 2e9, (wide + narrow) / 1e9, 100 * (wide / 2) / (wide + narrow)))
