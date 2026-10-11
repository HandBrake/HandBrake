#!/usr/bin/env python3
"""One GEMM shape with each of the graph's epilogues in turn, and the K loop on its own.

On Intel's Windows compiler the staged GEMM is 1.2-1.5x Mesa's time a frame, and the worst call
sites are short K loops under a heavy epilogue: the gate activation, the residual and the QKV
epilogue at K = 64 run at ~2.1x, the same epilogues at K = 256-512 at 1.3-1.5x, and the narrow
N = 32 contracts at Mesa's speed (HANDOFF, 2026-10-02). A frame cannot say which part of a call
is slow. This times one shape at a time, each pass on the device's own timestamps:

- `f32`: no epilogue, a float32 output, stored straight from the accumulators;
- `half`: no epilogue, a half output, through the shared-memory stage like every publish;
- `e4m3`, `gate`: the E4M3 publish, and the gate activation then the publish (`0x1300`);
- `resid`: the residual with a half skip, then the publish, as the blocks run it (`0x41100`).

Then a K sweep at a fixed output, `f32` and `gate`, so the K loop's cost per step and what is
left at K -> 0 show apart. The same command on both drivers, set side by side, says which part
the compiler loses time in.

`--kernel` puts every call on one of libxmx's three GEMM kernels, through the thresholds that
choose between them: `staged`, the graph's own for K >= 32 (`gemm_staged.spv`); `tiled`, the
16x32 register block with no shared-memory stage (`gemm_tiled.spv`); `resident`, the plain 8x16
kernel, `gemm_coopmat_batched` addressed by pointer (`gemm_resident.spv`). The last column
names the kernel each row's calls ran on, from the device's own profile.

    python3 src/bench/gemm_epilogues.py
    python3 src/bench/gemm_epilogues.py --shapes 64512x128x64 320x1024x4096 --calls 20
    python3 src/bench/gemm_epilogues.py --kernel resident

Each number is the least of `--rounds` medians, the variants taken in turn within a round, after
`--warm` seconds of GEMMs: a laptop's GPU clock climbs and dips, and one round reads it too.
"""
import argparse
import os
import pathlib
import statistics
import sys
import time

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "gpu"))
import xmxres  # noqa: E402

# the 1344x768 frame's worst call sites against Mesa, and one narrow contract that is not
SHAPES = ("64512x128x64", "64512x64x64", "16128x128x128", "4032x128x256", "4032x256x256",
          "320x4096x1024", "320x1024x4096", "64512x32x128")
SWEEP = (16128, 128, (32, 64, 128, 256, 512, 1024))
# libxmx reads these when it builds its pipelines: K below XMX_STAGE_K leaves the staged
# kernel, K below XMX_TILE_K the tiled one, and what is left is the plain 8x16 kernel
KERNELS = {"staged": {}, "tiled": {"XMX_STAGE_K": "1000000"},
           "resident": {"XMX_STAGE_K": "1000000", "XMX_TILE_K": "1000000"}}
FAMILIES = {0: "resident", 1: "tiled", 2: "staged"}


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--shapes", nargs="*", default=list(SHAPES), metavar="MxNxK")
    parser.add_argument("--calls", type=int, default=12,
                        help="calls a variant, in one submit; the first is left out")
    parser.add_argument("--rounds", type=int, default=3,
                        help="rounds over every variant; the least median of each is kept")
    parser.add_argument("--warm", type=float, default=2.0, metavar="SECONDS",
                        help="GEMMs run first, so the clock has climbed before anything is timed")
    parser.add_argument("--no-sweep", action="store_true")
    parser.add_argument("--kernel", choices=KERNELS, default="staged",
                        help="the GEMM kernel every call runs on")
    args = parser.parse_args()

    os.environ.update(KERNELS[args.kernel])      # before the runtime builds its pipelines
    rt = xmxres.Runtime()
    xmxres.profile(True)
    rng = np.random.default_rng(3)
    ran = set()                                  # the kernels the timed calls ran on
    print(f"  {rt.lib.xmx_memory().decode()}")
    print(f"  kernel: {args.kernel}")

    def buffer(values, dtype):
        buf = rt.buffer(values.size, dtype)
        xmxres.host_write(buf, values.astype(dtype))
        return buf

    def timed(record):
        xmxres.profile_reset()
        rt.begin()
        for _ in range(args.calls):
            record()
        rt.submit()
        ran.update(FAMILIES.get(kind // 32, str(kind)) for kind in xmxres.profile_each_kinds())
        return statistics.median(xmxres.profile_each()[1:]) * 1000

    def kernels():
        names = ",".join(sorted(ran))
        ran.clear()
        return names

    def least(records):
        """The least median of each variant over the rounds, the variants in turn."""
        for record in records:                       # pipelines built outside the timing
            rt.begin()
            record()
            rt.submit()
        best = [float("inf")] * len(records)
        for _ in range(args.rounds):
            for i, record in enumerate(records):
                best[i] = min(best[i], timed(record))
        return best

    def operands(m, n, k):
        return (buffer(rng.standard_normal(m * k) * 0.3, np.float16),
                buffer(rng.standard_normal(k * n) * 0.05, np.float16),
                rt.buffer(m * n, np.float32), rt.buffer(m * n, np.float16),
                buffer(rng.standard_normal(m * n) * 0.5, np.float16),
                buffer(rng.uniform(0.5, 1.0, n), np.float32))

    # the clock climbs through the first busy second or so
    a, b, c32, _, _, _ = held = operands(16128, 128, 256)
    until = time.perf_counter() + args.warm
    while time.perf_counter() < until:
        rt.begin()
        for _ in range(16):
            rt.gemm(a, b, c32, 16128, 128, 256)
        rt.submit()
    for buf in held:
        buf.free()

    ran.clear()
    print(f"\n  {'M x N x K':16} {'f32':>8} {'half':>8} {'e4m3':>8} {'gate':>8} {'resid':>8}"
          f"   us a call, and the kernel")
    for shape in args.shapes:
        m, n, k = map(int, shape.lower().split("x"))
        a, b, c32, c16, skip16, cos = held = operands(m, n, k)
        row = least([lambda: rt.gemm(a, b, c32, m, n, k),
                     lambda: rt.gemm(a, b, c16, m, n, k, narrow=True),
                     lambda: rt.gemm(a, b, c16, m, n, k, epilogue=xmxres.EPI_E4M3, narrow=True),
                     lambda: rt.gemm(a, b, c16, m, n, k, epilogue=xmxres.EPI_GATE_E4M3,
                                     narrow=True),
                     lambda: rt.gemm_residual(a, b, skip16, cos, c16, m, n, k,
                                              epilogue=xmxres.EPI_E4M3, narrow=True,
                                              skip_half=True)])
        print(f"  {shape:16} " + " ".join(f"{t:8.1f}" for t in row) + f"   {kernels()}",
              flush=True)
        for buf in held:
            buf.free()

    if args.no_sweep:
        return
    m, n, ks = SWEEP
    print(f"\n  K sweep at {m}x{n}: us a call, and what each step of 32 adds")
    print(f"  {'K':>6} {'f32':>8} {'gate':>8}")
    previous = None
    for k in ks:
        a, b, c32, c16, skip16, cos = held = operands(m, n, k)
        f32, gate = least([lambda: rt.gemm(a, b, c32, m, n, k),
                           lambda: rt.gemm(a, b, c16, m, n, k, epilogue=xmxres.EPI_GATE_E4M3,
                                           narrow=True)])
        step = ""
        if previous:
            steps = (k - previous[0]) / 32
            step = (f"   +{(f32 - previous[1]) / steps:.1f} / +{(gate - previous[2]) / steps:.1f}"
                    f" us a step of 32")
        print(f"  {k:6d} {f32:8.1f} {gate:8.1f}{step}   {kernels()}", flush=True)
        previous = (k, f32, gate)
        for buf in held:
            buf.free()


if __name__ == "__main__":
    main()
