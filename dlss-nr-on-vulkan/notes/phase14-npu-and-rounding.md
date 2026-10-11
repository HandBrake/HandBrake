# Is the NPU worth using? No. Is anything else? Yes, and it was the rounding.
2026-09-09

The owner asked whether the ~48 TOPS NPU could take the network, and noted that the
whole thing sits inside one SoC so something ought to be optimisable. The second half
was right and the first was not, though not for the reason one would guess.

## The NPU is there

```
00:0b.0 Processing accelerators: Intel Core Ultra 200V Series Processors NPU [8086:643e]
intel_vpu driver loaded, /dev/accel/accel0 present and world-readable
no Level Zero loader, no OpenVINO — nothing can currently talk to it
```

## Four reasons it is not the answer

**1. It is not the fastest engine in this chip.** Lunar Lake's published split is NPU
48 TOPS, **GPU 67 TOPS**, CPU 5 TOPS. Moving work to the NPU is a step down from the
engine we already use.

**2. Our GPU kernel is at 4 % of that GPU.** The cooperative-matrix kernel measures
**1348 GFLOP/s**; the Arc 140V's FP16 peak is around 33 TFLOPS. There is a **25x**
headroom in the hardware we are already on — no shared-memory staging, no K-blocking,
no register reuse in the shader — before a second accelerator is worth discussing.

**3. Everything shares one memory pool.** *(Corrected 2026-09-09 —
`notes/phase20-machine-limits.md`. The figures first written here, 23.5 GB/s for a copy
and 9.8 GB/s for a read, were **single-threaded numpy** and are not the machine's
bandwidth. One core cannot saturate an on-package LPDDR5X controller: eight give
**70.8 GB/s** on STREAM triad, and the GPU itself reaches **69-91 GB/s**, against
136.5 GB/s theoretical.)* A 720p frame moved **16.46 GB** across the host boundary at
the time, which residency has since deleted. The NPU sits behind the same controller,
so relocating the arithmetic would not change the number that limits us.

That cuts the other way too, and it is the real opportunity the owner was pointing at:
because it is one physical pool, a device-resident implementation needs **no copies at
all** — an upload is an address, not a transfer. Shared memory makes residency worth
more here than it would be on a discrete card.

**4. The graph does not fit an NPU compiler's operator set.** The bit-affine softmax
reinterprets float16 bit patterns as integers, shifts them and adds `0x7FF88000`. The
E4M3 publishes need exact round-half-even into a 4-bit mantissa. The cosine normalise
needs a *specific* half-precision reduction order. OpenVINO takes a fixed op set and
the NPU has no host callback, so anything unsupported would split the graph and add
round trips rather than remove them. And NPU 4's strength is INT8, while we have
already measured that E4M3's 6.25 % quantum sits at an avalanche threshold
(`notes/phase9-numerics.md`) — INT8 would not degrade this model, it would change the
semantics the whole project was spent recovering.

**Verdict: not worth it.** The same effort spent on the shader we already have, or on
residency, pays far more.

## What the "one SoC" instinct did find: the rounding

Chasing the memory-bandwidth question turned up something better. **numpy has no SIMD
path for float16 conversion**, only for float32:

| | Gelem/s |
|---|---|
| float32 copy | 2.94 |
| float32 -> float16 | **0.31** |
| float16 -> float32 | 0.53 |
| **torch float32 -> float16 -> float32** | **2.29** |

This also corrects `notes/phase8-xmx-graph.md`, which said "the float16<->float32
conversions are hardware (F16C)". They are not, in numpy. The rewrite that note
describes was still a real 2x win, but the reason given was half wrong.

Doing the rounding on the float32 exponent and mantissa directly — the trick that won
3.3x for E4M3 — **loses** here, 2.2x: float16's subnormal range needs a second branch,
so it is 14 numpy passes against `astype`'s two. Verified bit-identical over all 65504
float16 values, every exact midpoint and six magnitude regimes first, then discarded.

`src/ref/nr_accel.py` uses torch's conversions instead, behind the same kind of hook as
`nr_model.MATMUL`. It is optional, it is off by default, and `install()` refuses to
proceed unless `verify()` passes — every representable float16 value, every representable
E4M3 value, every exact midpoint of both, and six magnitude regimes, all bit-identical.

### The trap: torch's thread pool

Installed naively it made the frame **worse**, 17.4 s -> 30.9 s, even though the
microbenchmark said torch was 13x faster. torch parallelises every elementwise op, the
graph issues thousands of small rounding calls per frame, and synchronising a pool that
many times costs more than the work — while also fighting the BLAS for cores.

| torch threads | frame |
|---|---|
| 1 | **10.3 s** |
| 2 | 11.3 s |
| 8 | 32.1 s |
| numpy | 17.7 s |

`nr_accel` pins one thread. Three times slower with eight than with one is worth
remembering for any future use of torch here.

## Where the frame stands

384x384 face crop, 1280x720 full frame, best configuration each:

| | 384x384 | 1280x720 |
|---|---|---|
| system numpy (netlib BLAS) | 37.7 s | — |
| system numpy + XMX | 17.0 s | 94.6 s |
| OpenBLAS numpy | 17.4 s | — |
| OpenBLAS + XMX | 18.3 s | — |
| **OpenBLAS + torch rounding** | **10.2 s** | **64.4 s** |
| OpenBLAS + torch rounding + XMX | 11.2 s | — |

The rounding was about 7 s of the 384 frame. The XMX path still does not pay for
itself, for the reasons in `notes/phase13-torch-and-blas.md`, and the ordering of what
is left is unchanged: device residency first, because it is what makes the GPU worth
anything and what deletes the 16.46 GB.
