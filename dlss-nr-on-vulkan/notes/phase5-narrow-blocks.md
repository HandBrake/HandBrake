# The narrow blocks: head_dim is 32 everywhere, and that is now a measured contradiction
2026-09-08

## The tool

`src/tools/ptx_addrform.py` reduces a PTX address to a linear form
`const + sum(coeff * special register)`, resolving through add/sub/mul/shl/mad and
treating the kernel's buffer pointer as the origin. Validated against the one address
that had been derived by hand:

```
qkv weights, cc_split_swin_16h_qkv_512
   decoded:  12288*%ctaid.z + 3072*%tid.y + 16*%laneid
   by hand:  32*(96*%tid.y + 384*%ctaid.z) + 16*%laneid      identical
```

Literals were a dead end for this question — at C >= 64 the Q/K/V sub-matrix is
selected entirely at run time, so all three report the same literal base. The strides
are present at every width.

## What the strides say

```
attention bias   32768*%ctaid.z + 8192*%tid.y + 16*%laneid + 1572864
per-head scale      16*%ctaid.z +    4*%tid.y             + 1703936
```

`%tid.y` steps the bias by 8192 B = 4096 FP16 = **exactly one 64x64 table**, and
`%ctaid.z` steps it by four of them. The scale steps by 4 B (FP32, as established) and
16 B respectively. Both give the same index: **`head = 4*%ctaid.z + %tid.y`**, 4 x 4 =
16 heads at C=512. The bias is one 64x64 table per head, indexed identically to the
scale — two structures, one indexing scheme, no assumptions.

## head_dim is 32 at every width — measured, not inferred

The QK normalisation reduces a sum of squares across lanes before the `rsqrt`. Its
width is readable directly: count the squaring `mul.f16x2 x,x` per lane and the
`shfl.sync.bfly` steps.

```
kernel                              C     heads   squares  butterflies  lanes  head_dim
cc_split_swin_16h_qkv_512          512      16       4          2         4      32
cc_tinlayout_fused_swin_8h_256_8   256       8       4          2         4      32
cc_tinlayout_fused_swin_4h_128_4   128       4       4          2         4      32
cc_tinlayout_fused_swin_2h_64_2     64       2       4          2         4      32
cc_tinlayout_fused_swin_1h_32_1     32       1       4          2         4      32
```

`4 lanes x 4 f16x2 pairs x 2 = 32` — identical in all five, including the two narrow
widths. The head count `C/32` is independently confirmed twice: it is the number in
each kernel's name, and it is `128C / 4096` — the bias region divided by one table.

## The contradiction, with numbers on both sides

The qkv span was measured at C=32 from the arena addresses: the ten weight tiles run
from 16480 to 22112, and with the trailing tile the total is **6144 B = 3.000 C^2 in
arena elements = 1.5 C^2 in container elements** — the layout's qkv size, confirmed by
address at the narrowest width.

Now put the three measurements together at C=32:

```
head_dim = 32, heads = 1   ->  Q must be C x 32 = 1024 = C^2       (forced)
qkv total = 1.5 C^2 = 1536 ->  K + V = 512, so 256 each = C x 8
K of width 8 cannot form a dot product with a Q of head_dim 32.
```

Every one of those three numbers is now measured rather than assumed, so the
contradiction is real and one of the premises is wrong. Before this the narrow blocks
were merely "undetermined"; they are now a specific inconsistency with a specific
place to look — the split of the 1.5 C^2, which is the only one of the three that was
not measured directly at this width. The tile assignment cannot settle it: Q, K and V
accumulator chains interleave in the unrolled code, and a walker that takes the first
producer it finds crosses between them. That is why the per-matrix breakdown printed
in the working session (Q 2 tiles, K 2, V 6) is **not** reported here as a result.

## What would settle it

Separate the Q/K/V chains properly — follow each attention `mma`'s A and B operands
back through their *own* accumulator chains without crossing, and read off the three
spans. The union span is already known to be exactly 1.5 C^2, so the three parts must
close to it, which makes the answer checkable rather than merely plausible.

Until then `hnet_model` continues to expose the narrow blocks' qkv whole and the 15
blocks continue to pass through, which remains the right behaviour: the loader does
not invent a split it cannot verify.

---

## The chains, separated honestly — and what that disproved

`src/tools/ptx_chains.py` builds the accumulator chains as **disjoint sets**: each
`mma`'s C operand is resolved by reaching definition, giving a C-link forest, and every
`mma` gets exactly one chain id. Tracing back from an attention operand then stops at
the first `mma` on *each* path and returns the **set** of chains reached, rather than
the first one a depth-first walk happens to find.

### It immediately found an error in my own earlier numbers

K and V came back with **identical chain sets at every width**. The cause: I had been
identifying the `P@V` matmul as "an `mma` whose A operand comes from `mul.f16x2`" — but
that is also exactly what the *attention* matmul looks like, because its A operand is
`q_hat * scale`, a `mul.f16x2`. So the V branch was re-measuring K.

Every per-role figure in the previous working session — "Q 2 tiles, K 2, V 6" and the
per-head 1024/512/1536 B breakdown — was K counted twice. It was already excluded from
the notes as unreliable; it was worse than unreliable.

The corrected taxonomy, by what each operand's reaching definition is:

```
cc_split_swin_16h_qkv_512 (224 mma, 96 chains)
   x96   A <- ld.shared      B <- ld.global v4        the QKV projections
   x64   A <- mul.f16x2      B <- movmatrix.sync.trans
   x32   A <- mul.f16x2      B <- mul.f16x2           C from global  (the bias)
   x32   A <- mul.f16x2      B <- mul.f16x2

cc_tinlayout_fused_swin_1h_32_1 (512 mma, 192 chains)
   x128  A <- mov.b32        B <- ld.global v4
   x128  A <- mul.f16x2      B <- ld.global v4
   x128  A <- mma            B <- ld.global v4
   x64   A <- mul.f16x2      B <- movmatrix.sync.trans
   x32 + x32  A,B <- mul.f16x2
```

**`movmatrix.sync.trans`** — an in-register fragment transpose — feeds the B operand of
64 matmuls. It was not in any earlier census.

### And it showed the address route cannot answer the question

Separating the chains is not the bottleneck. Measuring each chain's weight footprint is,
and it cannot be done:

```
cc_split_swin_16h_qkv_512   B-operand global loads: 192 seen, 40 reduce to a linear form
cc_tinlayout_fused_swin_1h  B-operand global loads: 768 seen, 384 reduce
```

What blocks the rest is **swizzled addressing**: `shr.u32` x192 and `or.b32` x64 on the
address chains. A right shift is linear only if the shifted value is a known multiple,
and neither is recoverable without modelling the swizzle itself.

The shared-memory staging route is worse, not better: `cc_split_swin_16h_qkv_512` has
only 2 `st.shared` and 8 `ld.shared` in the whole kernel and all ten are non-linear,
and the C=32 kernel has **none at all** — it stages through `cp.async.bulk`, i.e. TMA,
where the layout lives in a descriptor rather than in address arithmetic.

So: the Q/K/V split at C=32 and C=64 **cannot be settled from addresses**. That is now
established rather than suspected, and it closes the route the previous note proposed.

## The route that is left

Numerical, not structural, and it needs no addresses. We hold the weights. Enumerate
the handful of candidate splits of the 1.5 C^2 region and test each against downstream
behaviour: with head_dim 32 (measured) and QK normalisation (measured), a correct split
must put the logits inside the +-6 clamp and give row entropies in the 3.8-4.1 band the
wide blocks show; a wrong one gives degenerate statistics.

The discipline this project has learned applies directly: **calibrate it at C=512
first**, where the split is known, and confirm the test picks the right answer out of
the same candidate set before running it at C=32. `notes/HANDOFF.md` already records that
statistical segmentation failed calibration twice — but those methods were looking for
*boundaries* in a continuum. This is a choice among a few discrete candidates scored by
a downstream measurement, which is a different thing, and the calibration step is what
decides whether it may be trusted.
