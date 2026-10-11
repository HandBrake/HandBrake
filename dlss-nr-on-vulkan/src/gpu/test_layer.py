#!/usr/bin/env python3
"""
test_layer — a real weight matrix on the XMX units, against the CPU.

**Superseded, and not in `make test`.** Two reasons, both worth knowing before running
it: it loads `work/weights_ht.bin`, the dense-FP16 decode that `notes/phase6` replaced —
so it cannot run on a clone that has not carved that file out of the DLL — and the
weights it reads are the wrong decode, kept only for the findings the surrounding files
encode. `notes/reviewing.md` says which tests are the live ones.

**It does not pass, and cannot.** Worst relative error 0.22. The dense-FP16 decode
produces values including FP16 subnormals, Mesa's default flushed subnormal operands to
zero (phase71: a driver's mode, not the XMX units') and the float64 reference does not,
so the two disagree by the size of that gap.
The premise it was written under — `notes/phase4-subnormal-flush.md`, '27 % of this
model's parameters are subnormal' — was itself an artefact of the same wrong decode;
the real figure is 0.00006 %. Kept as the record of a measurement that was real at
the time and is not reproducible now.
"""
import subprocess
import sys
import tempfile
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "ref"))
sys.path.insert(0, str(ROOT / "src"))
import nr_build  # noqa: E402
from hnet_model import Model  # noqa: E402

RUNNER, SPV = nr_build.executable("gemm_runner"), nr_build.shader("gemm_coopmat.spv")


def gpu_gemm(A, B):
    M, K = A.shape
    K2, N = B.shape
    assert K == K2 and M % 8 == 0 and N % 16 == 0 and K % 16 == 0
    with tempfile.TemporaryDirectory(prefix="nr-layer-") as temporary:
        d = Path(temporary)
        (d / "a").write_bytes(np.ascontiguousarray(A, dtype="<f2").tobytes())
        (d / "b").write_bytes(np.ascontiguousarray(B, dtype="<f2").tobytes())
        r = subprocess.run([str(RUNNER), str(SPV), str(M), str(N), str(K),
                            str(d / "a"), str(d / "b"), str(d / "c")],
                           capture_output=True, text=True)
        if r.returncode:
            print(r.stderr); raise SystemExit("runner failed")
        return np.frombuffer((d / "c").read_bytes(), dtype="<f4").reshape(M, N)


def main():
    m = Model(str(ROOT / "work" / "weights_ht.bin"))
    idx = {b.index: b for b in m.blocks}
    rng = np.random.default_rng(7)
    print("=== real DLSS-NR weights on Intel Xe2 XMX ===\n")
    print("  %-34s %-13s %-12s %-12s %s" % ("tensor", "shape", "max|gpu-cpu|", "relative", "verdict"))
    worst = 0.0
    cases = [
        (23, "wq",           "block23.wq   (split-Swin qkv, Q)"),
        (23, "proj",         "block23.proj (output projection)"),
        (23, "ffwd",         "block23.ffwd"),
        (31, "wq",           "block31.wq   (ViT-1D, C=1024)"),
        (31, "ffn_expand",   "block31.ffn_expand 1024->2048"),
        (31, "ffn_contract", "block31.ffn_contract 2048->1024"),
        (39, "weight",       "block39.weight (dec upsample)"),
    ]
    for bi, tname, label in cases:
        W = idx[bi].t[tname]
        M = 64
        x = (rng.standard_normal((M, W.shape[0])) * 0.5).astype(np.float16)
        g = gpu_gemm(x, W)
        c = x.astype(np.float64) @ W.astype(np.float64)
        dev = np.abs(g.astype(np.float64) - c).max()
        rel = dev / max(np.abs(c).max(), 1e-30)
        worst = max(worst, rel)
        print("  %-34s %-13s %-12.4g %-12.4g %s"
              % (label, "%dx%d" % W.shape, dev, rel, "OK" if rel < 1e-4 else "FAIL"))
    print("\n  worst relative error: %.4g" % worst)
    print("  %s" % ("PASS - the XMX path reproduces the CPU reference on real weights"
                    if worst < 1e-4 else "FAIL"))
    return 0 if worst < 1e-4 else 1


raise SystemExit(main())
