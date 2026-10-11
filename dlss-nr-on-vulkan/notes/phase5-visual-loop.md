# A frame goes in and a recognisable frame comes out
2026-09-08

## Why this exists

The owner redirected the goal: a *working* version that can be tested, and eventually
run in a game, ahead of notes/CLAUDE.md's "reimplement, do not execute NVIDIA's kernels"
constraint. That reframes what counts as progress.

The AMD project reached bitwise parity by capturing exact state from an RTX card and
replaying NVIDIA's own translated PTX. Both halves of that are closed to us: no RTX,
and no PTX consumer on Intel. Their evidence log is explicit that even they never
recovered the *semantics* — E-24: "semantic field names and tensor layouts remain
unproven pending dynamic RTX resource snapshots". Nobody has published them.

So the useful test is not "does it match NVIDIA" but **"does an image go in and a
recognisable image come out"**. That needs no reference activations, which is the one
thing this project cannot obtain, and it improves measurably as each unknown is fixed.

`src/ref/run_frame.py`, `src/ref/image_io.py` (ImageMagick, since Pillow is absent).

## Measured progress

```
                                    correlation   seam ratio (v/h)   output PNG
plain pass, no skips                   +0.357      -                    8.5 KB
+ encoder->decoder skip connections    +0.785      2.81 / 3.19         171 KB
+ bilinear resample                    +0.848      1.02 / 1.25         175 KB
```

**Skip connections were the big one.** A U-Net without them cannot recover detail
after four downsamples, and the output was a flat mosaic. The kernel names
`BSUpsampleSkip`, `SubtiledSkipBlend` and `cc_tinlayout_upsample_skip_block` confirm
the connections are real; the blend used here is a plain add, which is a stand-in.

**The 8x8 seams were mine, not the network's.** They looked like window-attention
boundaries, so shifted-window alternation was implemented first — and changed the seam
ratio by 0.004, i.e. not at all. The actual cause was the nearest-neighbour resample
standing in for the learned up/downsample layers; bilinear dropped the ratio from 2.81
to 1.02. Worth recording: the obvious suspect was the wrong one, and the cheap
measurement said so immediately.

Shifted-window alternation is kept anyway — it is architecturally correct and the
`_shifted_` kernel variants exist — but it is not what fixed the seams.

## What is still a stand-in

- the stem (`input_adapter_weight`) — RGB is tiled up to 32 channels
- the output head — the first three channels are taken
- down/upsample — bilinear instead of the learned layers
- skip blending — a plain add
- the leading `2.5C^2 + 64C` of every fused block — unapplied, roles unknown
- 15 narrow blocks pass through — their qkv split is undetermined

Everything between those is the recovered network doing real work on NVIDIA's weights.

## Honest reading of the current output

The frame is recognisable, smooth and correctly structured, but blurred and
colour-shifted. That is consistent with the stand-ins: the stem and head are the
colour path, and the unapplied `pre` regions are most of each block's capacity.
Correlation 0.848 with the input is the number to beat as those are filled in.

---

## Correction: correlation with the input is a bad objective (2026-09-08)

The +0.848 -> +0.896 improvement attributed above to an output projection recovered
from the fused blocks' leading region does not mean what I said it meant.

Measured directly: `pre_proj` shrinks the attention branch by **11x to 18x**
(branch sd ratio 0.055-0.093 across blocks 9, 12, 15, 21, 49, 57). Correlation with
the input rewards doing nothing — a branch scaled toward zero scores perfectly — so
almost any small matrix in that position would have produced the same "improvement".
The result is an artefact of the metric, not evidence for the hypothesis.

## A metric attenuation cannot game

`clean_and_noisy()` builds a clean frame and a noisy copy; `denoise_score()` fits
`out = a*clean + b`, allowing the network any rescale it likes, and reports the
residual against the **clean** reference. An identity passes the noise through and
scores exactly the input noise. Scoring *below* that requires actually removing noise,
which attenuation cannot do.

```
config                              correlation   denoise score
input (reference)                        -           0.05753
no skips, no proj                     +0.583         0.42533   7.39x worse
+ skips                               +0.843         0.19578   3.40x worse
+ skips + pre_proj                    +0.891         0.15580   2.71x worse
+ skips + pre_proj + pre_ffn          +0.891         0.15614   2.71x worse
```

**Every configuration is worse than the input.** The reconstruction currently adds
distortion rather than removing noise. That is the honest state, and the earlier
"recognisable frame, correlation 0.85" framing oversold it: the frame is recognisable
mostly because the skip path carries the input through, not because the network is
doing useful work.

Two things are still worth keeping from that round. The ordering is monotone
(7.39 -> 3.40 -> 2.71), so skips and the extra projection do genuinely reduce
distortion by a metric that cannot be gamed by attenuation. And the target is now a
number rather than an impression: **get below 1.0x**, i.e. beat the noisy input.

The `pre_ffn` split changes nothing (2.71x either way) and remains unsupported.

---

## Structural fixes measured against the honest objective (2026-09-08)

### Skip blending, not skip addition

The kernel is called `SubtiledSkipBlend`. A plain add at each of five levels was the
dominant source of scale drift:

```
skip = add     2.71x worse    output mean 3.032 against an input mean of 0.506
skip = mean    1.45x worse    output mean 0.506
skip = rms     1.43x worse    output mean 0.478
```

Blending instead of summing removes the drift completely and halves the distortion.
This is the strongest structural result of the session, and unlike the earlier
"projection" claim it survives a metric that attenuation cannot game.

### Gate form

```
gate on branch    out = skip + c*branch          1.43x worse   spread kept 0.888
gate on skip      out = c*skip + branch          1.01x better  spread kept 0.993
convex            out = (1-c)*skip + c*branch    COLLAPSED     spread kept 0.000
convex inverse    out = c*skip + (1-c)*branch    1.01x better  spread kept 0.993
```

`gate on skip` matches the parameter name `attn_cos_skip` literally and is the first
configuration that is not worse than the input. But 1.01x is parity, not success: the
network is behaving as a near-identity, and the raw output decays to sd 0.006 before
the display normalisation stretches it back.

### The metric had a hole too

The first version divided the residual by the fitted gain, which normalises an honest
rescale but turns into 0/0 when the output collapses. A gate form that drove the
output to sd 0.006 scored *better* than everything else. `denoise_score` now also
returns the retained spread and reports `inf` below 0.2, which correctly flags the
convex form as collapsed rather than best.

Two metrics, two holes, both found by testing rather than by reasoning. Worth
remembering before trusting the next one.

## Standing

```
start of the visual work    7.39x worse than the noisy input
+ skip connections          3.40x
+ leading-region projection 2.71x
+ blend instead of add      1.43x
+ gate on the skip          1.01x  (parity)
```

Getting meaningfully below 1.0x almost certainly needs the parts that are still
stand-ins rather than more guessing at the parts that are recovered: the stem, the
output head, the learned resampling, and the unapplied leading region that holds most
of each block's capacity.

---

## Final sweep of the session, with all three metric holes plugged

```
configuration                    score     corr      verdict
--skip-rms --gate-gated-skip    0.05697   +1.0000   PASS-THROUGH, network bypassed
--skip-rms                      0.08216   +0.9735   1.43x worse than the input
--skip-mean                     0.08345   +0.9723   1.45x worse
defaults (skip = add)           0.15614   +0.8907   2.71x worse
--no-pre-proj                   0.19650   +0.8423   3.42x worse
--no-skips                      0.29516   +0.7166   5.13x worse
```

**Best non-degenerate result: 1.43x worse than doing nothing.** The reconstruction
runs end to end on NVIDIA's weights and is structurally coherent, but it still
degrades the frame rather than improving it.

### Three metric holes, all found by testing

1. *Correlation with the input rewards inaction.* An "improvement" from adding a
   projection turned out to be the branch being attenuated 11x-18x.
2. *Dividing by the fitted gain becomes 0/0 on collapse.* A gate form that drove the
   output to sd 0.006 scored best of all until a retained-spread guard was added.
3. *A pass-through scores exactly 1.00x.* The 1.01x "win" was the network being
   bypassed entirely, visible as correlation +1.0000.

None of these were caught by reasoning about the metric; each needed an adversarial
configuration to expose it. The metric now reports COLLAPSED and PASS-THROUGH
explicitly.

### What survived

Skip **blending** rather than addition, on the strength of the kernel name
`SubtiledSkipBlend` and a 2.71x -> 1.43x improvement under a metric attenuation
cannot game. That is the one structural claim from this phase I would defend.

### What did not

The leading-region projection (explained by attenuation), the feed-forward split
(no effect either way), and the gate-on-skip form (produces a pass-through).

### Why 1.43x and not better

Most of each block's capacity is still unapplied, and the colour path is entirely
stand-ins: the stem, the output head, the learned resampling and the skip blend
weights. Guessing harder at the recovered parts will not close that; the stand-ins
have to be replaced with the real layers, and those need the semantics that neither
this project nor the AMD one has recovered.

---

# Correction 2026-09-08: the acceptance test was not measuring the network

Every number in this document was produced by the resampling and skip-blend
scaffolding, not by the recovered model. `run_frame.py --no-attend` disables the
attention branch in all 44 blocks that have one, and reproduces the whole progression:

```
configuration                        with network   with --no-attend
--no-skips                              5.13x            5.14x
--skip-mean                             1.45x            1.45x
--skip-rms   (the "best result")        1.43x            1.43x
```

The branches move the output frame by **0.35 % relative**, and the score by 3e-5 —
the fourth decimal. So "5.13x -> 1.43x, entirely from skip blending" is true and also
beside the point: it was the *scaffolding* being tuned. Switching the attention from
softmax to the clamped bit-trick softmax that the hardware actually runs
(`phase5-softmax-found.md`), adding or removing the `128C` bias, and exponentiating
`attn_scale` all leave the score identical to four decimals, for the same reason.

## Why the branch is inert

Per-block, measured with `--trace`:

```
branch sd / trunk sd   0.005 - 0.021   at every one of the 44 attending blocks
```

Each block moves the trunk by one to two percent, in a direction that is close to
random, so 44 of them compose to nothing.

It is not a missing gain. Sweeping a global multiplier on the branch:

```
gain     1      1.43x   (moved 0.003)
gain     4      1.44x   (moved 0.011)
gain    16      1.61x   (moved 0.065)
gain    64     29.62x   (moved 0.55)
gain   256      NaN
```

Monotonically worse. Whatever is wrong is the branch's **direction**, not its
magnitude, so the "~450x branch/skip factor" from `phase3-block-internals.md` would
not rescue this even if it were found: applied here it produces noise.

Two of the three attending block families also do nothing by construction:
the 29 fused-Swin blocks have no `attn_scale` extracted, so their logits are a bare
cosine in `[-1, 1]` and every attention row is uniform to 0.01 nats.

## What the test reports now

`run_frame.py` carries a second trunk, `x0`, through the identical scaffolding with
every branch omitted, and reports:

```
denoise: input 0.05753 -> output 0.08216   spread kept 0.888   WORSE 1.43x
scaffolding alone (no branches): 0.08213   network moved the frame by 0.00349 relative
NETWORK-INERT -- the branches contribute nothing; this score measures the stand-in
resampling and skip blend, not the recovered model
```

That is the fourth hole found in this metric, and the largest: the first three let a
wrong configuration score well, this one let the *scaffolding* be mistaken for the
model for an entire phase. The pattern holds — none of the four were found by
reasoning about the metric, only by an adversarial configuration. `--no-attend` is
now the first thing to run when a score moves.

## Standing, restated honestly

The recovered network runs end to end on NVIDIA's weights, agrees between CPU and XMX
to 9.7e-07, and **changes the frame by 0.3 %**. The 1.43x figure is the cost of
bilinear resampling through five levels plus an RMS skip blend. There is no result
here about the model yet.
