# The decoder input upsample, recovered — and the first honest improvement
2026-09-08

## The layer

`block39.layer0.layer` holds **262,656** elements. That is `512^2 + 512` exactly: a
**512 x 512 projection with a 512-element bias**. It is the only layer in the whole
model carrying a bias rather than a cosine gate.

The kernel confirms it is one GEMM and nothing else:

```
cc_dec_input_upsample_1024_512   64 mma, 16 chains, chain length 4 -> K = 64 static
                                 all 64: A <- ld.shared, B <- weight arena
                                 no attention, no gate, no softmax
```

So `1024_512` in the kernel name is the **stage transition** — the 1024-wide ViT-1D
bottleneck feeding the 512-wide decoder — and not the matrix shape. The earlier reading
of it as a `1024 x 512` matrix does not fit the container by a factor of two and is
dropped.

## Measured effect

Wired in place of the pass-through, with the stand-in output head
(`--slice-head`) and the recovered attention:

```
frame   input     attention on    attention off
seed 0   0.05753   0.05618   0.07837
seed 1   0.05761   0.05621   0.07849
seed 2   0.05773   0.05635   0.07843
seed 3   0.05775   0.05632   0.07825
```

Two things happen at once, and the second is the important one.

**The score crosses below the input for the first time in a configuration that is not
degenerate** — 1.02x better, with `spread kept 0.959` and correlation `+0.9985`, below
the `0.9995` pass-through threshold that flagged every previous crossing. The margin is
thin and the improvement is small: two percent.

**The attention branch now measurably helps.** Turning it off with `--no-attend` moves
the same configuration from 0.05618 to 0.07837 — a **1.39x swing**. Through every
previous session the branch changed the score in the fourth decimal; here it is the
dominant term. That is the first evidence that the recovered attention is doing useful
work rather than merely running.

## What it is not

Not a working denoiser. The output is attenuated sevenfold (sd 0.042 against the
input's 0.308), the metric permits that by fitting a gain, and two percent is close
enough to the noise floor of a single test frame that it needs the seed sweep to mean
anything. Four frames give a mean of 1.024x with a spread of 0.004 -- consistent, and
consistently small. And with the **recovered** output head instead of the stand-in the same
configuration scores 32x worse, so the chain is still a mixture of recovered and
stand-in parts and its end-to-end number is not yet a measure of the model.

What changed is narrower and real: one more stand-in was replaced by a layer read out
of the container and confirmed against its kernel, and doing so made the network's own
contribution visible for the first time.
