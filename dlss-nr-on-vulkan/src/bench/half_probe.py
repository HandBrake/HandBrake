#!/usr/bin/env python3
"""Three ways to round float32 to half on the device, against numpy's float16.

`float(float16_t(x))` is the obvious one and the compiler folds it away — that bug cost
this project a silent loss of every vendor rounding point. `packHalf2x16` is a real
hardware conversion; if it survives and rounds the same way, the bit-twiddled version
can go, and with it four branches per gate.
"""
import os, pathlib, sys, time
import numpy as np
ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src"))
import nr_build  # noqa: E402
os.environ["XMX_UNARY_SPV"] = str(nr_build.shader("half_probe.spv"))
sys.path.insert(0, str(ROOT / "src" / "gpu"))
import xmxres

rng = np.random.default_rng(3)
values = np.concatenate([
    rng.standard_normal(1 << 18).astype(np.float32) * 4.0,          # ordinary
    rng.standard_normal(1 << 16).astype(np.float32) * 1e-5,         # half subnormals
    rng.standard_normal(1 << 14).astype(np.float32) * 1e-7,         # far below
    (rng.standard_normal(1 << 14).astype(np.float32) * 1e5),        # overflow range
    np.float32([0.0, -0.0, 65504.0, 65520.0, 65519.0, 6.09e-05, 5.96e-08, 2.98e-08]),
]).astype(np.float32)
n = (values.size + 255) // 256 * 256
padded = np.zeros(n, np.float32); padded[:values.size] = values

rt = xmxres.Runtime()
src = rt.buffer_from(padded)
outs = [rt.buffer(n) for _ in range(3)]
rt.begin()
rt.unary(0, src, outs[1], n, second=outs[0], third=outs[2])
rt.submit()
want = padded.astype(np.float16).astype(np.float32)
for name, buf in zip(("bit-twiddled", "packHalf2x16", "float(float16_t(x))"), outs):
    got = buf.view()[:n]
    bad = ~((got == want) | (np.isnan(got) & np.isnan(want)))
    where = np.flatnonzero(bad)
    print("  %-22s mismatches %7d / %d%s" % (name, bad.sum(), n,
          "" if not bad.sum() else "   first: x=%.9g got %.9g want %.9g"
          % (padded[where[0]], got[where[0]], want[where[0]])))
