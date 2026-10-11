# Optional smaller input and output buffers

Two independent experiments in `improve`. Both defaulted to **off**: the 140V
measurements did not justify changing the default, and B570/B580 have not been tested.
Neither changes the model or removes useful output channels.

> **The compact head is on by default since 2026-09-25.** Re-measured paired, three runs
> each, after the week's fusions had shrunk everything else: at 1280x720 the head read
> 3.6 -> 0.75 ms and the whole frame 199-200 -> 195-196 ms; at 320x320 the read 0.45 ->
> 0.2-0.28 ms. The table below was taken when the rest of the frame was 300 ms and hid it.
> On a card across PCIe the read is a quarter of the bytes, which should matter more.
> `NR_COMPACT_HEAD=0` restores the sixteen columns.

## Input: NR_INPUT_FP16=1

> **On by default since 2026-09-25, and built in place.** What made it slower below was
> NumPy's float32 -> float16 cast on the host. The native feature assembly now writes half
> itself, straight into the mapped input (`ResidentFrame.input_view`, the daemon's path), so
> neither the host copy nor the GPU's to_half pass is left; other callers get a native cast.
> It cannot change a value: every feature is a half value already (checked in
> `test_native_image.py`). Paired on the daemon's own path together with the compact head,
> answers byte-identical: 640x360 at 0.5, 37.0-38.1 -> 34.6-36.3 ms; 1280x720 at 0.35,
> 50.4-52.4 -> 48.7-51.2.

The existing path writes sixteen float32 features per pixel into a mapped input
buffer, then runs `to_half` on the GPU. The new path converts directly into a mapped
half buffer with NumPy, and the first GEMM consumes it. It eliminates one dispatch
and halves the input buffer (36 -> 18 MiB at network extent 1024x576). Feature
construction still produces float32 on the CPU; this experiment does not eliminate
that construction or its host allocation.

The mapped input is a HOST_WRITE buffer even with XMX_STAGING=1. This is not an
extra staged upload. On a discrete GPU the selected memory type determines whether
the host writes VRAM or the GPU reads host memory; do not interpret host timings
as a direct measurement of PCIe bandwidth.

## Output: NR_COMPACT_HEAD=1

The last GEMM computes sixteen columns because that is its cooperative-matrix tile,
but only the first four are useful. The compact path keeps the same arithmetic,
stores the tile in shared memory, and writes just those four columns to a contiguous
FP32 output. No separate packing dispatch is added. The output buffer drops from
36 to 9 MiB at 1024x576, and the CPU reads a contiguous array instead of gathering
four values out of every sixteen.

Flag 0x10000 selects this store in the base 8x16 GEMM shader. The runtime requires
N=16, ldc=4 and plain FP32 output, and excludes the larger tiled/staged kernels.
The Python entry point additionally rejects custom leading/offsets. Tests cover
batch strides, transposed B, output guards and both specialization modes.

Each option has its own graph-key bit and buffer name. Toggling an option never
reallocates a buffer referenced by an existing captured graph.

## Measurements on Intel Graphics (LNL / Arc 140V)

16 alternating pairs per row, FFN batching enabled, identical output checked after
every run. Warm replay time includes input conversion/write, graph wait and head read;
it excludes feature assembly, composition, sockets and the game. Different processes
were used for the rows: compare before/after within each row, not across rows.

| option | network extent | off median ms | on median ms | off / on range ms |
| --- | --- | ---: | ---: | --- |
| FP16 input | 1024x576 | 290.516 | 296.195 | 286.356..295.212 / 291.034..302.753 |
| compact head | 1024x576 | 310.365 | 308.728 | 306.533..316.528 / 303.980..310.624 |
| compact head | 832x512 (800x450 input) | 209.760 | 208.027 | 205.717..212.999 / 205.945..211.387 |

FP16 input was slower on this machine. Compact head saved a small amount, with
overlapping ranges; it is not evidence of a large FPS gain. At 832x512 the host-read
median fell from 1.893 to 1.131 ms, while graph-wait medians were 205.580 / 205.799 ms.
Medians of individual components do not necessarily sum to the median total.

The benchmark now applies the daemon's vendor-aligned geometry. An earlier 800x450
run accidentally used an unpadded graph and is excluded from the table above.

## Reproduce on B570/B580

```sh
make
python3 src/gpu/test_input_fp16.py
python3 src/gpu/test_compact_head.py
XMX_STAGING=1 python3 src/gpu/test_input_fp16.py
XMX_STAGING=1 python3 src/gpu/test_compact_head.py
python3 src/bench/ffn_batch.py --optimization input --size 576 1024 --pairs 16
python3 src/bench/ffn_batch.py --optimization head --size 576 1024 --pairs 16
python3 src/bench/ffn_batch.py --optimization head --size 450 800 --pairs 16
```

The benchmark toggles only the selected option and prints the other options, the
GPU and selected memory. Leave unrelated environment overrides unset for a baseline.
Set either option on the **daemon**, not just the game's Steam launch line, to test
it in a game. Restart the daemon after changing its environment.

## Validation

- GPU/CPU half conversion: 256,000 cases, including every half value, signed zeros,
  overflow boundaries and both sides of rounding ties; NaN payloads are not compared.
- Compact head: 24 exact GPU cases with guard checks and invalid-argument rejection.
- Full frames at two extents: exact results across replay/single/block modes,
  specialization on/off, cached toggles, changed inputs and combined input/output modes.
- Both new test programs passed normally and with forced staging on LNL.
- Full CMake build and all 25 CTest entries passed on LNL (87.36 seconds),
  including layer presentation, temporal controls and native image operations.

This validates behavior on the iGPU, not discrete-card performance or driver behavior.
