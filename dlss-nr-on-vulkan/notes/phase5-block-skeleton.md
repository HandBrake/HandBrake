# The fused Swin block, as six GEMM classes
2026-09-08 — mechanical, with reaching definitions in program order

`cc_tinlayout_fused_swin_8h_256_8`: 576 `mma`, 160 chain-starts. Classified by where
each operand comes from (SHARED = staged activations, GLOBAL = the weight arena,
MMA = another matmul's result):

```
 x384   A=SHARED          B=GLOBAL      C=MMA / imm        the projection GEMMs
  x32   A=SHARED          B=GLOBAL      C=GLOBAL+SHARED    the GATED RESIDUAL, start
  x64   A=GLOBAL+MMA      B=MMA         C=GLOBAL / MMA     S = Q.K^T
  x64   A=MMA (softmax)   B=MMA         C=MMA / imm        P @ V
  x32   A=MMA             B=GLOBAL      C=imm / MMA        a further weighted GEMM
```

The three previously-established facts all land in the right class: the gated residual
in the C operand (`phase5-gate-on-skip.md`), the per-head FP32 scale on the A path of
`S = Q.K^T` (`phase5-attn-scale-fp32.md`), and the whole bit-trick exp on the A path of
`P @ V` (`phase5-softmax-found.md`, its f32 constants appear verbatim in that class).

## New: the attention accumulator is initialised from the arena

The chain-starting `S = Q.K^T` matmuls do not begin from zero. Their C operand is a
`ld.weak.global.ca.v4.u32` from the weight arena, at offsets that step by **512 bytes**
per tile:

```
cc_tinlayout_fused_swin_8h_256_8    C <- arena + 1114656, +1115168, ...
cc_split_swin_16h_qkv_512           C <- arena + 1572864, +1573376, ...
```

`mma` computes `D = A*B + C`, so the block computes `S = Q.K^T + table`. **An additive
pre-softmax logit bias exists**, structurally, in both kernel families. That is what
`hnet_ops` has been doing with the `128C` region all along.

## And it contradicts the values we extract

It cannot be reconciled with the region as currently read. `phase3-bias-region.md`
measures a per-head constant of **-56.2** with sd 0.86. Under the real softmax there is
no max subtraction and the logits are clamped to +-6 (`phase5-softmax-found.md`), so:

```
S = Q.K^T + (-56.2)         |Q.K^T| <= scale <= 28
  in [-84, -28]             ->  t = S*0.0449 + 1.3008  <=  -0.16
                            ->  clamped to lo = 1.03125 for every entry
                            ->  every attention weight identical -> uniform attention
```

Measured, and it is what happens: adding the region raises row entropy toward the
4.159-nat uniform maximum instead of sharpening anything.

So the structure is right and the **bytes are wrong**: whatever sits at the arena
offset the kernel loads, it is not the run our block layout labels `attn_bias`. The
question is now sharp and falsifiable rather than open-ended, which it was not before.

Two candidates, in order of prior:

1. **Our layout puts the bias in the wrong place.** The `128C` boundary was derived
   from a size formula and anchored on the cosine gate, not on anything the kernel
   does. The kernel gives an actual address.
2. **It is not FP16 there.** The precedent is fresh: `attn_scale` was FP32 inside an
   FP16 container (`phase5-attn-scale-fp32.md`), and the region's own storage is odd —
   sign and exponent constant, 321 distinct values in 65,536 words, almost all
   `0xD3xx`. FP32 pairs are ruled out (`0xD3xxD3xx` is 2^39); something narrower is not.

## Next step, concretely

Map arena offsets onto our block layout. The absolute offsets mix per-layer bases with
shared-memory constants, so use **differences**, which are base-independent: the bias
tiles step by 512 B, and the gate, scale and weight offsets in each class give a set of
deltas that can be matched against the layout's element counts (for C=256:
pre 360,448 B | gate 512 B | qkv 196,608 B | bias 65,536 B | tail ~66,096 B).

Anything recovered from the leading region then has a numeric acceptance test already
waiting: it must bring the branch from 0.015*x to about 0.13*x
(`phase5-gate-on-skip.md`).

---

## Addresses resolved: the bias is 64x64 per head, and the shape was right

Taking **only `add.s64` immediates** as genuine arena byte offsets (32-bit adds and
shifts are shared-memory and index arithmetic, and mixing them in was what made the
first offset dump unreadable):

```
cc_split_swin_16h_qkv_512   C operand: 1572864, +512, +512, ...  16 tiles, span 8192 B
cc_tinlayout_fused_swin_8h  C operand: 1114656, +512, +512, ...  16 tiles, span 8192 B
```

16 tiles x 512 B = 8192 B = **4096 FP16 = 64 x 64**, in both kernel families. The
per-head stride is computed at run time from `ctaid`/`tid`, so only one head's table
appears as literals. So the `128C` region really is `H` tables of `64 x 64`, and
`hnet_model`'s `(H, TOKENS, TOKENS)` reshape is correct. Structure, address and shape
all check out.

## The values still do not, and one more reading is now dead

Measured across 16 blocks: the per-head constant is **not** the uniform -56.2 the
earlier note reported from a single block. It ranges from **-8.2 to -79.9** by block,
and the raw span reaches -99.8.

A promising-looking reading died within the minute: printed at two decimals the maximum
is `0.00` in every 512-wide block, which is the exact signature of an additive
attention **mask** (0 where allowed, large negative where masked). It is a formatting
artefact. The fraction of entries that are *exactly* zero is **0.0000**, and the
narrow blocks give maxima of 0.04 / 0.01 / 0.13. Not a mask. (Third time this region
has produced a false positive; the first two are in `phase3-bias-region.md`.)

Subtracting the per-head constant does give usable numbers — centred ranges of
+-6 to +-10 for blocks 40/45/25 and entropy falling from 4.15 (uniform) to 2.9-3.6 —
but block 23 centres to +56.2 on the positive side, far outside the +-6 clamp, so a
plain re-centring is not the answer either.

## Where that leaves the region

Structure confirmed, values not. The remaining explanations, in order:

1. **The arena is host-built and the stored form is not the runtime form.** There is
   already one proven instance: `attn_scale` is FP32 in the arena inside an FP16
   container (`phase5-attn-scale-fp32.md`). The region's storage has the same smell --
   sign and exponent constant across almost all 65,536 words, so the FP16 value is a
   *linear function of the mantissa field*, i.e. an offset-and-scale encoding of a
   small integer. What the host multiplies it by is not in the weight blob.
2. **The layout boundary is off.** It was derived from a size formula and anchored on
   the cosine gate, never on an address. We now have addresses; the per-layer arena
   base is the missing link, and it is obtainable by matching the *deltas* between the
   attention-bias base, the scale and the gate against the layout's element counts.

Option 2 is the cheaper of the two and is the next step.

---

## The arena is not the blob: every weight matrix is stored twice

Path 2 was "find the per-layer arena base and match the deltas". It closed, and the
answer invalidates the premise of the question.

The qkv weight address in `cc_split_swin_16h_qkv_512` has **no constant term** --
`addr = arena_base + 32*(96*%tid.y + 384*%ctaid.z) + 16*%laneid` -- so the qkv sits at
offset 0 of the kernel's weight pointer, and the attention bias at 1572864, the
per-head scale at 1703936.

Measuring the bias offset across all four fused widths and fitting:

```
C=32   22,624 B      bias_offset(bytes) = 16C^2 + 258C + 32   for C >= 64
C=64   82,080 B
C=128  295,200 B     C=32 falls 2048 B = 1024 elem = exactly C^2 short
C=256  1,114,656 B
```

Against the blob layout before the bias -- `pre 2.5C^2+64C | pad 8 | gate C | qkv 1.5C^2`:

```
C      blob elem    x2 everything   x2 except the gate    measured
64        20,552          41,104               41,040       41,040   EXACT
128       73,864         147,728              147,600      147,600   EXACT
256      278,792         557,584              557,328      557,328   EXACT
32         6,184          12,368               12,336       11,312   short by C^2
```

**Every weight matrix occupies twice as many elements in the arena as in the
container. The C-length gate vector does not, and neither do the bias table (the
bias->scale gap is exactly 128C at both widths) nor the per-head scale.** The split is
along a meaningful line: the doubled regions are exactly those consumed as `mma` A/B
fragments; the untouched ones are the accumulator table, a broadcast vector and a
scalar. A fragment-layout repack that stores each matrix twice -- two operand
orders, or the shifted and unshifted window variants -- is the obvious shape of the
explanation, but which of those it is has not been measured, so it is not claimed.

The C=32 shortfall is `C^2` in arena elements = `0.5 C^2` in container elements, which
is **exactly the C=32 anomaly `phase3-block-layout.md` already records** ("the formula
`4C^2 + 64C` is right for C >= 64 and wrong by 0.5C^2 at C = 32"). Two entirely
independent measurements -- parameter accounting over the container, and load
addresses in the compiled kernel -- reproduce the same deviation at the same width.
That is the strongest cross-validation the block layout has.

### Withdrawn: "3 C^2, therefore K and V are pre-replicated"

The previous draft of this section read the `qkv -> bias` span of 3 C^2 (against 1.5
C^2 stored) as GQA 4:1 with K and V replicated across the four query heads that share
them -- "an exact fit, the only decomposition that works". It is withdrawn. The factor
is **2, uniformly, across every matrix in the block**, including ones with no grouped
query structure at all. Fitting a GQA story to it was reading a specific mechanism
into a general constant, which is the same mistake as the mask reading two sections
above and the ~450x factor before that.

### What this changes

**Arena offsets do not map one-to-one onto container offsets.** Every inference of the
form "the kernel loads at arena X, therefore the container region at X is that tensor"
is invalid unless the 2x on matrices is divided out first.

That is the mechanism `option 1` needed. It does not by itself prove the `128C` values
are transformed — the bias could be expanded in offset and untouched in value — but it
removes the reason to think they are not, and it explains how a region can be
structurally confirmed at an address while the bytes we read elsewhere disagree.

**Two things to re-examine because of this:**

1. `phase3-execution-order.md` states our decode is *byte-identical to a model arena
   captured from a running RTX 50* (SHA-256 `A5513B18...BD4EE3E5`). If the runtime
   arena carries qkv at 3 C^2, that capture was of the **container**, not the runtime
   arena. The claim is probably fine and mislabelled, but it is load-bearing — it is
   the project's only anchor to real hardware — so it should be checked rather than
   assumed.
2. The Q/K/V split at C=32 and C=64 -- **attempted and not settled**. At C=64 and
   above the sub-matrix selection is entirely runtime-indexed through `ctaid`/`tid`,
   so Q, K and V report the *same* literal base and the literals carry no information.
   Only C=32 exposes distinct ones (Q 19552, K 20576, V 21088/21600/22112, stepping
   512 B up to the bias at 22624), and they are incomplete -- one Q tile was captured,
   and there is no way to show it is the whole matrix. The route that remains is to
   decode the **index strides** rather than the literals: the multipliers in
   `32*(96*tid.y + 384*ctaid.z)` are the sub-matrix strides, and they are present at
   every width.

### Confirmed as a by-product

The ordering `[qkv][128C bias][H x FP32 scale]` is now
established by address at two widths, not by a size formula.
