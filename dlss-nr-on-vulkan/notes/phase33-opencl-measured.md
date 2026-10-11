# Phase 33 — the last lever, measured: OpenCL does not beat Vulkan

2026-09-10. Phase 26 ended by naming OpenCL "the only lever with a plausible factor in
it": it exposes `cl_intel_subgroup_matrix_multiply_accumulate` and a **subgroup size of
16**, where every cooperative-matrix configuration on this device demands 32, and a
SIMD16 thread has twice the per-lane register space that phase 26 identified as the
ceiling. A parallel run left a DPAS kernel — `src/bench/opencl_gemm.cl` — with no
harness and no measurement. This is the measurement.

## It works, and it is correct

`src/bench/opencl_gemm.py` drives it through ctypes, since the machine has
`libOpenCL.so` and no CL headers. `intel_sub_group_f16_f16_matrix_mad_k16` with FP32
accumulators reproduces numpy to **2.08e-06** relative at every block size — the same
accuracy as our Vulkan config-1 path, as it should be, since both accumulate in FP32.

## It is slower

At 8192x512x1024, the same shape on both APIs:

| register block | OpenCL DPAS, subgroup 16 | Vulkan coopmat, subgroup 32 |
|---|---|---|
| 8x16 | 1783 GFLOP/s | |
| 16x16 | 2708 | |
| **16x32** | **3533 (11.0 %)** | **3828 (12.0 %)** |
| 32x32 | 1843 | |
| 32x64 | 1019 | |

**Vulkan is 8 % ahead.** And the shape of the curve is the giveaway: OpenCL peaks at the
**same 16x32 block** and collapses beyond it in the **same way** — 32x64 falls to a
third of the peak in both APIs.

Two APIs, two subgroup widths, two compilers, one curve. That is a hardware limit
expressing itself, not an artefact of the Vulkan path. Phase 26's mechanism survives;
phase 26's hope does not.

**Withdrawn: "the only lever with a plausible factor in it is OpenCL."** There is no
factor there. Measured, not assumed.

## What is left untried, stated precisely

This kernel loads its operands with ordinary scalar reads. The extension that was the
actual reason to look at OpenCL — **`cl_intel_subgroup_2d_block_io`**, the hardware 2D
block load Intel's own libraries use to feed XMX without spending registers on
addressing — is **present on this machine**: a probe naming a wrong variant gets
`did you mean 'intel_subgroup_block_read_u16_m8k16v2'?` back from the compiler, so the
family resolves.

It is untested. But it now has to clear a higher bar than phase 26 imagined: it would
have to buy **more than 8 % merely to reach today's Vulkan number**, and far more than
that to be the factor the project was hoping for. Given that both APIs already collapse
at the same block size, the loads are unlikely to be what is holding the ceiling down.

Worth an afternoon if someone wants certainty. Not worth planning around.

## Reproducing

```sh
python3 src/bench/opencl_gemm.py                       # the sweep above
python3 src/bench/opencl_gemm.py --shape 4096,1024,1024 --blocks 2x2,4x2
```

The ctypes trap is worth one line: **declare `argtypes` and `restype` for every entry
point.** Without them ctypes truncates 64-bit handles to `int`, and the first call into
the driver takes the process down with a core dump and no diagnostic.
