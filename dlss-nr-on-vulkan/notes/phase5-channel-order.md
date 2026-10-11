# The 16-channel packing order, read out of the pre-block
2026-09-08

## Where it is written

`cc_tinlayout_fused_pre_block_swin_1h_32_1` packs its assembled input vector with two
`st.shared.v4.b32`, four `b32` each, two FP16 per `b32` — **sixteen channels**, split
into two halves 1024 bytes apart in shared memory.

Reading them cost a wrong turn worth recording. The stores sit at instruction indices
126 and 140 while the texture reads are at 234-455, so a straight-line reading says the
stores cannot consume them. That is false: `%rs70` is defined at 247 and 440 and **used
at 123**, a use before both definitions. The block is a **loop body**, its store tail
laid out textually ahead of the sampling. Every reaching-definition analysis in this
project assumes straight-line code, and here that assumption breaks — the earlier
"textures reach `st.shared`" claim was produced by a forward walk with no program-order
check at all and is withdrawn; this one replaces it by reading the packing block
directly.

## The order

```
ch0,1,2     mul.ftz.f32 products of f32 terms
ch3         uniform (high half of %r83)
ch4,5,6     COLOUR  R,G,B      (x - a) * b
ch7,8,9     HISTORY R,G,B      (x - a) * b     -- the same a and b
ch10,11     uniform
ch12,13,14  selp.f32 between +1.0 and -1.0 under one predicate: validity flags
ch15        uniform (high half of %r84)
```

The two RGB triples are the solid part and they are attributed, not guessed:
`%rs2/%rs3/%rs4` come from the required colour texture (`STRUCT+0`), and
`%rs17/%rs19/%rs22` trace to `HISTORY.c0/c1/c2` — the five-tap texture at `STRUCT+8`.
Both triples pass through the identical affine normalisation, subtracting `%rs21` and
multiplying by `%rs23`, two per-frame scalars converted from f32 at the top of the
kernel. Current frame and reprojected history, normalised the same way.

`ch12,13,14` never carry sampled data at all — they are a `selp` between `+1.0` and
`-1.0` on a single predicate, i.e. sign-encoded validity, which is what the trust mask
becomes after thresholding.

## And it retracts "the leading 512 are the input adapter"

`notes/phase5-stem.md` proposed that block0's 512 surplus elements are a `16 x 32`
projection over the packed input. Wired with the *measured* channel order — colour in
4-6, the same frame as history in 7-9 because a still frame reprojects to itself, and
the validity flags at +1 — it scores **72.75x worse** against 1.00x for the stand-in,
worse than the earlier zero-filled attempt.

The reason is now visible and it is not the ordering. The pre-block reads roughly forty
512-byte weight tiles from the arena — about **10,240 elements**, essentially the whole
of block0's 10,848-element tensor — and runs **528 `mma`**. So the stem is not a small
matrix bolted onto the front of an ordinary block: **the pre-block is a complete block
that happens to take 16 channels in and produce the C=32 trunk.** The 512 surplus is a
part inside it, not a separable projection, and the `input_adapter` framing is
withdrawn.

`--stem` stays available and off by default.

## What is worth keeping

The channel order itself, and the two facts under it: the network is fed **current
colour and reprojected history as two identically-normalised RGB triples**, plus
sign-encoded validity. That is a real constraint on any correct reconstruction, and it
is measured. `src/ref/run_frame.py` builds the synthetic input in that order now, which
is right even though the projection it feeds is not.
