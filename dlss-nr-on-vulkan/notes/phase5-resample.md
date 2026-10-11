# All ten resampling layers located; applying them as projections does not work
2026-09-08

## The localisation is systematic

The method that found the stem and the output head — surplus over a standard block of
the same width — finds every U-Net level at once:

```
encoder (downsample)                 decoder (upsample)
blk4   C=32    +1,024   = C^2        blk39  C=512   = C^2 + C   (its own tensor)
blk8   C=64    +4,088   = C^2 - 8    blk48  C=256   = C^2 + 240
blk14  C=128   +16,376  = C^2 - 8    blk56  C=128   = C^2 + 112
blk22  C=256   +65,528  = C^2 - 8    blk62  C=64    = C^2 + 48
blk30  C=512   +262,152 = C^2 + 8    blk66  C=32    = C^2 + 32
```

Ten transitions, five levels, matching `SKIP = {4:66, 8:62, 14:56, 22:48, 30:40}`
exactly. Encoder blocks carry a bare `C x C`; decoder blocks carry `C x C` plus a short
vector, which is what `block39` already showed as `512^2 + 512`. The `+-8` is the
zero pad the standard block's accounting places differently at each end.

Position established by front- versus end-aligned correlation against a standard
neighbour of the same width: **encoder surplus is at the BACK** (the block works, then
downsamples), **decoder surplus is at the FRONT** (upsample, then work). `blk30`
carries its surplus as a separate `layer4` tensor rather than appended.

They look like trained matrices, and their scale is orderly:

```
C        32       64      128      256
sd    0.0341   0.0164   0.0076   0.0038
```

Halving as C doubles, monotone across four widths.

**`blk66` is excluded.** Its surplus has sd **19.2** against 0.003-0.034 for every
other one — three orders of magnitude out of family. It is not the same kind of object
and no shape is invented for it.

## Applying them makes the result worse

Wired as plain `C x C` projections at the block's own width, in the recovered
positions:

```
                                    denoise   correlation   verdict
--slice-head                        0.05618     +0.9985     IMPROVED 1.02x
--slice-head --resample             0.05753     +1.0000     PASS-THROUGH
```

Exactly the input score, and the pass-through guard fires. The frame does move — 0.0899
relative, more than without them — but it moves *linearly*: eight more matrices in a
trunk that is already near-linear keep it linear, and they wash out the small
non-linear contribution the attention had been making.

Left **opt-in** behind `--resample`, default off, because it regresses. That is a
different judgement from the gate form and the output head, which are kept on despite
the score: for those the *operation* is confirmed by the kernels, while here only the
*existence and position* of the matrices are established. How they combine with the
spatial resample is not, and a plain projection at the source width is the simplest
guess rather than a measured one.

## What this leaves

The pattern is too regular to be coincidence — five levels, two families, orderly
scaling, exact `C^2` sizes — so the matrices are real. What is missing is their
coupling to the spatial change: at each transition the resolution changes by 2x in both
axes while the channel count changes by 2x, and a `C x C` matrix alone accounts for
neither. A space-to-depth or depth-to-space rearrangement has to sit alongside it, and
the `_ds` / `_upsample` kernel variants are where that is written: `_ds` runs 528 mma
against `cc_dec_input_upsample_1024_512`'s 64, so most of what those kernels do is not
this matrix.
