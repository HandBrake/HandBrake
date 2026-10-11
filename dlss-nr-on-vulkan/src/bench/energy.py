#!/usr/bin/env python3
"""Joules per frame, from the package energy counter.

RAPL counts the whole SoC — CPU, GPU and memory controller share one package here —
which is the honest number for a shared-memory part: there is no separate board power
to quote. Idle is measured either side and subtracted, so what is left is the frame.
"""
import pathlib, sys, time
import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "gpu")); sys.path.insert(0, str(ROOT / "src" / "ref"))
import xmxres, nr_model, nr_frame_resident as F

RAPL = pathlib.Path("/sys/class/powercap/intel-rapl:0/energy_uj")
WRAP = int(pathlib.Path("/sys/class/powercap/intel-rapl:0/max_energy_range_uj").read_text())


def joules():
    return int(RAPL.read_text()) / 1e6


def measure(work, seconds_hint=1.0):
    start_e, start_t = joules(), time.perf_counter()
    work()
    end_e, end_t = joules(), time.perf_counter()
    energy = end_e - start_e
    if energy < 0:
        energy += WRAP / 1e6
    return energy, end_t - start_t


def idle_watts(seconds=3.0):
    energy, elapsed = measure(lambda: time.sleep(seconds))
    return energy / elapsed


H, W = (int(sys.argv[1]), int(sys.argv[2])) if len(sys.argv) > 2 else (768, 1280)
model = nr_model.NeuralRenderingModel.from_safetensors(ROOT / "work" / "mlxw" / "dlssnr-logical.safetensors")
rt = xmxres.Runtime()
frame = F.ResidentFrame(rt, model.weights, H, W)
features = (np.random.default_rng(11).standard_normal((H, W, 16)) * 0.3).astype(np.float32)
frame.run(features)

before = idle_watts()
count = 20
energy, elapsed = measure(lambda: [frame.run(features) for _ in range(count)])
after = idle_watts()
idle = (before + after) / 2

per_frame = elapsed / count
package = energy / elapsed
attributable = (energy - idle * elapsed) / count
print(f"  network extent {W}x{H}")
print(f"  idle              {idle:6.2f} W   (mean of before and after)")
print(f"  during the run    {package:6.2f} W   over {count} frames in {elapsed:.2f} s")
print(f"  frame             {1000 * per_frame:6.0f} ms")
print(f"  energy per frame  {energy / count:6.2f} J package, {attributable:5.2f} J above idle")
print(f"  arithmetic        {459.6 / 1e3 * (H * W) / (768 * 1280):.2f} TFLOP per frame")
print(f"  efficiency        {459.6e9 * (H * W) / (768 * 1280) / max(attributable, 1e-9) / 1e9:6.1f} GFLOP per joule")
