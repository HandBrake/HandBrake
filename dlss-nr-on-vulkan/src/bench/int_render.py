#!/usr/bin/env python3
"""Render a real frame with integer-quantised GEMM weights, through the normal CLI.

Usage: int_render.py BITS IN OUT   (BITS 0 leaves the weights alone)
"""
import pathlib, runpy, sys
import numpy as np
ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "gpu")); sys.path.insert(0, str(ROOT / "src" / "ref"))
import xmxres
from int_weights import quantise  # noqa: E402  (import order: xmxres must resolve first)

BITS = int(sys.argv[1])
real = xmxres.Runtime.buffer_from


def hooked(self, array, dtype=np.float32, pad=0):
    if BITS and dtype == np.float16 and np.ndim(array) == 2:
        array = quantise(np.asarray(array, np.float32), BITS, True)
    return real(self, array, dtype, pad)


xmxres.Runtime.buffer_from = hooked
sys.argv = ["nr_frame.py", sys.argv[2], sys.argv[3], "--resident"]
runpy.run_path(str(ROOT / "src" / "ref" / "nr_frame.py"), run_name="__main__")
