# Fusing the elementwise passes, a driver bug that stopped the better version, and
# what 30 fps would actually take

2026-09-09

## Where the resident frame's time goes

Per-submit timing of a 720p frame, by family:

| | ms | share | submits |
|---|---|---|---|
| window blocks | 478 | 45.5 % | 36 |
| transitions | 138 | 13.1 % | 8 |
| stem, block 70 and the head | 135 | 12.8 % | 2 |
| block 0 | 130 | 12.4 % | 1 |
| split blocks | 70 | 6.7 % | 17 |
| global blocks | 36 | 3.4 % | 9 |

Nothing dominates, but the shape is clear: the cost sits in the **wide, shallow blocks
at high resolution**, not in the deep narrow ones. Blocks 0 and 70 alone — one block
each, at the full 1280x768 — are a quarter of the frame.

That is traffic, not arithmetic. At full resolution with C=32 and a 128-wide hidden
layer, one pass over the hidden buffer is 126 MB in float32.

## The fusion, and the win

The graph writes a float32 hidden buffer, gates it, publishes it as E4M3 and narrows it
to half for the next GEMM — **four trips over the same buffer**. Those three
elementwise passes fold into one that reads float32 and writes float16:

```
GATE_E4M3_HALF   out_f16 = e4m3(gate(in_f32))
E4M3_HALF        out_f16 = e4m3(in_f32)
GATE_HALF        out_f16 = half(gate(in_f32))
```

Legitimate without any approximation: an E4M3 value is exactly representable in half,
and the gate already returns a half value, so the narrowing loses nothing either way.
All three are bit-identical to the reference chain.

**720p: 1560 ms -> 1231 ms**, passes 2966 -> 2630, with the result unchanged —
correlation with the host reference still 0.981311 and the head's sd still 0.1833. The
global blocks, whose hidden layer is 4096 wide, went from 136 ms to 36 ms.

## The better version is blocked by a driver bug

> **SUPERSEDED TWICE.** By `notes/phase21-fusion-and-tiling.md` (2026-09-09) for the
> conclusion, and by `notes/phase36-the-bug-is-narrower.md` (2026-09-10) for the table
> itself: a minimal reproducer says **four of the five rows below are wrong**.
> Whole-matrix arithmetic, element assignment and a copy through a fresh matrix are all
> correct on Mesa 26.2.1. Only a *converting* store is broken. The measurements below
> were taken inside the resident kernel, with buffer-reference addressing, batch strides
> and slice offsets; none of that is in the reproducer, and the claim did not survive
> being isolated.
>
> The original conclusion, for the record: the bug is real The corruption is in the *arithmetic*, not in the store: an untouched
> accumulator reaches **shared memory** intact, and the epilogue can then be applied to
> ordinary scalars on the way out. Every epilogue is bit-exact that way
> (`src/gpu/test_epilogue.py`), and the GEMM now carries one.


Folding the same work into the **GEMM's epilogue** — transforming the accumulator
before it is ever written — would turn four trips over block 0's 503 MB buffer into
one 252 MB write, about 13x less traffic rather than 2.6x.

It cannot be done on this driver. **Any operation on the accumulator between
`coopMatMulAdd` and `coopMatStore` scrambles the result.** Established at the crudest
level available:

| what was tried | result |
|---|---|
| untouched accumulator | correct |
| `acc = acc * 2.0` — whole matrix, no element access | **wrong** |
| `acc[i] = acc[i] * 2.0` with constant indices | wrong |
| the same into a fresh matrix, then assigned back | wrong |
| `coopmat<float16_t,...>(acc)` conversion store | wrong |

The values are doubled but land in the wrong places, and not even as a clean
permutation — a sorted comparison against `2 * correct` also fails, so elements are
lost as well as moved. `acc.length()` reports the right 4, so the query works and only
the store's mapping is wrong. Mesa 26.2.1, ANV on Lunar Lake, `VK_KHR_cooperative_matrix`
revision 2.

Worth reporting upstream; until then the epilogue lives in `resident.comp` instead,
fused with the other elementwise passes rather than with the GEMM.

## What 30 fps at 720p would actually take

> **SUPERSEDED 2026-09-09 by `notes/phase25-the-frame-rate-wall.md`.** This section
> extrapolated from arithmetic and bandwidth. Measured across nine extents, the frame is
> `20 ms + 632 ms per megapixel`: 640x384 is **191 ms, 5.2 fps**, not the 30 this
> section's arithmetic suggested, and 60 fps is below the fixed cost outright. The
> kernel work that was supposed to close the gap has since happened — 1025 -> 649 ms —
> and both remaining levers measured null.


The owner's target is 30 fps at 720p — 33 ms a frame against today's 1231 ms, a factor
of 37. Being straight about it:

*(Revised 2026-09-09 by `notes/phase20-machine-limits.md`: the bandwidth figure this
section originally used was single-threaded numpy and understated the machine by 3x.
The corrected version follows.)*

- **The arithmetic.** A 720p frame is 458.6 GFLOP. Our GEMM measures 2584 GFLOP/s on a
  large shape — **8.1 %** of this iGPU's ~32 TFLOP/s FP16 peak — which is 177 ms. At a
  normal 30-40 % of peak it would be **40-50 ms**.
- **The traffic.** About 10 GB of activations a frame, at the **69-91 GB/s** the GPU
  actually reaches: **110-145 ms**, and roughly half that if activations were half
  rather than float32, which is what the vendor's own kernels do.

> **SUPERSEDED 2026-09-09 by `notes/phase21-fusion-and-tiling.md`.** The "about 10 GB
> of activations" below is low by six times: counting the passes as recorded, the frame
> moved ~65 GB, three quarters of it in the elementwise half. It *was* bandwidth-bound.
> The per-dispatch fixed cost is 4.5 us, so 2000 dispatches is 9 ms — dispatch overhead
> was never the problem.

The measured frame is 1025 ms, so most of it is neither peak FLOPs nor peak bandwidth:
it is dispatch overhead and poor occupancy on the many small shapes. That is the
encouraging part — the slack is in our code, not in the chip.

A well-optimised implementation plausibly lands at **100-200 ms, 5-10 fps at 720p**.
30 fps needs about a **640x384** extent, which is a sensible internal resolution for a
720p output, or a face-sized region, where today's 320x320 would land in single-digit
milliseconds.
