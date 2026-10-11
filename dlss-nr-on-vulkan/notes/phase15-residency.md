# The resident runtime: 16-30x, and the phase13 thesis confirmed

2026-09-09

`notes/phase13-torch-and-blas.md` concluded that the XMX path loses to a good CPU BLAS
not because the kernel is slow — it measures 1348 GFLOP/s and beats OpenBLAS on every
shape — but because every GEMM round-trips its activation through host memory. That
made residency the precondition rather than an optimisation. This is the first slice of
it, and the thesis holds: **the same arithmetic, kept on the device, runs 16-30x faster
than the fair CPU baseline.**

## The design: pointers, not descriptors

The device reports `bufferDeviceAddress = true` and 256 bytes of push constants. So
operands travel as 64-bit device addresses inside the push constants and there are no
descriptor sets at all. Recording a pass is push-and-dispatch, which means a whole
chain goes into **one command buffer with one fence at the end** instead of one submit
per GEMM.

`coopMatLoad` reads through a `buffer_reference` — worth checking before designing
around it, and it compiles and runs.

```
src/gpu/gemm_resident.comp   the cooperative-matrix GEMM, operands by pointer
src/gpu/resident.comp        e4m3, quadratic gate, half rounding, f32<->f16, scale, residual
libxmx.c                     buffer pool, recording, one submit
src/gpu/xmxres.py            Runtime / Buffer
```

Because the APU's memory is shared and the pool is HOST_CACHED, `Buffer.view()` hands
back a numpy array over the same bytes the GPU reads. Feeding an input or reading an
output is an address, not a transfer.

## The subnormal rescale turned out to be unnecessary — and phase4 was wrong

`xmx.py` rescales both operands by a power of two before every GEMM, because
`notes/phase4-subnormal-flush.md` found **27.22 %** of parameters were FP16 subnormal
and XMX flushes those to zero. Measured on the *correctly decoded* logical weights and
on a real frame's activations:

| | float16-subnormal |
|---|---|
| GEMM activations | 0.0015 % |
| GEMM weights | 0.00006 % |

**The 27 % was an artifact of the wrong decode** — reading packed E4M3 and permuted
bytes as dense FP16 produces garbage that is largely subnormal. Flushing every
subnormal operand to zero for a whole frame moves the head by 8.08 % of its sd, *below*
the 11-12 % floor the FP16 path already sits on. So the resident path needs no
max-reduction, no scale buffers and no rescale at all, and it says so rather than
carrying machinery for a problem that does not exist.

## A trap: `float(float16_t(x))` is not a rounding

The shader's half rounding was written as the obvious round trip. The compiler folds it
away, the value stays float32, and **every one of the vendor's rounding points silently
disappeared** — the gate came out as the pure float32 expression. It is not a small
error: the graph's whole character lives in those roundings.

Replaced with the explicit form on the float32 exponent and mantissa — round-half-even
at bit 13, the subnormal range at its fixed 2^-24 step, overflow to infinity — which is
the algorithm already verified bit-exact in numpy and cannot be elided. `precise` is
needed on the gate for the same class of reason: an FMA contraction would skip the
rounding between the multiply and the add.

## What it does

`src/gpu/test_resident.py`. The operators are **bit-identical** to `nr_model` over four
magnitude regimes including the inf/NaN one: e4m3, the quadratic gate, half rounding.

A whole feed-forward — `to_half, gemm, gate, e4m3, to_half, gemm, residual` — is
**7 passes in one submit**. Against the reference given the same half inputs it agrees
to 2.6e-04, which is the E4M3 publish amplifying a float32 accumulation-order
difference; against the float32 reference it differs by 7.2e-03, the known FP16-operand
effect.

Against the fair CPU baseline — OpenBLAS numpy with the torch rounding of
`notes/phase14-npu-and-rounding.md`:

| tokens | host | resident | |
|---|---|---|---|
| 4096 | 22.8 ms | 1.41 ms | **16.1x** |
| 36864 | 133.2 ms | 4.43 ms | **30.1x** |
| 147456 (block 0's extent at 384x384) | 432.4 ms | 16.3 ms | **26.5x** |

## What is still on the host

This is one block family's feed-forward, not the graph. Still to port: the bit-affine
softmax, the fragment-tree cosine normalise and publish, the window partition and
reverse, average pool, the nearest and learned upsamples, the decoder merges, and the
concatenations the branched and split feed-forwards need. Then the block dispatch has
to record a whole frame rather than a chain.

The arithmetic ceiling from `notes/phase11-what-is-left.md` is unchanged: 340 ms for a
720p frame at the kernel's measured throughput, so full-frame real-time remains out of
reach by about 20x. What residency buys is the distance between today's 64.4 s and that
floor.

---

## Later the same day: the whole attention path, bit-identical

The remaining row-wise and permutation operators are done, so a complete window
attention now runs on the device in **19 passes and one submit**.

`src/gpu/attention.comp` holds the two reductions. Both had to have their order
*measured* rather than assumed:

- **The softmax's total is not accumulated in half.** `sum(dtype=float16)` in numpy —
  and in torch — runs the reduction sequentially in **float32** and rounds once at the
  end. An eight-accumulator pairwise tree, the obvious guess from numpy's internals,
  does not match; a plain float32 sequential sum does. The comment in `nr_model` that
  said otherwise is corrected.
- **The cosine normalise** uses the kernel's own fragment tree, which is a fixed shape
  of half operations rather than any reduction library's order, and transcribes
  directly.

The softmax shader also needs its own float32 to float16 bit conversion, because the
vendor's approximation reinterprets the half bit pattern as an integer, shifts it and
adds `0x7FF88000`.

`resident.comp` gained the permutations: window partition and reverse, the head split
that turns `(windows, tokens, 3C)` into Q, K or V as `(windows, heads, tokens, 32)`,
the merge back, and the per-head bias add.

### Verification, and a pleasant surprise

Feeding the reference's float32 projection to both sides isolates the port from the one
real difference, the qkv GEMM's float16 activation. Every stage is then **bit-identical**:
the two cosine publishes, the E4M3 of V, the softmax, the context, the merge publish and
the output.

That includes both **batched GEMMs**, which was not obvious: the device multiplies in
float16 and accumulates in float32 in a different order from numpy. It agrees exactly
because the operands are E4M3-published — four mantissa bits, so products carry eight —
and a sum of 32 or 64 such products is *exact* in float32 whatever the order. The
vendor's own quantisation is what makes the reduction order stop mattering.

End to end, from an ordinary float32 block input, the attention differs from the float32
reference by 3.0e-02 and from the half-input reference by 2.2e-03 — all of it the qkv
GEMM's operands, and **1.98 ms against the host's 37.60 ms, 18.9x**.

### Still on the host

The branched and split feed-forwards' concatenations, the pools and upsamples, the
decoder merges, the shifted-window padding, and the frame-level dispatch that would
record a whole graph rather than a block.

---

## All three block families, one submit each

`src/gpu/nr_resident.py` now records a whole block of any family as a single command
buffer. Correlation with the reference is above **0.9998** everywhere; against the
reference with its GEMM operands rounded to half — which is what the device actually
computes — the spread runs from bit-identical to 4e-02.

| block | family | heads | C | passes | corr | vs half-ref | vs float32 ref | speedup |
|---|---|---|---|---|---|---|---|---|
| 1 | window | 1 | 32 | 30 | 0.999996 | 5.3e-05 | 1.6e-02 | 7.5x |
| 2 | window, shifted | 1 | 32 | 30 | 0.999982 | **6.9e-08** | 4.9e-02 | 9.8x |
| 6 | window, shifted | 2 | 64 | 36 | 0.999841 | 6.5e-03 | 5.9e-02 | 15.1x |
| 9 | window | 4 | 128 | 40 | 0.999881 | **0.0** | 3.7e-02 | 24.0x |
| 20 | window, shifted | 8 | 256 | 48 | 0.999833 | 3.3e-02 | 5.1e-02 | 21.9x |
| 23 | split | 16 | 512 | 49 | 0.999909 | 6.9e-03 | 5.6e-02 | 21.9x |
| 44 | split | 16 | 512 | 49 | 0.999925 | 4.2e-02 | 4.2e-02 | 24.4x |
| 31 | global | 32 | 1024 | 27 | 0.999818 | 4.5e-02 | 4.5e-02 | 7.3x |
| 38 | global | 32 | 1024 | 27 | 0.999770 | 5.0e-02 | 5.0e-02 | 15.3x |

The maxima look large and the correlations say why: a difference of one E4M3 quantum is
6.25 % of a cell, so a handful of flipped cells out of half a million sets the maximum
while leaving the correlation at five nines. Checked directly on the split feed-forward,
stage by stage: correlation **0.99999999** at the first projection, **1.00000000** at
the gated group expansion, 0.99999999 after the group projection. The slices, strides
and offsets are right; the deviation is the avalanche of `notes/phase9-numerics.md`.

### What each family needed

**Branched (blocks 5-22).** Each output head's GEMM writes straight into a 32-column
slice of one wide buffer, and the whole thing is published in a single dense pass —
legitimate because the E4M3 publish is elementwise, so per-head-then-concatenate equals
concatenate-then-publish.

**Split (23-30, 40-47).** `e4m3(x @ first)`, then a per-64-channel-group 64 -> 256 -> 64
MLP. The gate sits between the two group GEMMs with no publish between, so the wide
buffer is gated in one pass and the group outputs are published together.

**Global (31-38).** No windows and no bias, but two things the window path does not
have: the `vit_1d` kernels' **symmetric logit clamp at +-3**, and a token count that is
the bottleneck's pixel count and so need not be a multiple of the cooperative-matrix
tile. The softmax therefore takes a row stride separate from its token count, and zeros
the padding so the following P@V contributes nothing from it. `sqrt(head_dim)` is folded
into the per-head scale on the host.

### Still on the host

The transitions — average pool, the learned and nearest upsamples, the decoder merges —
the stem and the output head, and the frame-level orchestration that would record a
whole graph rather than a block. Those are the remaining pieces before a resident frame
can be timed end to end.

---

## The whole graph, resident: 0.26 s at 384x384 and 1.56 s at 720p

`src/gpu/nr_frame_resident.py` records the entire 71-block graph on the device. The
stem writes a device buffer, every block, transition and skip reads and writes device
buffers, and only the head comes back. **2966 passes** for a frame.

| | best CPU | resident | |
|---|---|---|---|
| 384x384 | 12.69 s | **0.26 s** | **48x** |
| 1280x720 | 64.4 s | **1.56 s** | **41x** |

Peak resident-set 1.0 GB at 720p. Correlation with the CPU reference is **0.9918** on
the head, its sd 0.1377 against 0.1395, and the composed images differ by 0.0032 while
each moves the frame by 0.0260 and 0.0263 — the same effect, and visually
indistinguishable.

For the day's arc at 720p: 123.1 s with the first GEMM hook, 94.6 s with batched
attention, 64.4 s on the CPU once the BLAS and the rounding were fixed, **1.56 s**
resident. `notes/phase11-what-is-left.md` put the arithmetic floor at 340 ms; we are
within **4.6x** of it, and what remains between is the elementwise passes' own
bandwidth rather than any round trip.

### Two bugs worth recording

**Block 0's publish order.** The post block's full-resolution skip is block 0's output
*published*, and the encoder pools its output *unpublished*. Deriving one from the
other — publishing in place and then pooling the published buffer — is wrong, and the
whole frame came out anti-correlated (-0.04).

**A scratch shared across window origins.** Blocks at one level share their working
buffers, and the cache key did not include the window origin. A shifted block has more
windows than an unshifted one — 21x21 against 20x20 at this level — so blocks 2 and 3
wrote past the end of buffers cut to block 1's size. The failure was quiet: the graph
still ran and still produced a picture, with the correlation decaying level by level
(0.97 at L1 down to 0.16 at L5) and the standard deviation inflating. Now the scratch
is sized for the largest window count any origin can produce and each block computes
its own count.

Both were found by comparing level by level against the reference rather than by
looking at the output, which is the only way either would have been found at all.

### What is still on the host

Nothing in the graph. The feature assembly, the head composition and the temporal path
remain host-side numpy, which is right — they are per-frame, not per-block, and they
are not where the time goes.

---

## The surroundings become the bottleneck, and the reprojection moves too

With the graph at 1.56 s for 720p, the host-side temporal work is no longer a rounding
error. Measured at 1280x720:

| | ms |
|---|---|
| `sample_history`, the five-tap Catmull-Rom | **861** |
| the rest of `make_temporal_features` | 350 |
| `extend_features` onto the network extent | 182 |
| `deterministic_noise`, three channels | 110 |
| `compose_temporal` | 56 |

The reprojection alone cost more than half of it, and it is a per-pixel gather, so
`src/gpu/history.comp` takes it: **861 ms -> 2.8 ms, 316x.** It agrees with the
reference to float32 rounding — mean 2e-07, correlation 1.00000000 — and the residual
last bits are the sample coordinate, which a steep image gradient turns into at most
5e-05. That is orders below the graph's own floor.

Two things had to be exact rather than merely close: the five weighted taps are summed
left to right in float32, and the sample coordinate is built the way the reference
builds it. `precise` on both, because a contraction in either changes the answer more
than the arithmetic suggests.

`nr_temporal.install_gpu_history()` puts it in place of the reference's, so it is a
drop-in. A 720p sequence goes from **3.0-4.0 s to 2.1-2.8 s** a frame, with the blend
gate reporting identical alphas — 0.5895, 0.6249, 0.6248, 0.6155 — so nothing about the
behaviour moved.

What is left on the host per temporal frame is about 0.6 s: the feature assembly's
colour scaling, the mirrored extension onto the network extent, the deterministic noise
and the composition. All per-pixel and all portable, but each is now small.
