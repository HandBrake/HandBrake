# First numerics on real weights — 2026-09-07

`src/ref/hnet_ref.py`. The first code in this project that loads NVIDIA's actual
parameters and computes something verifiable with them.

## 1. Confirmed shape bindings

Matching config-derived sizes against the 153 container tensors gives exact hits:

| container tensors | elements | kernel / shape |
|---|---|---|
| `block{23..30,40..47}.layer0` (16) | 262 144 | `Conv2d1x1<512,512>` — `cc_split_swin_16h_proj_512` |
| `block39.layer0` | 262 656 | `512x512 + 512` bias |
| `block{31..38}.layer4` (8) | 525 312 | `512x1024 + 1024` (`final_head` / `dec_input_upsample` shape) |
| `block{31..38}.layer2` (8) | 1 572 928 | `1.5 C^2 + 64` at C=1024 — the ViT-1D QKV |
| `block{31..38}.layer1` (8) | 2 098 176 | `2 C^2 + C` at C=1024 |

That is 41 of 153 tensors bound to a kernel shape from NVIDIA's own template
parameters. The rest are the fused Swin blocks, whose budgets still need modelling.

## 2. The fused Swin block slices at 4C^2 and 5C^2

For `cc_tinlayout_fused_swin_4h_128_4` (C = 128, blob = 98 592 elements), the only
`add.s64` immediates that land inside the blob and are not tile strides are

```
131 072 bytes =  65 536 elements = 4 C^2
163 840 bytes =  81 920 elements = 5 C^2
```

against a total of `6 C^2 + 2C + 32`. So the packing is

```
[ 0    .. 4C^2 ]   65 536 elements
[ 4C^2 .. 5C^2 ]   16 384 elements
[ 5C^2 .. end  ]   16 672 elements  = C^2 + 2C + 32
```

The smaller immediates (512, 1024, 2048, 4096, 8192, 16384, 24576 bytes) are all
exact multiples of C elements — tile strides, with a recurring `+512`-byte pad.

## 3. The 2-D layout is real, not an artefact

Reshaping `block23.layer0` to 512x512 and comparing against the same values randomly
permuted:

```
as stored : row-norm cv = 1.1176   col-norm cv = 0.3333
shuffled  : row-norm cv = 0.1712   col-norm cv = 0.1645
```

Stored row norms vary **7x more than chance**, and rows and columns are strongly
asymmetric where the shuffle is symmetric. A wrong reshape could not produce that.
The matrix has effective rank 246 of 512 at 99 % of energy, condition number 1.4e4 —
an ordinary trained weight matrix.

## 4. The numeric contract for Phase 4, measured rather than assumed

Running `y = x @ W` on that real matrix with FP16 operands, against a float64
reference:

```
output range          : +-0.8994
fp32 accumulate error : 8.610e-07   (9.57e-07 relative)
fp16 accumulate error : 2.443e-04   (2.72e-04 relative)
```

**FP32 accumulation is 284x more accurate on this layer's real weights.** That settles
the Phase 4 choice with a number: cooperative matrix **config 1, `fp16 x fp16 -> fp32`**
(`notes/hw-coopmat.md`). The bf16 configs are not merely unnecessary — accumulating in
the narrow type would cost nearly three orders of magnitude of accuracy, and this
network is 71 blocks deep.
