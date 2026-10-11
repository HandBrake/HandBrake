# Phase 61 — the parameter count was the byte count halved

2026-09-16. The owner had read, perhaps from Hardware Unboxed, that DLSS 5 NR has about
twice the parameters this project published, and asked whether that was a crude estimate.
**It was right, and this project was wrong** — about the count, and in the same pass about
two more things in the published architecture.

## Measured

Summing the shapes in `work/mlxw/dlssnr-logical.safetensors` — the 649 tensors this
implementation actually computes with, which match MLX-DLSS's independent `weight_spec.json`
exactly:

| | tensors | parameters |
| --- | ---: | ---: |
| large matrices (`qkv`, projections, FFN expand/output, adapters) | | 143 046 313 |
| small tensors (attention biases, branch projections, cosine gates) | | 2 708 096 |
| `attn_scale`, FP32 | 70 | 714 |
| **total** | **649** | **145 755 123** |

## Why that is the real number and 73 841 889 is not

The DLL's weight section is **147 697 152 bytes**. Stored densely as FP16, 145.8 M
parameters would need 291.5 MB — twice the section. They fit because the large matrices are
**FP8 E4M3, one byte each**: MLX-DLSS's `unpack_qmma_e4m3_matrix` reads them as `uint8` QMMA
tiles and decodes them through an E4M3 table. One byte for the large matrices, two for the
small FP16 tensors and four for `attn_scale` predicts 148.47 MB against the section's
147.70 — **within 0.52 %**, the residue being tile scales and layout the estimate ignores.

**73 841 889 × 2 = 147 683 778**, which is the section to within 13 KB. The published count was
the container's `data_len / 2` from `src/tools/hnet_weights.py` — the dense-FP16 reading
this project had already withdrawn *as an encoding* on 2026-09-08, and never re-checked *as a
count*. `notes/CLAUDE.md` even dismissed the early "~148 M FP8" press figure as "the byte
count read as one byte per parameter". Most parameters really are one byte. The press was
right.

## What else fell with it

**Grouped-query attention, 4:1.** `docs/ARCHITECTURE.md` published `QKV = 1.5C²`. The logical
`qkv_weight` is `(C, 3C)` — full multi-head attention. `notes/HANDOFF.md` had withdrawn GQA
days earlier; the architecture document was written from the older brief and repeated it.
The "factor 2 uniform across all matrices" that HANDOFF attributed to "the packing" was
this same one-byte storage.

**27 % FP16 subnormals.** Published as "the trap that costs a quarter of the network". On
the real weights: **7 subnormal values in 145 754 409**, and 4.96 % exact zeros. E4M3's
smallest non-zero magnitude sits far above FP16's normal threshold, so decoded FP8 cannot
be subnormal. The 27 % was measured on the misread container bytes — `notes/INDEX.md`
already listed that decode as superseded; the number outlived it. The XMX flush itself is
real and the `2^k` rescale is a correct guard; it just has almost nothing to guard here.

**"No dequantisation step."** There is one: E4M3 to float, at extraction.

## Corrected

`README.md` and `docs/ARCHITECTURE.md` — the two published statements — and the weight
section and roadmap of `notes/CLAUDE.md`, with the history marked rather than erased.
`notes/MODEL-SPEC.txt` is left as it is and described correctly: a table of the container,
whose element counts are storage.

## The trap, once more

`notes/INDEX.md` says it in its own words: a withdrawn finding lives on wherever it was
written down. This one was withdrawn in the encoding and lived on in the count for eight
days, through a README, a published specification and a public repository. The check that
would have caught it is one line — sum the logical shapes — and nobody ran it, because the
number had been "verified" once.
