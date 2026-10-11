# First operator running on real weights — 2026-09-07

`src/ref/hnet_ops.py`. One attention window of `block23` (split-Swin-16H, C=512,
slots 24–56) evaluated with NVIDIA's own parameters.

## What the slices turned out to be

```
wq   (512, 512)     Q projection, all 16 heads
wk   (512, 128)     K projection, 4 KV heads  (GQA 4:1)
wv   (512, 128)     V projection, 4 KV heads
proj (512, 512)     output projection          — Conv2d1x1<512,512>, size-verified
bias (16, 64, 64)   additive attention bias, one 64x64 table per head
```

The GQA split consumes `layer2`'s 1.5C² region **exactly**: C² + C²/4 + C²/4, with
nothing left over. That is an assertion in the code, not a comment — a wrong split
raises rather than silently working on misaligned data.

## Evidence that the layout is right

Numbers from the self-test, on real weights and a synthetic window:

```
window partition (16,16,8) -> (4,64,8) -> back        exact round-trip
attention rows sum to 1                               max deviation 4.4e-16
per-head entropy (first four heads)                   3.40, 3.92, 3.82, 3.60 nats
output (64,512)                                       finite, mean +4.4e-05, sd 2.6e-04
```

The entropies are the informative part. A window has 64 tokens, so a uniform
distribution would be ln 64 = **4.159** nats and a collapsed one would approach 0.
Measured 3.4–3.9 is the signature of a trained attention: clearly structured, not
degenerate, not uniform. Had the bias table been misaligned or misinterpreted, it
would land at one extreme or the other. It is the strongest functional check we have
so far that the recovered layout is genuinely correct and not merely arithmetically
consistent.

## Still open

`layer1` and `layer3`, both C²/2 + C, are marked HYPOTHESIS in the code and are not
exercised by this test. The stage exposes six kernel types (`qkv`, `proj`,
`proj_pool`, `ffwd`, `ffwd_proj`, `final_head`) but each block carries only four
sub-tensors, so a "block" here is one sub-layer group rather than a whole
transformer block. Which of the two feed-forward tensors is which, and whether the
33 slots of the stage compose eight full transformer blocks or something looser,
needs the per-slot launch order — the one thing only hardware capture provides.

Nothing downstream depends on that yet: the attention path is complete and checked.

---

## Update — the trailing `C` is a gate, not a bias (2026-09-07)

Measured on `layer1` and `layer3` of blocks 23, 24 and 40, all of size `C^2/2 + C`:

```
block23 layer1   tail C values: min -0.3135  max +0.9824  mean 0.639
block23 layer3   tail C values: min -0.9795  max +1.0000  mean 0.960
block24 layer1   tail:          min +0.4750  max +1.0000  mean 0.837
block40 layer3   tail:          min +0.2660  max +0.9990  mean 0.789
```

Every value lies in **[-1, 1]** with the maximum sitting exactly at 1.0. These are
**cosines**, matching the parameter names `attn_cos_skip` / `ffn_cos_skip` recovered
from `.rdata` in Phase 2. So the layout is `[C^2/2 matrix][C cosine gate]`, and those
matrices carry **no bias at all** — which is why the earlier "+C bias" reading never
sat right.

Orientation: reading the body as `(C, C/2)` gives row/column norm cv of 0.314 / 0.106
against a shuffled control of 0.137 / 0.100; as `(C/2, C)` it is 0.287 / 0.152. The
`(C, C/2)` reading is the more strongly asymmetric one, so the matrix maps C -> C/2.

`cos_skip(branch, skip, gate) = cos*skip + sin*branch`, `sin = sqrt(1-cos^2)`, is a
norm-preserving mix and is implemented as a stated hypothesis.

## Full block forward, and what it reveals

```
attention          (64,512)  sd 2.58e-04
after cos_skip A   (64,512)  sd 6.46e-02
feed-forward       (64,512)  sd 5.09e-06
after cos_skip B   (64,512)  sd 6.19e-02
mean row norm      2.2406 -> 1.4008   (ratio 0.625, no blow-up, no collapse)
```

Everything is finite and the scale is stable. But note the imbalance: the attention
branch is ~250x smaller than the skip path, and the feed-forward branch ~12,000x
smaller. With a cosine gate averaging 0.64–0.96 the skip dominates almost completely,
which cannot be the intended behaviour of a 71-block network.

**That points at a missing normalisation.** Two pieces of evidence line up with it:
`attn_scale` is in the parameter-name table and we have not located or applied it,
and `linalg_vector_norm` appears among the surviving ATen op names — this model
normalises by vector norm somewhere. Finding where is the next concrete question,
and it is a much better-posed one than "write the operators" was an hour ago.

---

## QK-normalisation and `attn_scale` located (2026-09-07)

`cc_split_swin_16h_qkv_512` issues exactly **32 `rsqrt.approx.ftz.f32`** for a 16-head
block — two per head, i.e. Q and K each L2-normalised before the dot product. The
surviving ATen op name `linalg_vector_norm` agrees. Note this normalisation is
**parameter-free**, which is why no norm weights ever turned up in the accounting.

`attn_scale` is the trailing `2H` values of `layer2`, at the **odd** positions:

```
block23 (C=512, 16 heads): 2.055 1.397 1.107 1.424 1.586 1.755 1.767 1.709
                           1.719 1.785 1.547 1.640 1.833 1.642 1.609 1.856
block31 (C=1024, 32 heads): 32 values, range 1.72 - 2.18
```

One value per head, in a tight 1.1 - 2.2 band: a learned attention temperature.
Count equals head count in both stages. The even positions of that same trailing run
are erratic (tiny values mixed with -24080, +1791, +512) and are **not** identified.

### The decisive check

QK-normalisation should make the branch invariant to input scale. Measured:

```
input sd 0.1  ->  raw branch 2.585e-04   QK-normed branch 2.606e-03
input sd 1.0  ->  raw branch 2.244e-03   QK-normed branch 2.230e-03
```

The raw path scales with the input by 8.7x; the normalised path moves by 1.2%.
That is the property QK-norm exists to provide, and it confirms the placement.

### The bias does contain hard masks

Range of the 128C bias region is `[-99.75, 0.000]`. `exp(-99.75)` underflows to
exactly zero, so shifted-window masking sits alongside the soft positional prior.
An earlier pass looked at a different slice and found nothing below -20; this
supersedes it.

Attention entropy over a 64-token window (uniform would be 4.159 nats):

```
content only   4.117      bias only   3.530      combined   3.496
```

Position prior leads, content modulates. Reasonable for a renderer.

## The open problem, stated precisely

The attention branch comes out ~450x weaker than the skip path, and the magnitude is
fully explained by the weights themselves (matrix sd ~0.008, averaging over 64
tokens) — so this is not an implementation slip. A multiplier of order a few hundred
is missing. Candidates: a separate output scale, a second normalisation after
attention, or a different `cos_skip` form than the norm-preserving one assumed here.

---

## The missing factor is a runtime constant, not data (2026-09-08)

Measured, per block, the single scale that would balance the attention branch against
the skip path:

```
block23  388.7  (2^8.60)     block40  518.6  (2^9.02)
block24  380.9  (2^8.57)     block44  262.7  (2^8.04)
block25  334.0  (2^8.38)     block47  245.2  (2^7.94)
block26  222.9  (2^7.80)
```

Every block lands between `2^7.8` and `2^9.0` — a narrow band. Searching each block's
unidentified `unknown_even` slots for a value in a 0.2x..5x window around its own
requirement finds nothing consistent; those slots hold values from 0.04 to 57,000 with
no relation to the needed number.

**A quantity that barely varies across blocks is a constant, not a parameter.** Were
it stored data it would track the weights block by block. This corroborates, from a
second direction, the earlier conclusion that the factor lives in the launch parameter
block and is not recoverable from the PTX or the weights alone.

### Why the branch cannot simply be small

It is tempting to write the weak branch off as ordinary residual behaviour. It is not.
The cosine gate averages ~0.96, so the trunk is *multiplied* by 0.96 each block: over
71 blocks that is `0.96^71 = 0.055`, a twenty-fold decay. A branch at 1/400 of the
trunk contributes `0.28/400 = 7e-4` per block and cannot compensate. The signal would
simply fade out. The branches have to be comparable to the trunk for the network to
carry anything to its output, which makes the missing factor a real gap rather than a
cosmetic one.

---

## Correction: there may be no missing factor at all (2026-09-08)

The argument above for a missing ~2^8.5 scale rested on the cosine gate being used as
`out = cos*skip + sin*branch` with `sin = sqrt(1-cos^2)`. Under that form the trunk is
multiplied by ~0.96 per block, decays twenty-fold over 71 blocks, and a branch at
1/400 of the trunk cannot carry the signal — hence "the gap is real".

**The PTX disproves the premise.** Across the fused Swin kernels at all four widths,
the split-Swin projections, the ViT-1D projection and feed-forward, and the output
post-block:

```
sqrt instructions                     0
fma against the immediate 1.0f        0
sub from the immediate 1.0f           0
```

Nothing derives a sine from a cosine anywhere. The `rsqrt` that *is* present (32 per
fused kernel, independent of head count, so one per subgroup lane) is the
QK-normalisation, not a gate computation.

So the gate is applied as a plain multiply. If it multiplies the **branch**
(`out = skip + gate*branch`), the trunk does not decay at all, and a branch two or
three orders below the trunk is ordinary residual behaviour in a refinement network —
**there is no missing factor, and the earlier conclusion was built on an assumption I
had not tested.**

The parameter name `attn_cos_skip` argues the other way, for the gate multiplying the
*skip*. Both readings are defensible from the name alone and the PTX does not settle
which operand it lands on without a full dataflow trace. What the PTX does settle is
that the norm-preserving sine/cosine form is not what happens.

Status of the "missing 2^8.5 factor": **withdrawn as a finding.** It was inferred from
a form that has now been ruled out. It may still exist — the launch parameter blocks
do carry unread scalars — but nothing currently demands it, and it should not be
carried forward as an open problem until there is evidence that does not depend on the
disproven premise.
