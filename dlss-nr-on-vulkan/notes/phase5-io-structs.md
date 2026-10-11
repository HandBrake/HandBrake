# The I/O kernels take descriptor tables, not packed tensors
2026-09-08

## Correcting an earlier reading

`notes/phase5-stem.md` proposed that the stem is a `16 x 32` projection over one packed
16-channel input image, and recorded that wiring it that way fails. The launch structs
say why, and it is not a channel-order problem.

`cc_tinlayout_fused_pre_block_swin_1h_32_1` was described in that note as "the sole
kernel of all 231 taking a single pointer". That is wrong in the way that matters: it
takes one *parameter*, and that parameter is a **pointer to a launch struct** which the
kernel dereferences —

```
mov.b64      %rd8, cc_tinlayout_fused_pre_block_swin_1h_32_1_param_0
ld.param.b64 %rd1, [%rd8]        ld.param.b64 %rd2, [%rd8+8]
ld.param.b64 %rd3, [%rd8+16]     ld.param.b64 %rd4, [%rd8+24]
ld.param.b64 %rd5, [%rd8+32]     ...
```

Far from having the fewest buffers, it has the most.

## The layout

```
kernel                         pointers                              pairs  scalars
pre_block_swin_1h_32_1         0,8,16,24,32 | 168,184,192,216,224      19       1
post_block_swin_1h_32          0,8,16,24,32,56 | 88,96,104,112,168     11       5
cg2r_post_process_kernel       0,32,64,...,256 | 296,304               28      13
```

`cg2r_post_process_kernel` is perfectly regular and settles the shape of the idiom:
nine pointers at **stride 32**, each followed by **exactly three `v2.b32` pairs**. That
is `8 bytes of pointer + 24 bytes of descriptor = 32` per entry — a table of **nine
buffer descriptors**, then thirteen loose scalars. It reads through `tex.2d` (x11) and
does pure f32 arithmetic with `ex2` (x42) and `lg2` (x34), i.e. `pow`: this is the
colour/tone stage, and the thirteen scalars are where the Structure and Tone Intensity
controls live.

The pre-block opens with **five consecutive pointers** at stride 8, followed by 16
descriptor pairs. Five input buffers, and the documented contract has exactly five
things to supply: colour, motion vectors, depth, trust mask, and the temporal state the
network carries between frames (`notes/phase5-stem.md`).

## What follows for the stem

**The inputs are five separate tensors with independent descriptors, not one packed
16-channel image.** Each carries its own dimensions and strides, and they are combined
inside the kernel. So `block0`'s dense leading 512 elements were never going to be a
`16 x 32` matrix over a packed input, and permuting channel order would not have helped
— the object being fed did not exist in that form.

This also sharpens the contrast already recorded against the output head: the head is a
**padded** `(64, 8)` holding a real `32 x 4` (`notes/phase5-output-head.md`), while the
stem's 512 contain no zero at all. Different kinds of object, and now with a reason:
the head projects one tensor to one tensor; the stem consumes five.

## Next

Read the pre-block's five descriptors — each is three `v2.b32` pairs, and the pairs are
dimensions and strides, the same quantity that gave the bias table its `64 x 64` and the
per-head scale its FP32 stride. That yields the channel count of each input directly
rather than by arithmetic fit, and it is the measurement the stem needs.
