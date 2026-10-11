# End-to-end pass through all 71 blocks, CPU and XMX in agreement
2026-09-08

`src/ref/forward.py`. A tensor is carried through every block in the execution order
recovered in `notes/phase3-execution-order.md`, on the CPU and again on the Xe2 XMX
units, and the two outputs are compared.

## Geometry, derived rather than assumed

`Attention1dConfig<32, 32, 128, 64, ...>` gives the bottleneck a 128-token sequence and
`Conv2d1x1Config<1024, 1024, 16, 8, ...>` makes that a 16x8 grid. Doubling upward
through the four encoder stages fixes the whole schedule:

```
C=32    512 x 256        C=512    32 x 16
C=64    256 x 128        C=1024   16 x 8   (bottleneck, 128 tokens)
C=128   128 x 64         then mirrored back up to 512 x 256
C=256    64 x 32
```

Every tensor shape chains correctly from input to output with no adjustment, which is
itself a check on the channel schedule, the block order and the window partitioning.

## Result

```
input   (512, 256, 32)
output  (512, 256, 32)   finite, sd 0.0107, range [-0.149, +0.145]
blocks with attention applied 44   passed through 27
matmuls 148
```

CPU against XMX over 4,194,304 output elements:

```
max |cpu - gpu|   1.438e-07
relative          9.676e-07
bit-identical     6.04 % of elements
```

Six percent identical and the rest differing in the last bits is the expected
signature of a different summation order in FP32. **The GPU path reproduces the CPU
reference end to end to one part in a million.**

## What this does and does not prove

It proves: the block order, the channel and resolution schedules, the window
partition/reverse, the attention path including grouped-query attention,
QK-normalisation and the per-head scale, the cosine-gate mixing, and the whole XMX
pipeline — all of it chains and agrees with the CPU across 71 blocks.

It does not prove the numbers are DLSS-NR's. Twenty-seven blocks pass through
untouched (the C=32 and C=64 fused blocks, whose qkv split is undetermined, plus the
ViT-1D, upsample and output-head paths not yet wired), the leading `2.5C^2 + 64C`
region of every fused block is unapplied because its sub-roles are unassigned, and
the runtime scale factor is still missing. This is the plumbing, verified.

## On the timing

The run takes 11.5 s on the CPU and 13.1 s on XMX, and **that comparison is
meaningless**: each of the 148 matmuls spawns a subprocess that builds a full Vulkan
instance, device and pipeline, writes its operands to files and tears everything down
again — roughly 80 ms of fixed cost per call, which is most of the 13.1 s. Measuring
XMX throughput needs a resident context that uploads the weights once and dispatches
repeatedly. That is worth building, but it is a separate piece of work and no
performance claim should be made until it exists.
