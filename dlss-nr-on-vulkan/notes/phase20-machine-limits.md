# What this machine can actually do, measured properly
2026-09-09

The owner pushed back on a number I had been quoting: LPDDR5X on package should do
136.5 GB/s, and 23 GB/s is far too slow to be the machine's limit. That was right, and
the mistake was mine.

## The bandwidth figure was single-threaded numpy

`notes/phase14-npu-and-rounding.md` reported 23.5 GB/s for a copy and 9.8 GB/s for a
read, and used them as if they were the machine's bandwidth. They were one core's
throughput through numpy. **One core cannot saturate an on-package memory controller.**

STREAM, eight threads, 192 MB arrays:

| | one thread | eight threads |
|---|---|---|
| copy | 31.1 GB/s | **51.9 GB/s** |
| scale | 30.6 | 60.3 |
| add | 32.1 | 67.0 |
| triad | 32.3 | **70.8** |

And the GPU, on 128 MB buffers through our own shaders:

| | GB/s |
|---|---|
| float32 copy | 69.4 |
| float32 -> float16 | 71.9 |
| E4M3 publish | **91.3** |
| the fused gate+publish+narrow | 82.9 |

So the machine delivers **70-91 GB/s** against 136.5 theoretical — 50-67 %, which is
what real workloads get. My figure was low by a factor of three, and the traffic
argument built on it was wrong in the same proportion.

## The GPU is not throttled, and the clock is not the problem

Sampled every 1.2 s through a twenty-second run of the resident graph:

```
act_freq = 1950 MHz, continuously, against max_freq = 1950
```

It sits at its ceiling for the whole run. Package limits are 35 W long-term and 37 W
short; temperatures during the run were 46-48 °C, with the CPU cores at 3.6-4.7 GHz.
Nothing is being held back.

## So the slack is in our kernel

A large GEMM, 4096x1024x4096: **2584 GFLOP/s, 8.1 %** of the ~32 TFLOP/s this GPU can
do in FP16 at 1950 MHz. The shader has no shared-memory staging, no K-blocking and no
register reuse — every workgroup re-reads both operands from memory for one 8x16 tile.

Against a 1025 ms 720p frame, the parts that are genuinely bounded add to roughly
300 ms: 177 ms of arithmetic at our current 8 %, and 110-145 ms of traffic. The other
**700 ms is dispatch overhead and occupancy** on the graph's many small shapes. That is
the encouraging reading — the room is in our code, not the chip.

## Should the cooling profile change?

**Not for this workload, not yet.** The GPU is already at its maximum clock for the
whole run and the package is at 46-48 °C, so there is nothing for a performance profile
to unlock: the limit being hit is our kernel's efficiency, not power or heat.

It becomes worth revisiting *after* the kernel work. At 8 % of peak the GPU draws very
little; at 30-40 % it would draw several times more, and the 35 W package limit could
start binding — especially with the CPU busy at the same time. So: leave it on silent
for now, and try performance mode again once a frame is a few hundred milliseconds
rather than a second. The measurement to repeat then is `act_freq` under load — if it
stops holding 1950, power has become the constraint.

(RAPL energy counters need root here, so package power under load could not be read
directly; the clock holding its ceiling is the evidence instead.)
