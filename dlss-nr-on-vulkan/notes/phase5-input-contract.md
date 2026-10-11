# The real input contract, read out of the pre-block kernel
2026-09-08

The external documentation says DLSS-NR takes colour + motion vectors + depth + a trust
mask. The binary is more specific, and the difference matters.

`cc_tinlayout_fused_pre_block_swin_1h_32_1` dereferences its launch struct and takes
**five 2D texture objects**, read with `tex.2d.v4.f32.f32` — not a packed tensor, not
linear buffers:

```
slot        taps  components used   consumers                        null-guarded
STRUCT+0      1        3 of 4       cvt.rn.f16.f32 x3                NO  -- required
STRUCT+8      5        3 of 4       fma.rn.ftz.f32 x12, mul x3       yes
STRUCT+16     1        2 of 4       fma.rn.ftz.f32 x2                yes
STRUCT+24     5        1 of 4       setp.leu/geu x5 each, selp x4    yes
STRUCT+32     1        2 of 4       mul.ftz.f32 x2                   yes
```

Two facts fall straight out.

**Colour is the only required input.** `setp.eq.b64 %rdN, 0` guards slots 8, 16, 24 and
32 and *not* slot 0. Every other input is optional, which is exactly why the community
bridges can drive this feature in games that supply no motion vectors at all.

**The channel counts are measured, not inferred**: 3, 3, 2, 1, 2 — eleven components in
total. Slot 0 is three components converted straight to FP16 (colour). Slot 24 is a
single component that only ever reaches `setp`/`selp` — a validity test, i.e. the trust
mask. Slots 8 and 24 take **five taps each**, the signature of a neighbourhood gather;
slot 8's three components through twelve `fma`s is the shape of a warp or filter, which
fits carried temporal colour rather than a fresh input.

## What this settles about the stem

`notes/phase5-stem.md` recorded that block0's dense leading 512 elements failed when
wired as a `16 x 32` projection, and `notes/phase5-io-structs.md` concluded from the
pointer layout that "the inputs are five separate tensors, so a packed 16-channel image
never existed". The first half stands; the second was too strong.

The five textures **are** gathered into one vector inside the kernel before the
projection. Eleven consumed components padded to sixteen and projected to C=32 is
`16 x 32 = 512`, which is exactly the size of the region. So the shape was probably
right and the **content** was wrong: the test fed colour into slots 0-2 and zeros
everywhere else, while slot 8 carries what looks like temporal colour — for a still
frame that should be the frame itself, not zero, under any channel ordering.

That is a real, testable correction rather than a permutation search. But the ordering
inside the sixteen is still unmeasured, so it stays unwired until the gather order is
read out of the kernel — which is tractable: each texture's components reach the
`mma` A operand through a known chain, and the order they are packed in is the order
they are written to shared.

## Also worth carrying

The pre-block does the projection itself — 528 `mma`, weights via `STRUCT+224` at
512-byte tiles — and writes eight `st.global.L1::no_allocate.b128`. So the "stem" is a
whole kernel, not a matrix bolted onto block0, and its weights live in the arena like
any other layer's.
