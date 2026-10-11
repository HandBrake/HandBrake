# The gate multiplies the skip, not the branch — read out of the accumulator
2026-09-08

## The question this settles

`phase5-visual-loop.md` tried four gate forms and picked between them by score:

```
gate on branch    out = skip + c*branch          1.43x worse
gate on skip      out = c*skip + branch          "1.01x", flagged a pass-through
convex            out = (1-c)*skip + c*branch    COLLAPSED
```

`gated_branch` was the default because `gated_skip` looked like a pass-through. That
was choosing by outcome. The kernels say which it is.

## What the kernels do

In `cc_tinlayout_fused_swin_8h_256_8`, 64 of the 65 four-byte global loads are **not**
FP32 scalars — they are FP16 pairs, and the discriminator is what happens next:

```
ld.global.b32 -> cvt.rn.f16.f32     one FP32 scalar   (x1  -- the attention scale)
ld.global.b32 -> mul.f16x2          two FP16 values   (x64 -- the gate)
```

Tracing the 64, with **reaching definitions in program order** (see the caveat below):

```
gate  = ld.global.b32 [arena + fixed offset], indexed by %tid.y   -- per channel
x     = ld.shared                                                 -- the block input
prod  = mul.f16x2 gate, x
                 -> operand C of an mma that STARTS an accumulator chain   (x64)
```

`mma` computes `D = A*B + C`. So the block computes

```
D = A.B + (gate (*) x)
```

The residual is fused into the matmul accumulator, **the gate is on the skip**, and
the branch `A.B` is added **unscaled**. Two sets of 32 loads at two fixed offsets, one
per channel, feeding the two GEMMs whose A comes from shared activations and whose B
comes from the weight arena.

`run_frame.py` now defaults to `gate_form = "gated_skip"`; `--gate-gated-branch`
keeps the old form available for comparison.

## A trap avoided, worth recording

The first run of this analysis reported `chain-start = False`, which would have meant
the gated value was added to an *existing* accumulator rather than starting one — a
different and weaker claim. That was an artefact: **PTX is not SSA**, 5 % of register
names in this kernel are defined more than once, and a def/use map that links a
register to *all* of its definitions invents edges. Re-running with the reaching
definition — the last definition before the use, in program order — gives
`chain-start = True` for all 64, cleanly.

5 % sounds small. It was 100 % of the registers this particular question depended on.
Any conclusion from `ptx_trace` that turns on *which* definition reaches a use, rather
than merely on connectivity, has to use program order.

## What it changes, measured

```
                                   network moved the frame   score    verdict
gate on branch (the old default)          0.35 %            0.08216   1.43x worse
gate on skip   (the kernels' form)        8.6 %             0.05697   PASS-THROUGH
```

A 25x increase in how much the network participates, and the first score below the
noisy input. But the metric correctly refuses to call it a win: correlation with the
input is +1.0000. With the gate on the skip the trunk becomes `prod(gate) * x`, a
per-channel rescale, so the whole path stays linear in the input while the branch
still contributes only 8.6 %.

**This is a correctness fix, not a result.** It is kept because it is what the
hardware does, not because the number improved.

## The constraint it hands us

The gates measure 0.82-1.00 with a mean near 0.87, and there are 44 attending blocks.
If the branch stayed as small as it is now, the trunk would decay by `0.87^44 ~ 0.003`
and the network would output a vanishing multiple of its input — which is exactly the
pass-through observed. For the trunk to be norm-stable, the branch has to contribute
of order `0.13 * x` per block. Ours contributes `0.015 * x`.

So the branch is short by roughly **8x**, not by the ~450x that
`phase3-block-internals.md` inferred from the old gate form. That earlier figure was a
consequence of assuming the gate sat on the branch. Eight is a tractable number and it
is consistent with the leading `2.5C^2 + 64C` of every fused block being unapplied --
by the block-layout accounting that region is most of each block's capacity.

**Next test:** whatever is recovered from the leading region should be checked against
this number. A correct reading of it ought to bring the branch to within a small
factor of 0.13, and if it does not, it is probably not the right reading.
