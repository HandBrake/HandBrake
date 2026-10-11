# The 1.02x depends almost entirely on the input, and here is the shape of it
2026-09-08

Prompted by the right question: does the improvement depend on the input? It does, and
characterising the dependence says far more than the single number did.

## First, a correction to my own claim

"Reproduced on four frames" oversold it. All four were `clean_and_noisy(seed=k)`, and
`test_pattern` is **fixed structure**: the same ramps, the same `sin(x/16)cos(y/16)`,
bright lines every 17 rows and dark ones every 23 columns, identical at every seed. Only
twelve random blocks and the sensor noise vary. Four seeds are four samples of one
image, not four images — and fixed-period content is exactly what could interact with
8x8 attention windows and five 2x resamplings to produce something that is not
denoising.

## Sweeping the noise level

```
sigma   input     output    ratio     residual*   noise retained
0.00    0.00000   0.01630     -       0.00000        -
0.02    0.01941   0.02472   0.785x    0.01858      0.957
0.06    0.05753   0.05618   1.024x    0.05376      0.935
0.12    0.11465   0.10744   1.067x    0.10620      0.926
0.20    0.19294   0.17862   1.080x    0.17787      0.922

* residual = sqrt(output^2 - distortion^2), the part attributable to noise
```

Two constants fall out and they explain everything:

- **A distortion floor of 0.0163.** Fed a *clean* frame, with no noise to remove, the
  network adds that much error. It is not denoising there — it is damaging.
- **It removes 6.5% +- 1.4% of the noise**, and that fraction is nearly constant across
  a tenfold range of noise level.

So the break-even is at **sigma ~ 0.048**, and the headline `1.02x` was measured at
sigma 0.06 — barely above it, where the fixed damage and the small benefit nearly
cancel. At sigma 0.20 the same pipeline gives 1.08x; at sigma 0.02 it gives 0.79x,
i.e. clearly worse.

**The single number was the least informative point on the curve.**

## And the constancy is itself a warning

A learned denoiser removes proportionally *more* noise when there is more of it — it
can tell signal from noise better at high SNR contrast. Removing a **constant 6.5%**
regardless of level is the signature of a **linear smoothing filter**, not of a learned
denoiser. That is consistent with everything else measured about the current state:
correlation with the input at +0.9985, a trunk that stays near-linear, and eight extra
recovered matrices collapsing it to an exact pass-through
(`notes/phase5-resample.md`).

So the honest reading of "the attention branch now matters, 1.39x swing" is narrower
than it sounded: the branch is contributing a *low-pass* effect that helps when noise
dominates and hurts when it does not. Real, measurable, and not yet denoising.

## A structurally different image reverses the sign — the attention claim is withdrawn

`smooth_pattern` is a sum of random low-frequency sinusoids: smooth, no hard edges, and
**no fixed periods**, unlike `test_pattern`'s lines every 17 and 23 pixels.

```
image           sigma   input     attn ON            attn OFF
test_pattern    0.06    0.05753   0.05618  1.02x     0.07837  0.73x
smooth          0.06    0.05989   0.05325  1.13x     0.03468  1.73x
smooth          0.12    0.11988   0.10628  1.13x     0.06935  1.73x
```

On the smooth image **switching the attention off is 1.73x better than the input, and
switching it on drops that to 1.13x** — the branch *costs* a factor of 1.53x, reproduced
to three digits at both noise levels. On `test_pattern` the sign was the other way.

So the previous session's headline — "the attention branch now measurably helps, a 1.39x
swing, the first evidence it is doing useful work" — **is withdrawn.** What that swing
measured was the attention partially repairing damage the *scaffolding* does to
fixed-period high-frequency content: bilinear resampling through five levels destroys
lines at period 17 and 23, `test_pattern` is full of them, and the branch put some of
that back. On content without such lines the scaffolding alone is a good low-pass filter
(1.73x) and the branch only adds distortion.

The distortion floor of 0.0163 measured above is the same effect seen from the other
direction, and it is the honest description of what the branch currently contributes.

## Real game frames settle it

Two Cyberpunk 2077 screenshots, taken as the clean reference with the same synthetic
noise added. (The harness needed a fix first: given a file it had been setting
`clean = img`, leaving the metric no noise to remove and making it degenerate.)

```
frame   sigma   input     attention ON      attention OFF     attention costs
CP-01   0.06    0.05398   0.04395  1.228x   0.03521  1.533x      1.248x
CP-01   0.12    0.10206   0.08165  1.250x   0.05129  1.990x      1.592x
CP-02   0.06    0.05893   0.05025  1.173x   0.04300  1.370x      1.169x
CP-02   0.12    0.11164   0.09378  1.190x   0.06349  1.758x      1.477x
```

The attention branch **costs 1.37x on average** on real content, in all four
configurations. And the scaffolding alone denoises real frames well — 1.37x to 1.99x,
improving as the noise grows, which is what a genuine denoiser does. With the branch on,
the ratio is flat at ~1.2x regardless of noise, which is what a fixed distortion added
to a good filter looks like.

Across three independent image classes:

```
test_pattern, fixed-period lines   attention HELPS   1.39x    <- the only one
smooth, no fixed periods           attention COSTS   1.53x
real game frames (2 x 2)           attention COSTS   1.17-1.59x
```

Four measurements out of five say the branch hurts, and the one that says otherwise is
the synthetic pattern whose fixed-period content the bilinear scaffolding destroys.

## Standing

**No demonstrated denoising.** The reproducible facts are: a fixed distortion of ~0.016
added by the network, ~6.5% of additive noise removed on one synthetic pattern, and a
sign reversal on a second. The scaffolding, not the model, is what removes noise.

Quoting "1.02x better" was wrong twice over — true only at one noise level and only on
one image. This is the fifth hole found in this metric, and the first one found by the
owner rather than by me — and the real frames that settled it were theirs too.
