# Phase 3 — architecture recovered from strings, RTTI and code. VERIFIED 2026-09-07.

Sources: `.rdata` literals (`notes/rdata-strings.txt`, 1684 entries), MSVC RTTI,
and targeted `objdump` cross-references. No CUDA toolkit was needed at any point.

## Names and provenance

- Feature codename **CG2R**; inference engine **HNet**.
- Source tree: `.../NGX/snippets/rel_310_8/source/features/dlssnr/`, with
  `cg2r_network_manager.cpp` and `cc_network.cpp` named in log strings.
- Two network config / backbone tags live in `.rdata`: **`crazy-cuckoo`** and
  **`hnet-vigilant-squid`**. The manager resolves a preset to a descriptor
  (`CG2RFindWeightByPreset`, `ResolvePresetToDescriptor`, `BuildActiveNetwork`) and
  a factory claims it by `arch_variant` tag via a dispatch table in `cc_network.cpp`.
- ATen operator names appear verbatim — `convolution`, `linalg_vector_norm`,
  `constant_pad_nd`, `clamp_min`, `ones_like`, `reciprocal`, `rsub`, `log10`,
  `stack`, `detach`, `alias`. **The model was exported from PyTorch.**
- `sm120` confirms Blackwell. The DLL also probes `wine_get_version`.

## The tensors are packed parameter blobs — this is why sizes never factored

Container names are `block{N}.layer{M}.layer`. The `.layer` suffix is not a matrix:
it is **all of that layer's parameters concatenated into one flat FP16 array**. That
is why 458 784 = 32 x 243 x 59 and 344 616 = 8 x 3 x 83 x 173 refused to decompose
into `in x out` — they are sums of several arrays, not products.

Confirmed by the code at `0x180032a20`: the four layer factories at
`0x180032ae9 / 0x180033638 / 0x180033c70 / 0x1800343dc` emit a *list of parameter
names* (the immediates 0xc/0xa/0xa/0x9/0x11 are string lengths, matching
`ffn_cos_skip`=12, `qkv_weight`=10, `attn_scale`=10, `attn_bias`=9,
`projection_weight`=17). The one exception in the whole container is
`block70.layer0.blend_scale`, a single scalar stored under its own name.

## Parameter inventory (contiguous table at VA 0x1800b2000-0x1800b21d0)

| group | parameters |
|---|---|
| attention | `qkv_weight`, `attn_scale`, `attn_bias`, `projection_weight`, `attn_cos_skip` |
| feed-forward | `ffn_cos_skip`, `weight0`, `weight1`, `weight2` |
| convolution | `conv_weight`, `dw_weight` (depthwise), `sin` |
| stem | `input_adapter_weight`, `layer0.dw_weight`, `layer0.conv_weight`, `layer0.sin` |
| upsample | `inp_upsample_input_scale`, `inp_upsample_sin` |
| output | `out_gain`, `out_conv_weight`, `blend_scale` |

`attn_cos_skip` / `ffn_cos_skip` and the `sin` parameters point at sinusoidal
conditioning — consistent with the published one-step-diffusion description.

## Channel widths — read straight off the kernel names

| stage | kernel | heads | channels |
|---|---|---|---|
| 1 | `cc_tinlayout_fused_swin_1h_32_1` | 1 | **32** |
| 2 | `cc_tinlayout_fused_swin_2h_64_2` | 2 | **64** |
| 3 | `cc_tinlayout_fused_swin_4h_128_4` | 4 | **128** |
| 4 | `cc_tinlayout_fused_swin_8h_256_8` | 8 | **256** |
| 5 | `cc_split_swin_16h_*_512` | 16 | **512** |
| bottleneck | `cc_vit_1d_*` | — | 1024 (from `cc_dec_input_upsample_1024_512`) |

**32 channels per head throughout.** The decoder input upsample is 1024 -> 512.

## Kernel inventory (300 names)

Stage kernels: `cc_tinlayout_fused_pre_block_swin_1h_32_1`,
`cc_tinlayout_fused_swin_{1h_32_1,2h_64_2,4h_128_4,8h_256_8}`,
`cc_split_swin_16h_{qkv,proj,proj_pool,ffwd,ffwd_proj,final_head}_512`,
`cc_vit_{qkv,attention,projection,ffn_expand,ffn_contract}`,
`cc_vit_1d_{...}` plus `cc_vit_1d_repack_{2d_to_1d,1d_to_2d}`,
`cc_dec_input_upsample_1024_512`,
`cc_tinlayout_fused_post_block_swin_1h_32{,_simple_blend,_control_mask,_rgb}`.

Modifiers, orthogonal to the above: `_shifted` (**shifted-window MSA confirmed**),
`_ds` (downsample), `_upsample`, `_inpview` / `_outview`, `_tilesync`, `_wait`,
`_chained`, `_publish`, `_full_rect`, `_fp8`.

Block-level tags also name conv blocks absent from the RTTI: `BSFusedConvBlock`,
`BSGroupedConvBlock`, `FusedSubtiledConvBlock`, `BSDownsample`, `SubtiledDownsample`,
`BSUpsampleSkip`, `SubtiledUpsample`, `SubtiledSkipBlend`, `OBSwinAttention`.

## FP8 reconciled

Every compute kernel ships in two variants, plain and `_fp8`. The **stored** weights
are FP16 (`notes/phase3-weight-format.md`); the `_fp8` kernels presumably quantise at
runtime on Blackwell. Both facts are true and they do not conflict. It changes
nothing for us: we consume the stored FP16 directly.

## Block-to-stage map (from container sizes + the width schedule)

```
block0-4      10 848 …  11 360   stage 1, 32 ch      stem + pre-block
block5-8      30 880 …  34 968   stage 2, 64 ch
block9-14     98 592 … 114 968   stage 3, 128 ch
block15-22   344 616 … 410 144   stage 4, 256 ch
block23-30   984 096 …           stage 5, 512 ch     4 sub-layers each
block31-38  6 293 577 each       ViT-1D bottleneck   5 sub-layers each
block39      262 656             transition / dec input upsample 1024->512
block40-47   984 096             decoder 512
block48-55   410 392 … 344 616   decoder 256
block56-62   115 088 …  98 592   decoder 128
block63-66    30 880 …  11 392   decoder 64
block67-70    10 336 …  10 905   decoder 32 + head + blend_scale
```

`CCVit1DBlock expects five layer descriptors` (error string at `0x18004377c`)
matches blocks 31-38 carrying exactly 5 sub-layers. Strong confirmation.

## Runtime contract, as logged by the DLL

- `CreateFeature(output %ux%u, network %ux%u, preset=%d)` — internal network
  resolution is separate from output resolution, as the tiling plan assumed.
- `EvaluateFeature Color=%p MVec=%p Depth=%p Output=%p intensity=%.2f reset=%d`
  — the four I/O surfaces.
- `enabled=%d localTone=%.3f localStructure=%.3f intensity=%.3f` — **the
  artistic-direction values from NVIDIA's research page, verified.**
- `dlssnr_prev_output` is **RGBA16F** — the carried temporal state, FP16 again.
- `DLSSNR.MVecScaleX` / `MVecScaleY` — motion-vector scaling parameters.

## Parameter budgets solved exactly for the two largest families

Anchoring the element counts to the channel widths read off the kernel names gives
closed forms that reproduce the container **exactly**, with no residual:

**Blocks 23-29 and 40-47 — C = 512, four sub-layers. Exact for all 15 blocks:**

```
layer0 = C^2            = 262 144     projection-shaped, square, no bias
layer1 = C^2/2 + C      = 131 584     C -> C/2 with a bias row
layer2 = 1.75 C^2 + 32  = 458 784
layer3 = C^2/2 + C      = 131 584     same shape as layer1
                 total  = 3.75 C^2 + 2C + 32 = 984 096
```

**Blocks 31-38 — C = 1024, five sub-layers. Exact for all 8 blocks:**

```
layer0 = 2 C^2 + 8      = 2 097 160
layer1 = 2 C^2 + C      = 2 098 176
layer2 = 1.5 C^2 + 64   = 1 572 928
layer3 = 1                            a lone scalar
layer4 = C^2/2 + C      =   525 312
                 total  = 6 C^2 + 2C + 73 = 6 293 577
```

C = 1024 here is independently corroborated by `cc_dec_input_upsample_1024_512`.

**Transitions, also exact at C = 1024:**

```
block30.layer4 = C^2/4 + 8    = 262 152
block39.layer0 = C^2/4 + C/2  = 262 656    the decoder input upsample 1024 -> 512
```

**Single-layer fused Swin blocks** all carry the same additive term `2C + 32`, at
every width — 96 at C=32, 160 at C=64, 288 at C=128, 544 at C=256 — with C^2
coefficients of 10.0 / 10.5 / 11.0 (C=32), 7.5 / 8.5 (C=64), 6.0 / 7.0 (C=128) and
5.25 / 6.25 (C=256). The coefficient does not follow a single quadratic across
widths, which is expected: a Swin relative-position-bias table scales with heads and
window size, not with C^2. The `+1.0` step between the two coefficients at each of
C=64/128/256 is one extra C^2-sized matrix — the downsampling variant of the block.

## Internal packing — partially solved empirically

The blob is FP16 throughout, so parameter boundaries show up as abrupt changes in the
value distribution. Segmenting by rolling standard deviation gives hard answers:

**1. `... + C` means a trailing bias vector, not a leading one.** For every
`C^2/2 + C` tensor tested (10 of them, at C=512 and C=1024, blocks 23/24/31/32/40),
the **last** C elements have a standard deviation 100-400x larger than the body,
while the first C match the body exactly. Layout is `[matrix][bias]`.
Control: `block23.layer0`, which is exactly `C^2` with no additive term, is uniform
end to end — as a bare matrix should be.

**2. `... + 64` in `1.5C^2 + 64` is a 64-element header at the *front*.** Its standard
deviation exceeds the body's by a factor of 170 000 - 260 000, consistently across all
eight bottleneck blocks. Decoded, it is **32 interleaved pairs** — and C/32 = 32 heads:

```
block31: -0.0078  +1.898  +512  +1.853  -0  +1.926  -2  +1.889  -0.0078  +1.882 ...
          ^even: {0, +-2^-7, +-2^1, +-2^9}      ^odd: 1.72 .. 2.08, tight cluster
```

Odd slots look like a per-head scale; even slots take only exact powers of two, which
reads as an exponent or shift selector. **One pair per attention head.**

**3. A `0.25 C^2` region of masking values.** In `block23.layer2` the tail after the
`1.5 C^2` weight matrix is 65 568 elements whose 1st-to-99th percentile spans just
-56.84 to -53.28 — pinned near **-56.5**, with rare outliers to -24 080 and +1 791.
In FP16 `exp(-56.5)` underflows to zero, so this is an additive **attention bias /
mask table**, not weights. Note 65 536 = 0.25 C^2 at C=512 = 64 x 64 x 16 — a 64-token
window (8x8) masked across 16 configurations, consistent with shifted-window MSA.
*(Shape reading is a hypothesis; the masking role is not.)*

**4. Parameters bounded in [0, 1] with maximum exactly 1.0** appear as short runs
(2C, 4C) in the small blocks — the signature of `attn_cos_skip` / `ffn_cos_skip`.

**5. `block70` ends with zero-filled runs** (3C and 120 elements) — padding.

## What is still missing

> **Partly superseded.** Items 1 and 3 below were answered later and this section was not
> updated — exactly the failure `notes/INDEX.md` warns about, a withdrawn finding living
> on where it was written down. The named, shaped logical extraction settles the blob
> slicing (item 1), and `notes/MODEL-SPEC.txt` records the 8x8 window (item 3); only the
> shift offset is still inferred. `docs/ARCHITECTURE.md` is the current statement.


1. **Which named parameter occupies which slice of each packed blob.** The totals are
   now pinned exactly, and the names are known, but the assignment and the internal
   order are not. Needs the layer `Init`/load path that slices the blob — *not* the
   four functions at `0x180032ae9` etc., which turned out to be plain
   `std::vector<std::string>` name-list builders (their immediates 0x10/0x1000/0x27
   are MSVC SSO and allocator thresholds, not dimensions).
2. Which of `crazy-cuckoo` / `hnet-vigilant-squid` this weight blob is, and whether
   both configs are present.
3. Exact Swin window size and shift.
