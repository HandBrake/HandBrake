#!/usr/bin/env python3
"""The quantisation contract, checked against the kernel rather than against itself.

`int8_quant` claims that a float GEMM can be handed to cooperative-matrix configuration 4
and come back within the accuracy `notes/improve-int8-bottleneck.md` measured. A module
that only checked its own round trip would prove nothing about that: the claim is about
the composition — quantise, dispatch, rescale — and the dispatch is on the GPU.

So every accuracy check here runs the real kernel. What is compared is the float GEMM the
graph would otherwise have done.
"""
import pathlib
import subprocess
import sys
import tempfile

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "gpu"))
import int8_quant as Q                                             # noqa: E402

sys.path.insert(0, str(ROOT / "src"))
import nr_build  # noqa: E402
RUNNER = nr_build.executable("gemm_runner")
SPV = nr_build.shader("gemm_coopmat_int8.spv")


def has_matrix_units():
    """Configuration 4 is a cooperative-matrix kernel: a device without
    VK_KHR_cooperative_matrix (MoltenVK, the phones) cannot build it at all."""
    import ctypes
    lib = ctypes.CDLL(str(nr_build.library("xmx")))
    if lib.xmx_open() != 0:
        return False
    return lib.xmx_coopmat() == 1
FAILURES = []


def check(name, ok, detail=""):
    print(f"  [{'ok  ' if ok else 'FAIL'}] {name}{'  ' + detail if detail else ''}", flush=True)
    if not ok:
        FAILURES.append(name)


def dispatch(packed_a, packed_b):
    m, k = packed_a.shape
    n = packed_b.shape[1]
    with tempfile.TemporaryDirectory() as room:
        room = pathlib.Path(room)
        (room / "a").write_bytes(packed_a.tobytes())
        (room / "b").write_bytes(packed_b.tobytes())
        done = subprocess.run([str(RUNNER), str(SPV), str(m), str(n), str(k),
                               str(room / "a"), str(room / "b"), str(room / "c")],
                              capture_output=True, text=True, timeout=600)
        if done.returncode != 0:
            raise RuntimeError((done.stderr.strip().splitlines() or ["no output"])[-1])
        return np.frombuffer((room / "c").read_bytes(), np.int32).reshape(m, n)


def through_kernel(a, b):
    packed_a, packed_b, a_scale, b_scale, rows, columns = Q.prepare(a, b)
    return Q.rescale(dispatch(packed_a, packed_b), a_scale, b_scale, rows, columns)


def relative(got, want):
    return float(np.abs(got - want).mean() / (np.abs(want).mean() + 1e-30))


def main():
    if nr_build.backend() != "vulkan" or not has_matrix_units():
        print("int8 quantisation: skipped (the integer kernel needs VK_KHR_cooperative_matrix, which this "
              "device or runtime does not have) — a skip is not a pass")
        return 0
    if not RUNNER.exists() or not SPV.exists():
        print("int8 quant: skipped (make first) — a skip is not a pass")
        return 0
    rng = np.random.default_rng(19)

    # 1. Storage. The scales are real bytes and are counted; the saving is not 2.00x.
    fake = {f"w{i}": rng.standard_normal((1024, 4096)).astype(np.float32) for i in range(2)}
    wide, packed = Q.storage_bytes(fake)
    check("int8 storage roughly quarters float32, scales included",
          3.9 < wide / packed < 4.0, f"{wide/1e6:.1f} MB -> {packed/1e6:.1f} MB "
          f"({wide/packed:.3f}x)")

    # 2. Padding is exact: zeros contribute nothing, and the crop removes them.
    a = rng.standard_normal((13, 50)).astype(np.float32)
    b = rng.standard_normal((50, 21)).astype(np.float32)
    pa, pb, rows, columns = Q.pad_for_tiles(a, b)
    check("padding reaches the tile shape and remembers the real size",
          pa.shape == (16, 64) and pb.shape == (64, 32) and (rows, columns) == (13, 21),
          f"A {a.shape}->{pa.shape}, B {b.shape}->{pb.shape}")
    check("padding leaves the original values untouched",
          bool(np.array_equal(pa[:13, :50], a) and np.array_equal(pb[:50, :21], b)))

    # 3. The composition, on the graph's own shapes. Accuracy is the point of the module.
    for m, n, k in ((64, 3072, 1024), (240, 1024, 4096), (13, 50, 21)):
        a = rng.standard_normal((m, k)).astype(np.float32)
        b = rng.standard_normal((k, n)).astype(np.float32)
        try:
            got = through_kernel(a, b)
        except Exception as error:                                  # noqa: BLE001
            check(f"{m}x{n}x{k} through the kernel", False, str(error)[:110])
            continue
        want = a @ b
        error = relative(got, want)
        # Two int8 grids over a K-long sum: the error floor is about 1/127 per operand,
        # and the sum averages it down. Anything near or above the single-operand step
        # would mean the scales are not being applied per row and column.
        check(f"{m}x{n}x{k} through the kernel", error < 0.02,
              f"relative {error:.5f} against the float GEMM")

    # 4. The claim that earns the module its shape: per-row beats one scale per tensor.
    #    phase23 measured the per-tensor case and called the activations unusable.
    m, n, k = 240, 1024, 4096
    a = rng.standard_normal((m, k)).astype(np.float32)
    a[::7] *= 60.0                       # a few tokens far brighter than the rest
    b = rng.standard_normal((k, n)).astype(np.float32)
    want = a @ b
    per_row = relative(through_kernel(a, b), want)
    peak = np.max(np.abs(a))
    flat = np.round(a / (peak / Q.LIMIT)).clip(-127, 127).astype(np.int8)
    packed_b, b_scale = Q.quantise(b, axis=0)
    per_tensor = relative(
        Q.rescale(dispatch(flat, packed_b),
                  np.full(m, peak / Q.LIMIT, np.float32), b_scale, m, n), want)
    check("a scale per row beats one per tensor on a frame with bright tokens",
          per_row < per_tensor / 2,
          f"per row {per_row:.5f} against per tensor {per_tensor:.5f} "
          f"({per_tensor/per_row:.1f}x better)")

    if FAILURES:
        print("FAILED: " + ", ".join(FAILURES))
        return 1
    print("int8 quant: quantise, dispatch and rescale agree with the float GEMM")
    return 0


if __name__ == "__main__":
    sys.exit(main())
