# block0 carries 512 elements no other block has — and reading them as the stem fails
2026-09-08

## The localisation (measured)

`block0.layer0.layer` is **10,848** elements. The other C=32 fused blocks — 1, 2, 3,
67, 68, 69 — are **10,336**. The surplus is exactly **512**.

It sits at the **front**, and that is established exactly rather than by correlation.
Every fused block ends with `[cosine gate][8 zeros]`; the gate begins 64 elements from
the end in block0 (10848 - 10784) and in block1 (10336 - 10272) alike. Identical from
the end, 512 different in total, so the surplus is leading. (A correlation test agreed
— 0.818 for `block0[512:]` against `block1` versus 0.662 unshifted — but that is the
weaker argument and is not what the claim rests on.)

It is a separate population, not part of the block:

```
                      sd        row-norm cv, reshaped (16,32)
block0[:512]        0.0306              0.154
block0[512:]        2.8575              -
control: 512 elements from inside block1   1.835
```

A hundredfold difference in scale, and uniform row norms where ordinary weights of the
same size give a 57x spread. `512 = 16 x 32`.

## The input contract (external, documented)

DLSS 5 Neural Rendering takes, per `jlrouzies-fr/DLSS5-Feeder`:

```
colour          the backbuffer                       3 ch
motion vectors  RG16F, in pixels                     2 ch
depth           R32F, raw hardware depth             1 ch
trust mask      R8, "bias-current-colour mask"       1 ch
```

evaluated as a **DLAA 1:1 contract with no jitter**, plus temporal state the network
carries itself. Seven external channels, and the binary corroborates it directly:
`cuda_capture_mv_dilate_kernel` is motion-vector dilation, `cuda_capture_output_
exposure_scale_kernel` and `cuda_capture_buffer_as_texture` are the rest of the capture
path. DX11/DX9/Vulkan titles reach it through a bridge onto a private D3D12 device —
the model itself only ever sees the D3D12 contract.

Seven external channels plus carried state fits 16 comfortably, which is why the
512 = 16 x 32 reading looked right.

## It does not work

Wired as a 16 -> 32 projection with colour in slots 0-2, zero motion, constant depth
and a fully-trusted mask:

```
                          denoise score   verdict
stand-in (RGB tiled)          0.05697     PASS-THROUGH
input_adapter 16 -> 32        2.41415     41.96x WORSE
```

Kept behind `--stem`; the stand-in remains the default.

One of the role, the orientation or the channel order is wrong, and there is a
principled reason to doubt the role rather than the details: **the magnitude is wrong
for a fan-in of 16.** A projection with 16 inputs would be initialised around
`1/sqrt(16) = 0.25`; this region measures sd 0.031, which corresponds to a fan-in near
1000. Ordinary weights elsewhere in the model sit at a median `|v|` of 4.6e-4, so the
region is 65x *larger* than the model's weights and 8x *smaller* than a 16-input
projection should be. It is neither.

Deliberately not pursued further by permutation. Trying channel orders until the score
improves is precisely how this project produced the shifted-window mask, the
leading-region projection and the ~450x factor, each of which had to be withdrawn. The
localisation is a result; the reading is not, and it is recorded as failed rather than
tuned into looking successful.

## What would decide it

The stem is a kernel, not just a tensor: `cc_tinlayout_fused_pre_block_swin_1h_32_1`
takes a **single pointer** (`param_0+0` only, the sole such kernel in all 231), which is
the signature of a pass that reads one input buffer and nothing else. Its `mma` shapes
and its one buffer's element stride give the input channel count directly, the same way
the bias table's 8192-byte stride gave 64x64. That is a measurement rather than a
guess, and it is the next thing to do.
