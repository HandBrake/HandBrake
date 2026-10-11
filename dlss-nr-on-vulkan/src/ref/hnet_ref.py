#!/usr/bin/env python3
"""
hnet_ref — the beginning of the Phase 3 CPU reference.

Two jobs:
  1. hand back HNet tensors by name as numpy arrays, in the shapes recovered in
     notes/ptx-kernel-configs.md;
  2. provide the numeric contract Phase 4 must reproduce on the GPU —
     FP16 operands, FP32 accumulation, matching the `fp16 x fp16 -> fp32`
     cooperative matrix config (notes/hw-coopmat.md, config 1).

Nothing here is tuned for speed. It is the ground truth.
"""
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from hnet_weights import parse


class HNet:
    def __init__(self, blob_path):
        self.blob = Path(blob_path).read_bytes()
        self.index = {t["name"]: t for t in parse(self.blob)}

    def raw(self, name):
        """One tensor, exactly as stored: flat float16."""
        t = self.index[name]
        return np.frombuffer(self.blob, dtype="<f2", count=t["n_elem"], offset=t["offset"])

    def matrix(self, name, shape, offset=0):
        """A slice of a packed blob, viewed as a 2-D float16 matrix."""
        n = int(np.prod(shape))
        return self.raw(name)[offset:offset + n].reshape(shape)


def linear_fp16_fp32(x, w):
    """
    y = x @ w with FP16 operands and FP32 accumulation.

    This is deliberately the exact shape of what XMX will do: inputs stay FP16,
    the accumulator is FP32. Computing it as float32 matmul on float16 inputs
    reproduces that; it is *not* the same as computing in float16 throughout.
    """
    assert x.dtype == np.float16 and w.dtype == np.float16
    return (x.astype(np.float32) @ w.astype(np.float32))


def _selftest():
    ref = HNet("work/weights_ht.bin")
    name = "block23.layer0.layer"

    print("== tensor ==")
    a = ref.raw(name)
    print("   %s: %s elements, dtype=%s" % (name, format(a.size, ","), a.dtype))

    print("\n== is the 512x512 reshape real structure, or an artefact? ==")
    M = ref.matrix(name, (512, 512))
    rows = np.linalg.norm(M.astype(np.float32), axis=1)
    cols = np.linalg.norm(M.astype(np.float32), axis=0)
    rng = np.random.default_rng(0)
    S = rng.permutation(M.astype(np.float32).ravel()).reshape(512, 512)
    srow = np.linalg.norm(S, axis=1)
    scol = np.linalg.norm(S, axis=0)
    cv = lambda v: v.std() / v.mean()
    print("   as stored   : row-norm cv = %.4f   col-norm cv = %.4f" % (cv(rows), cv(cols)))
    print("   shuffled    : row-norm cv = %.4f   col-norm cv = %.4f" % (cv(srow), cv(scol)))
    print("   -> stored rows are %.0fx more variable than chance; the 2-D layout is real."
          % (cv(rows) / cv(srow)))

    print("\n== numeric contract: fp16 operands, fp32 accumulate ==")
    rng = np.random.default_rng(1)
    x = rng.standard_normal((64, 512)).astype(np.float16)
    y32 = linear_fp16_fp32(x, M)
    y64 = x.astype(np.float64) @ M.astype(np.float64)
    y16 = (x @ M).astype(np.float64)          # fp16 accumulation, for contrast
    err32 = np.abs(y32.astype(np.float64) - y64).max()
    err16 = np.abs(y16 - y64).max()
    scale = np.abs(y64).max()
    print("   output range          : +-%.4g" % scale)
    print("   fp32 accumulate error : %.3e  (%.2e relative)" % (err32, err32 / scale))
    print("   fp16 accumulate error : %.3e  (%.2e relative)" % (err16, err16 / scale))
    print("   -> fp32 accumulation is %.0fx more accurate. Phase 4 must use config 1."
          % (err16 / err32))
    return 0


if __name__ == "__main__":
    raise SystemExit(_selftest())
