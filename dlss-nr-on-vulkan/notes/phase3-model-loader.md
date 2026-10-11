# The model is materialised — 2026-09-08

`src/ref/hnet_model.py`. The artifact the rest of the project builds on: it turns
147 MB of opaque FP16 into **71 blocks of named, correctly-shaped tensors**, and it
validates itself. Every slice must consume exactly its region; a leftover or an
overrun raises. If it exits 0, the recovered layout is internally consistent for all
153 records.

```
parameters placed 73,841,889 of 73,841,889   -> ALL ACCOUNTED
```

## Block types

```
fused_swin       x45     encoder/decoder Swin stages, C = 32/64/128/256
split_swin_16h   x16     the 512-wide stage (block30 carries a 5th sub-layer)
vit_1d           x8      the 1-D bottleneck, C = 1024
dec_upsample     x1      block39, 512x512 + 512 bias
output_head      x1      block70, weights + blend_scale
```

Example of what comes out:

```
block23  split_swin_16h  C=512   proj(512,512) ffwd(512,256) ffwd_gate(512,)
                                 ffwd_proj(512,256) ffwd_proj_gate(512,)
                                 wq(512,512) wk(512,128) wv(512,128)
                                 attn_bias(16,64,64) attn_scale(16,)

block31  vit_1d          C=1024  ffn_expand(1024,2048) ffn_contract(2048,1024)
                                 ffn_gate(1024,) wq(1024,1024) wk(1024,256)
                                 wv(1024,256) proj(1024,512) proj_gate(1024,)
                                 attn_scale(32,) scalar(1,)
```

## Two things the loader taught us

**Gates cannot be found by spread.** At C=32 the weights themselves have sd 0.065 and
lie inside [-1,1], so they are indistinguishable from cosines by range and spread
alone. The discriminator is the **mean**: weights sit at ~0, every measured gate at
0.63–0.99. And spread fails in the other direction too — gates in the deeper decoder
blocks are squeezed against 1.0 with sd as low as 0.012 (`block43.layer3`: min 0.911,
mean 0.994). Those blocks pass their input through almost unchanged, which is a real
architectural observation, not noise.

**Dispatch by size, not by sub-layer count.** `block30` has five sub-layers but is a
split-Swin block with an extra one, not a ViT-1D block. Counting sub-layers puts it in
the wrong family; its `layer0` size settles it immediately.

## What is still unidentified

The 512 parameters in the **even** slots of the `2H` attention-scale runs (16 per
split-Swin block, 32 per ViT-1D block). They are kept under `unknown_even` so the
accounting closes rather than being quietly dropped. Their values are erratic —
tiny numbers mixed with -24080, +1791, +512 — and no reading has fit them yet.

Also open: the `+8` of `vit_1d` `ffn_expand`, `block30`'s `extra`, the per-block
`scalar`, and the fused blocks at C=32 where no gate run appears in the leading
region at all (`gate(0,)`), consistent with `q = 0` in the size formula.
