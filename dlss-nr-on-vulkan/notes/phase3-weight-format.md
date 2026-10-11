# Phase 3 — the weight blob is FP16, not FP8. VERIFIED 2026-09-07.

**This overturns the central premise of the project.** notes/CLAUDE.md was built on
"the model ships FP8 E4M3 weights", and everything about dequantisation,
losslessness and BF16 followed from that. It is wrong. Reader:
`src/tools/hnet_weights.py`.

## The container

`.rsrc` holds exactly two resources: `VERSION` (1 184 B) and one
**`RCDATA/WEIGHTS_HT`, 147 695 410 B**. "HT" for HNet tensors. Its format:

```
u64   total_size          == 147 695 410, exactly the resource size
repeat until EOF:
    u64   name_len
    char  name[name_len]           "block24.layer2.layer"
    u64   rec_len                  bytes from here+8 to record end
    u64   rec_len                  (repeated)
    u64   data_len                 == 2 * n_elem
    u32   dtype                    always 1
    u8    data[data_len]           little-endian FP16
    u64   0
    u64   1                        ndim
    u32   n_elem                   shape[0]
```

Sequentially walking this lands on byte 147 695 410 **exactly** — the format is
confirmed, not guessed. 153 tensors, **73 841 889 elements**, 147 683 778 bytes
of tensor data.

## Why FP16 and not FP8

1. **Arithmetic.** `data_len == 2 * n_elem` in **153 of 153** records, where
   `n_elem` is an explicit field in the container, not an inference from file size.
   One byte per element does not fit under any reading.
2. **Decode.** On a mid-size tensor:

   | reading | mean | sd | min | max |
   |---|---|---|---|---|
   | **FP16 little-endian** | +2.71e-06 | **1.96e-03** | -1.97e-02 | +2.17e-02 |
   | FP16 byte-swapped | -7.05e-06 | 1.84e-03 | -1.79e-02 | +1.78e-02 |
   | BF16 little-endian | +4.13e-20 | 2.94e-18 | -1.21e-16 | +2.50e-16 |
   | BF16 byte-swapped | -4.36e-22 | 8.75e-19 | -6.59e-17 | +6.03e-17 |

   BF16 is out by twenty orders of magnitude under either byte order.
3. **Byte order**, settled by an exact NaN/Inf census over all 73.8 M elements:
   little-endian FP16 yields **11** non-finite values (0.0000 %), byte-swapped
   yields **61 092** (0.0827 %). Little-endian, consistent with every other field.
4. **The entropy anomaly is explained.** `.rsrc` entropy 5.89 looked too low for
   dense FP8. It is exactly right for FP16: the high byte (sign + exponent) takes
   only 126–135 distinct values while the low byte (mantissa) spans all 256. That
   asymmetry *is* the FP16 signature, and it also confirms the byte order
   independently.

The reported "~148 M parameters at FP8" was almost certainly the file size divided
by one byte. It is **73 841 889 parameters at FP16** — same bytes, half the count.

## Consequences — all of them simplify the project

1. **There is no dequantisation step.** Phase 3's first task disappears. No E4M3
   decoder, no calibration, no error budget.
2. **Do not convert to BF16.** That was only ever a workaround for absent FP8
   hardware. FP16 → BF16 drops 3 of 10 mantissa bits and would be a real, avoidable
   precision loss.
3. **The primary GPU path is FP16, not BF16.** Config 1 of our cooperative matrix
   table is `fp16 × fp16 → fp32` (`notes/hw-coopmat.md`) — identical shape and
   accumulation to the bf16 config. The stored weights feed the XMX units with
   **zero conversion of any kind**.
4. The old invariant "any divergence is an implementation bug, never quantisation"
   survives, and is now stronger: the weights are used exactly as stored.

*(The reported "FP8 E4M3-specific kernels" seen in a runtime frame capture may still
be real — Blackwell could quantise on the fly at execution time. That is a statement
about NVIDIA's runtime, not about the stored format, and it does not affect us.)*

## Architecture: a symmetric U-Net of 71 blocks

Names are `block{0..70}.layer{N}.layer`, plus one `blend_scale` (a single scalar) in
block70. All 71 block ids present, none missing. Element counts per block:

```
block0-4        10 848 …  11 360     stem
block5-8        30 880 …  34 968
block9-14       98 592 … 114 968
block15-22     344 616 … 410 144
block23-30     984 096 … 1 246 248
block31-38   6 293 577 each          bottleneck, 8 identical blocks
block39        262 656               transition
block40-47     984 096
block48-55     410 392 … 344 616
block56-62     115 088 …  35 024
block63-66      30 880 …  11 392
block67-70      10 336 …  10 905     head
```

Perfectly symmetric about blocks 31–38. Sub-layer shapes line up with the RTTI
taxonomy in `notes/phase2-graph-rtti.md`:

- blocks 23–30 and 40–47 carry **4 sub-layers** — 262 144 / 131 584 / 458 784 /
  131 584 — the QKV / projection / FFN-expand / FFN-contract signature of a
  transformer block;
- blocks 31–38 carry **5** — 2 097 160 / 2 098 176 / 1 572 928 / **1** / 525 312;
- 46 blocks carry a single sub-layer.

Note 131 584 = 512 × 257 and 262 144 = 512 × 512, consistent with 512-wide
projections carrying a bias row. Exact 2-D shapes are not in the container — `ndim`
is 1 throughout, everything is flattened — so recovering them needs the `CCNetwork`
construction code in `.text`. **That is the next task.**

## Open

- 2-D shapes per tensor, and the block→layer-class mapping (which of the 71 blocks
  is Swin, which is ViT, which is the decoder upsample).
- 8 tensors read with sd > 0.5 while the other 145 sit near sd ≈ 1e-3: all are
  `block{31..38}.layer2`, the 1 572 928-element ones. Different role — scales,
  normalisation, or embeddings. Identify before building the reference.
