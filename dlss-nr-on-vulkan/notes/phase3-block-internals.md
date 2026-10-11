# Internal layout of the remaining block types — 2026-09-08

Derived inline after both background agents died on a session rate limit. Method is
the same one that worked before: classify regions by the value signature (weights /
log-bias / cosine gate / scale) and check the result across several widths.

## ViT-1D bottleneck (block31..38, C = 1024, 32 heads, stage `vit_1d`, slots 57–98)

```
layer0   2C^2 + 8      2,097,160   all weights, 1024 x 2048          + 8 unexplained
layer1   2C^2 + C      2,098,176   weights 2048 x 1024 | C cosine gate
layer2   64 + 1.5C^2   1,572,928   32 pairs (attn_scale) | GQA qkv
layer3   1             1           a single scalar
layer4   C^2/2 + C     525,312     weights 1024 x 512   | C cosine gate
```

Gates measured: `layer1` tail 0.535–1.000 mean 0.864; `layer4` tail 0.518–1.000 mean
0.874. Both sit in [-1, 1] with the maximum at exactly 1.0 — cosines, as in the
512-wide stage. **`cos_skip` is universal in this architecture.**

`layer2`'s 64-element *header* is 32 pairs, and the odd slots are the per-head
attention scale: 32 values for 32 heads, range 1.72–2.18. Same construction as the
16-head stage, except there the pairs sit at the *tail*.

**There is no attention bias table here.** `1.5C^2 + 64` leaves no room for one:
a `128C` region would require 1,704,000 elements against an actual 1,572,928. That
is correct rather than surprising — ViT-1D attends globally over 128 tokens, so
there is no window to position-bias. Only the Swin stages carry the `128C` table.

`layer3`, one scalar per block: `0.109, -0.248, 5.0e-05, -0.0104, 0.186, 0.041,
0.082, -558.5`. Ordinary magnitudes for blocks 31–37 and a wild **-558.5** for
block38, the last bottleneck block. Unexplained; flagged rather than guessed at.

## Fused Swin blocks — the leading region resolves

Probing the leading `4C^2 + 64C` for cosine-gate runs splits it identically at every
width:

```
C=64   [ 3.5  C^2 ] [ gate 1.5C  ] [ 1.477 C^2 ]      total 5.00 C^2
C=128  [ 3.0  C^2 ] [ gate 0.5C  ] [ 1.496 C^2 ]      total 4.50 C^2
C=256  [ 2.75 C^2 ] [ gate 1.125C] [ 1.496 C^2 ]      total 4.25 C^2
```

The trailing ~1.5C^2 is the **GQA qkv**, sitting immediately before the `128C`
attention bias — the same ordering as the split-Swin-16H stage, where `layer2` is
also `qkv | bias`. The leading part is exactly

```
X = 4C^2 + 64C - 1.5C^2 = 2.5C^2 + 64C
```

which reproduces 3.5 / 3.0 / 2.75 C^2 at C = 64 / 128 / 256 with no residual.

So a fused Swin block reads, in order:

```
[ 2.5C^2 + 64C ] [ cos gate ] [ 1.5C^2 qkv ] [ 128C attn bias ]
                                             [ q*C^2/2 ] [ r*C^2 ] [ 2C + c ]
```

The `64C` is worth noting: 64 is exactly the token count of an 8x8 window, so a
`(64, C)` per-window-position table is the natural reading — a learned positional
embedding. **Inferred, not measured**; its position inside the `2.5C^2 + 64C` run is
not yet pinned.

## Status

Every block type now has an ordered layout. What is still missing is not structure
but two scalars' worth of semantics: the ~450x branch/skip magnitude factor (see
`notes/phase3-first-operator.md` — it lives in the launch parameter block and is not
recoverable from PTX alone), the `+8` in ViT-1D `layer0`, and `layer3`'s scalar.

---

## Fused Swin layout, anchored on the gate (2026-09-08)

The earlier reading put the cosine gate *inside* what I was calling `wq` — caught by a
simple sanity check: the extracted `wq` had `|max|` of exactly 1.0000 at three
different widths, which is the cosine signature, not a weight. Re-anchoring on the
gate gives a layout that is regular at every width:

```
[ pre weights ][ 8 zeros ][ C-element cosine gate ][ 1.5C^2 qkv ][ 128C bias ][ tail ]
```

Measured, all four widths:

```
C     pad   gate   qkv       bias      tail
32     8     32     1,536     4,096       568
64     8     64     6,144     8,192     2,136
128    8    128    24,576    16,384     8,344
256    8    256    98,304    32,768    33,056
```

**The gate is exactly C long and the pad is exactly 8 zeros, at every width.** After
the fix `wq` has `|max|` 0.04–0.10 — ordinary weights, no contamination.

Deriving the bias position *from the gate* rather than from a boundary formula is what
made this exact. The formula `4C^2 + 64C` is right for C >= 64 and wrong by 0.5C^2 at
C = 32, and neither the standard-deviation scan nor the mean scan resolves the
boundary to the element; the gate does, because its values identify it unambiguously.

`src/ref/hnet_model.py` now splits Q/K/V only where GQA 4:1 divides exactly (C >= 128)
and otherwise exposes the qkv region whole, rather than inventing shapes for the two
narrow stages.
