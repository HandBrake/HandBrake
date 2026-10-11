# Phase 26 — all 64 XMX engines are busy, and each is 90 % idle

**Update 2026-09-10.** Every spill count below was measured on the **universal**
shaders, which carry code for every flag combination at once. Pipeline specialization
compiles one variant per flag word and lets the driver drop the rest, and re-measuring
through it changes two of the three things this note says:

| block | universal (below) | specialized |
|---|---|---|
| 16x32 | 15:15 | **0:0** |
| 32x32 | 97:141 | 31:59 |
| 32x64 | 474:435 | 153:182 |

- **Withdrawn: "not a better Vulkan kernel ... the one avenue left is OpenCL."** That
  was wrong. Specialization is a Vulkan-side change worth a *paired, same-buffer*
  **11 %** at 720p on this machine (556.5 -> 494.9 ms, bit-identical), and this note
  missed it. `phase27-pipeline-specialization.md`.
- **Corrected: the spill counts are about 3x too high**, and the block the graph
  actually uses does not spill at all once specialized.
- **Still standing: the mechanism, and the ranking.** Specialization cuts spilling by
  a constant factor; it does not change that an accumulator costs 4 of 128 registers.
  Re-measured with specialization on, 16x32 still wins by a wide margin — **2247**
  GFLOP/s against 1281 for 32x32 and 710 for 32x64 — because the larger blocks still
  spill. The register file is still the ceiling; it is just a slightly higher one than
  this note measured.

2026-09-10. Asked whether the port is using all the XMX cores, on the understanding
that there are 128 of them and 67 TOPS to be had. Both halves of that are worth
correcting, and the answer underneath is the most useful thing measured in this project
so far.

## The hardware, as the machine reports it

```
clinfo:  Device Name  Intel(R) Arc(TM) Graphics
         Max compute units          64
         Max clock frequency        1950MHz
```

**64, not 128.** Arc 140V is 8 Xe2 cores of 8 vector engines, one XMX per vector
engine. The arithmetic that follows is consistent with the marketing figure rather than
against it: 64 XMX x 128 MAC x 2 = 16 384 FLOP/clock, at 1.95 GHz = **31.9 TFLOP/s
FP16**, and INT8 at twice the rate is **~64-67 TOPS** — which is the 67 TOPS number,
for INT8, for the GPU alone.

The memory arithmetic in the question is exactly right: 128-bit bus at 8533 MT/s is
16 B x 8.533 GHz = **136.5 GB/s** theoretical, which is the figure this project has
been carrying. Measured, the GPU reaches **70-91 GB/s** of it — 51-67 %, which is
ordinary for LPDDR5X behind an iGPU.

## Yes, every engine is busy

`src/bench/occupancy.py` holds the work per workgroup fixed and grows the grid. One
workgroup is one subgroup of 32 computing a 16x32 block:

| workgroups | GFLOP/s | of FP16 peak |
|---|---|---|
| 1 | 13.9 | 0.0 % |
| 8 | 162 | 0.5 % |
| 64 | 1282 | 4.0 % |
| 128 | 2387 | 7.5 % |
| **256** | **3214** | **10.0 %** |
| 512+ | ~2000 | 6.3 % (A no longer fits in cache) |

Throughput scales **linearly** from one workgroup to 256 and flattens there. 256
subgroups over 64 XMX engines is four per engine — the machine is full, and the graph's
real dispatches are in the thousands of workgroups, well past it. So nothing is sitting
idle for want of work.

A single subgroup runs at 13.9 GFLOP/s, 2.8 % of one engine's peak: 64 K-steps of four
dependent loads and four multiply-accumulates with no other subgroup to hide the
latency. That is the whole story in miniature — the units are fast and the feed is not.

## And each engine is ~90 % idle, because of the register file

The obvious fix is a bigger register block: more multiply-accumulates per operand load.
`src/bench/block_peak.py` measures each shape at the grid that suits it, and Mesa's own
compiler says why the answer goes the wrong way.

| block | MACs per load | spills:fills | peak GFLOP/s | of peak |
|---|---|---|---|---|
| 16x32 | 1.00 | **15:15** | **2715** | **8.5 %** |
| 32x32 | 1.33 | 97:141 | 1218 | 3.8 % |
| 16x64 | 1.33 | 218:209 | 761 | 2.4 % |
| 64x32 | 1.60 | 412:402 | 724 | 2.3 % |
| 32x64 | 2.00 | 474:435 | 555 | 1.7 % |

`MESA_SHADER_CACHE_DISABLE=true INTEL_DEBUG=cs` prints the spill counts. They are
monotonic with the loss, and the mechanism is arithmetic:

- The compiler runs these at **SIMD32 with 128 GRF registers**. In SIMD32 one register
  holds one dword per lane.
- One accumulator is 8x16 float32 = 128 floats over 32 lanes = **4 registers**. An
  A fragment is 2, a B fragment is 4.
- 16x32 needs 4 accumulators + 2 A + 2 B = **28 registers** of matrix state, and still
  spills 15 — the epilogue and the loop take the rest.
- 32x64 needs 16 accumulators + 4 A + 4 B = **88 registers** of matrix state alone, and
  spills 474.

**The register file is the ceiling.** A SIMD32 subgroup cannot hold enough accumulators
to raise the arithmetic intensity, and the 8x16x16 cooperative-matrix shape fixes how
much each one buys. There is no route past it from here:

- **SIMD16 would double the per-lane register space** — but every cooperative-matrix
  configuration on this device requires `subgroupSize = 32` (`notes/hw-coopmat.md`), so
  the matrices go away with it.
- **A 256-register mode** exists on Intel hardware; this Mesa's `INTEL_DEBUG` exposes no
  way to ask for it, and it reports `GRF registers: 128` for every shader we build.

## Which explains everything else measured this week

Three separate negative results turn out to be one result:

- **Register tiling beyond 16x32 loses** (phase 21) — spills.
- **Shared-memory operand staging does not pay** (phase 22) — the bottleneck was never
  operand latency from global memory; it is that a subgroup cannot hold enough
  accumulators for staged operands to be reused.
- **The frame sits at 4.4 % of arithmetic peak and 50 GB/s of a 90 GB/s ceiling**
  (phase 22) — latency-bound in both halves, and this is the reason for the GEMM half.

The best throughput seen anywhere in this project is **3.9 TFLOP/s** on a deep, wide
shape with staged operands — **12 % of peak**. Call the usable range 8-12 %. The 32
TFLOP/s figure assumes the units are fed back to back, which needs more registers than
a SIMD32 subgroup has.

## Not a thermal or a power limit

Worth settling separately, because "the hardware's limit" usually means heat. It is
not that. Sampled every three seconds through a minute of continuous frames:

```
1950 1950 1950 1950 1950 1950 1950 1950 1950 1950
1950 1950 1950 1950 1950 1950 1950 1950 1950 1950  MHz
package 55 C (limit 100), RAPL 35 W long / 37 W short, max_freq 1950
```

Twenty samples, no dip. The part's own ceiling is 1950 and its efficient point is 700,
so the graph runs pinned at the top clock, cool, and inside a 35 W envelope shared with
the CPU. There is no headroom being lost to throttling and none to be recovered by
cooling: the fan profile question from phase 20 is now answered — leaving it on silent
costs nothing.

The limit is architectural. The units are all busy, at full clock, and mostly idle
inside each cycle.

## What would actually change it

Not a better Vulkan kernel. The one avenue left is **OpenCL**, which exposes what Vulkan
does not on this part: `cl_intel_subgroup_matrix_multiply_accumulate` with shapes Vulkan
has no configuration for, and — visible in `clinfo` on this machine —
**`cl_intel_subgroup_2d_block_io`** and `cl_intel_subgroup_buffer_prefetch`, the hardware
2D block loads that Intel's own libraries use to feed XMX without spending registers on
addressing. That is a second backend, not a flag, and notes/CLAUDE.md has said so since the
start. It is now the only lever with a plausible factor in it.
