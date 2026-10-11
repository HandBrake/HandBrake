# The 128C region is not the relative-position bias I assumed
2026-09-08

Established facts about the `128C` region that follows the qkv in every Swin block
(one 64x64 table per head, `h = C/32` heads, 8x8 window = 64 tokens):

## What it is not

**Not a shifted-window mask.** An earlier test appeared to find one, peaking at
shift 4 with 0.75 agreement across seven blocks. That result was degenerate: with a
threshold of -20, *every* entry counted as masked, so the "agreement" was simply the
canonical mask's own density, and the peak at shift 4 only reflected that shift 4
gives the densest mask. Withdrawn.

**Not a 2-D relative-position bias.** Grouping the residual by 2-D relative position
(15x15 = 225 classes for an 8x8 window) leaves spreads *within* a class as large as
the total range — head 0: residual range 12.3, worst within-class spread 11.0. A
positional function would have near-zero within-class spread.

**Not row/column structured either.** Removing row and column means from head 0 leaves
sd 0.746 of the original 0.860 — 87 % of the variation survives.

## What it is

```
per-head constant offset   -56.19 .. -56.29 across all 16 heads   (essentially identical)
within-head variation      sd 0.86
```

Softmax is shift-invariant, so **the -56.23 constant has no effect whatsoever**; only
the 0.86 of variation can matter, and 0.86 nats is a ~2.4x modulation.

The raw storage is the surprise. Of 65,536 sixteen-bit words, only **321 distinct
values** occur, and almost all are `0xD3xx`:

```
0xD316 x3109   0xD318 x2946   0xD317 x2932   0xD315 x2908   0xD314 x2783 ...
```

Decoding `0xD316` as FP16: sign 1, exponent 20 (2^5), mantissa 790, giving
-(1 + 790/1024) x 32 = -56.69. **The sign and exponent are constant and only the low
mantissa bits vary** — roughly 8 bits of information per 16-bit element, not 16.

## What this means

The region carries real information (321 distinct values, sd 0.86 after the constant
is removed) but it is neither a mask nor a positional bias, and it is stored at half
the precision the container implies. Feeding it into a softmax as an additive term —
which is what `src/ref/hnet_ops.py` currently does — is a guess that the evidence no
longer supports.

It stays wired that way for now because it is dimensionally correct and the GPU/CPU
comparisons that rely on it compare like with like. But it should be understood as
unresolved, not as the "attention bias" the earlier notes called it.

## Also observed

`block40` and `block45` have visibly less extreme content than their neighbours
(74.8 % and 62.1 % below the range midpoint against 100 % elsewhere), which is at
least consistent with Swin alternating masked and unmasked blocks. Not pursued
further, since the mask reading itself did not survive.
