# Batched FFN groups

Branch `improve`, based on `ac6d2a1`. This change only combines independent FFN
group products into batch items of the existing GEMM shader. It does not alter the
matrix tile, inner reduction, epilogue or weight layout.

`NR_BATCH_FFN=1` (default) batches groups. `NR_BATCH_FFN=0` records the original
per-group dispatches, with the existing independent regions. The setting is read
by the daemon/runtime, not the game layer. A runtime toggle gets a separate cached
graph so A/B runs cannot accidentally replay the other schedule.

The 36 branched blocks record 384 group GEMMs separately, or 72 in batches.
The 16 split blocks record 256 separately, or 32 in batches: 640 -> 104, a reduction
of 536 dispatches. This is not a prediction of time saved. The original independent
dispatches could already overlap, and batching can change scheduling and cache use.

## Checks

No GPU or weights required for address and graph-key checks:

```sh
python3 src/gpu/test_ffn_batch.py
```

On a supported Vulkan GPU (the full-frame test also needs the logical weights):

```sh
make test-ffn
XMX_STAGING=1 make test-ffn
python3 src/bench/ffn_batch.py --size 576 1024 --pairs 8
```

The GPU operator check compares separate and batched dispatches bit for bit, with
guard elements after the output and row counts selecting the base, tiled and
shared-memory-staged pipelines under default tile settings. The full-frame check
compares both schedules and cached replays, with and without specialization, at
multiple extents. The benchmark alternates run order, excludes capture/compilation,
checks every head and reports median wall time including input write and output
read. It does not report game FPS.

Run the paired benchmark on B580 before claiming a performance improvement there.
`NR_BATCH_FFN=0` is the A/B fallback if a driver or shader variant behaves differently
with batched strided operands.

## Local validation

- `make -j4 all`: passed, including the C libraries and all default shaders.
- `test_ffn_batch.py`: passed the 16 addressing cases, output coverage, cache-key
  separation and production-recorder count reduction.
- `test_frame_cache.py`: passed.
- `claims_check.py` and `git diff --check`: passed.
- The initial sandbox run of `make test-ffn` could not see a physical device.
  Running outside the sandbox exposed Intel Graphics (LNL), Mesa 26.2.2.
- `make test-ffn`: passed on that GPU, including 48 bit-identical operator comparisons,
  output guards and full-frame replay/mode/cache/recovery checks.
- `XMX_STAGING=1 make test-ffn`: passed the same checks with unmapped graph buffers.
- Full `make test`: passed outside the sandbox on LNL, including the layer/socket,
  presentation, temporal, GPU operator, frame and CPU reference tests.

## Paired measurement on Arc 140V / LNL

`python3 src/bench/ffn_batch.py --size 576 1024 --pairs 8`, Mesa 26.2.2.
Warm replay with identical inputs and buffers; eight alternating pairs:

| schedule | recorded passes | median ms | min..max ms |
| --- | ---: | ---: | ---: |
| separate | 1664 | 289.522 | 286.871..293.178 |
| batched | 1128 | 287.833 | 285.770..294.213 |

Every output was bit-identical. The median difference is 1.689 ms (1.006x), with
overlapping ranges: this run does not establish a substantial speed improvement
on the iGPU. It does confirm the 536-pass reduction on the actual recorded graph.
This is graph replay plus input write/head read, not a daemon round trip or game FPS.
B580 performance and correctness remain unmeasured.
