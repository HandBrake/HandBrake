#!/usr/bin/env python3
"""The GEMM's fused epilogue, against the same chain run as a separate pass.

The publish runs on the accumulator's own components and the matrix stores straight to
global. `notes/phase18-fusion.md` claimed the driver forbade touching a cooperative
matrix between `coopMatMulAdd` and `coopMatStore`, which is withdrawn — the only real
rule is that a matrix must be stored into an array of its own component type
(`notes/phase38-there-was-no-bug.md`). This checks the fused path is exact: the plain
store still matches numpy, and every epilogue matches the reference chain applied to
the GPU's own GEMM output.
"""
import pathlib, sys
import numpy as np
ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "gpu")); sys.path.insert(0, str(ROOT / "src" / "ref"))
import xmxres, nr_model

M, N, K = 128, 256, 128
rng = np.random.default_rng(7)
A = (rng.standard_normal((M, K)) * 0.3).astype(np.float16)
B = (rng.standard_normal((K, N)) * 0.3).astype(np.float16)
want = (A.astype(np.float32) @ B.astype(np.float32))

rt = xmxres.Runtime()
a = rt.buffer_from(A, np.float16); b = rt.buffer_from(B, np.float16)

def run(epilogue, narrow):
    c = rt.buffer(M * N, np.float16 if narrow else np.float32)
    rt.begin(); rt.gemm(a, b, c, M, N, K, epilogue=epilogue, narrow=narrow); rt.submit()
    out = xmxres.host_view(c, np.float16 if narrow else np.float32,
                           count=M * N).reshape(M, N).astype(np.float32)
    c.free()
    return out

def report(name, got, expect):
    delta = np.abs(got - expect)
    limit = 3e-3 * max(1.0, float(np.abs(expect).max())) if "control" in name else 0.0
    ok = "OK  " if delta.max() <= limit else "FAIL"
    print("  %-28s max|d| %-12.3e  mean|d| %-12.3e  %s"
          % (name, delta.max(), delta.mean(), ok))
    return ok == "OK  "

half, gate, e4m3 = nr_model._half_rounded, nr_model.quadratic_gate_activation, nr_model.e4m3

# The epilogue is judged against the reference chain applied to *this GPU's own* GEMM
# output, not to numpy's: the two GEMMs differ by 6e-05 and E4M3's 6.25 % quantum turns
# that into a whole step, which is the graph's chaos and not the epilogue's error.
base = run(0, False)
good = report("plain store vs numpy (control)", base, want)
good &= report("half out", run(4, True), half(base))
good &= report("e4m3", run(1, False), e4m3(base))
good &= report("e4m3, half out", run(1, True), e4m3(base))
good &= report("gate", run(2, False), gate(base))
good &= report("gate + e4m3", run(3, False), e4m3(gate(base)))
good &= report("gate + e4m3, half out", run(3, True), e4m3(gate(base)))
good &= report("half round", run(4, False), half(base))
print("\n  %s" % ("all epilogues correct — fused into the accumulator, stored straight out"
                  if good else "something is still wrong"))
sys.exit(0 if good else 1)
