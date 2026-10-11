# Resident Vulkan context, and the first honest throughput numbers
2026-09-08

## Why

The subprocess runner built a Vulkan instance, device, pipeline and buffers for every
single matmul, then tore it all down — about 80 ms of fixed cost per call. Any timing
taken through it measured process startup, not the GPU, which is why the earlier
end-to-end comparison (13.1 s XMX against 11.5 s CPU) was reported as meaningless
rather than as a result.

`src/gpu/libxmx.c` keeps all of it alive: instance, device, queue, descriptor set,
pipeline, command buffer, fence, and growable host-visible buffers. A call is now a
memcpy, a submit and a fence wait. `src/gpu/xmx.py` drives it through ctypes, so the
existing tests kept working unchanged.

Effect on the end-to-end pass through all 71 blocks:

```
subprocess runner   13.1 s
resident context     6.3 s          CPU numpy for comparison: 11.5 s
```

Correctness is unaffected — the layer tests still give the same 2.8e-04 worst
deviation, and the end-to-end CPU/GPU agreement is unchanged.

## Throughput

`xmx_gemm` takes an `iters` count that dispatches the same work repeatedly inside one
submit with a barrier between passes, so host copies and submit latency amortise away.

```
shape                iters   GFLOP/s   per dispatch
512x512   K=512       200      1018      0.264 ms
1024x1024 K=1024      100      1900      1.131 ms
2048x2048 K=512        50       716      5.999 ms
4096x1024 K=1024       40       973      8.832 ms
8192x512  K=512        40       787      5.457 ms
```

## What these numbers are worth

Roughly **0.7–1.9 TFLOP/s**, and that is a floor, not a ceiling. The kernel is
deliberately the simplest thing that could work: one 8x16 output tile per subgroup, no
shared-memory staging of the A and B tiles, no K-blocking, no register reuse across
tiles. Every subgroup re-reads its operands from global memory, so this is bound by
memory traffic and not by the matrix units at all.

An Arc 140V-class Xe2 part has substantially more FP16 matrix throughput available
than this, so there is a large factor left on the table. Getting it needs the usual
work — stage tiles through shared memory, block over K, have each subgroup hold
several accumulators — which is exactly the optimisation notes/CLAUDE.md says to defer until
correctness is settled. Recording the baseline now means the improvement will be
measurable later.

The end-to-end 6.3 s is likewise not a frame time. Most of it is the CPU-side softmax,
normalisation and window shuffling in numpy; only the 148 matmuls run on XMX.
