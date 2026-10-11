# MLX-DLSS unpacks our container into 649 logical tensors — and our FP16 premise was wrong
2026-09-09

`iamwavecut/MLX-DLSS` (owner's find) ships working extraction code for the same DLL we
have. Running it end to end:

```
extract_dlssnr_weights.py  ->  153 packed tensors, sourceSHA256 e16bcf15...  (our DLL)
unpack_dlssnr_weights.py   ->  649 logical tensors, format dlssnr-logical-v18
                               unsupported 0, opaque 0
```

Two independent confirmations of our own reader arrive for free: **153 tensors** and the
DLL hash.

## The correction that matters

`block0.layer0.input_adapter_weight` is `(16, 32)` — the 512-element region we located
at the front of block0 by size difference. Right place, right shape. **But the values do
not match our decode at all:**

```
                     shape      sd        vs our raw FP16 read of the same 512 elements
theirs (logical)    (16,32)   0.2489     correlation -0.0197, max |diff| 0.977
ours   (raw FP16)   512       0.0306
```

`notes/phase5-stem.md` argued the region could not be a 16-input projection *because*
its magnitude was wrong: "a projection with 16 inputs would be initialised around
1/sqrt(16) = 0.25; this region measures sd 0.031". The logical tensor measures
**0.2489**. The prediction was right and the raw read was what was wrong.

**So the container does not hold plain dense FP16.** It holds *packed backend payloads*
— permuted into `mma` fragment order and, per their notes, partly E4M3 — which the
unpacker decodes. The logical file is **1.974x** the packed file, which is our
"arena is 2x the container" rule seen from the other side, now with a cause.

This invalidates a foundational claim in notes/CLAUDE.md ("73,841,889 parameters stored
little-endian FP16... the weights are used exactly as stored"). `data_len == 2*n_elem`
held in 153 of 153 tensors, but that fixes the byte count, not the encoding.

It also explains, at a stroke, a long list of this project's dead ends: the stem failing
when wired, the `128C` values saturating the clamp, the Q/K/V split being unfindable by
any method, and the swizzled addresses.

## What the logical inventory answers

```
block0  (C=32)                          block5  (C=64)
  input_adapter_weight   (16, 32)         qkv_weight        (64, 192)
  qkv_weight             (32, 96)         attn_bias         (2, 64, 64)
  projection_weight      (32, 32)         attn_scale        (2,)
  attn_bias              (1, 64, 64)      attn_cos_skip     (64,)
  attn_scale             (1,)             ffn_cos_skip      (64,)
  attn_cos_skip          (32,)            ffn_expand_weight (2, 4, 2, 32, 32)
  ffn_cos_skip           (32,)            ffn_branch_projection_weight (2,4,32,32)
  weight1 (32,128) weight2 (128,32)       ffn_output_projection_weight (64,64)
```

- **`qkv_weight` is `(C, 3C)`: full multi-head attention, no GQA.** The narrow-block
  problem never existed — there was no fractional KV head to find. Three sessions of
  failed methods were looking for a split that is not there.
- **Two cosine gates per block**, `attn_cos_skip` and `ffn_cos_skip`, both length C —
  consistent with the gate-on-skip form we read out of the `mma` accumulator.
- `attn_bias` is `(H, 64, 64)` — our shape, confirmed.
- 27 tensor roles in total, including the ones notes/CLAUDE.md listed as named-but-unlocated:
  `out_conv_weight`, `out_gain`, `blend_scale`, `conv_weight`, `inp_merge_cos/sin`,
  `inp_upsample_sin`.

## What survives from our own work

Everything measured from the *kernels* rather than from the container bytes: the clamped
softmax and its exact constants, `attn_scale` being FP32 per head, the gate multiplying
the skip inside the `mma` accumulator, head_dim 32, the 16-channel input order
(colour at 4-6, history at 7-9), the five-texture input contract, and the 32->4 head.
Their notes agree with the channel order independently.

## Reproducing

```
git clone --depth 1 https://github.com/iamwavecut/MLX-DLSS work/mlx-dlss
T=work/mlx-dlss/python/mlxdlss/tools
PYTHONPATH=work/shim python3 $T/extract_dlssnr_weights.py work/dl/nvngx_dlssnr.dll work/mlxw/dlssnr-packed.safetensors
PYTHONPATH=work/shim python3 $T/unpack_dlssnr_weights.py work/mlxw/dlssnr-packed.safetensors work/mlxw/dlssnr-logical.safetensors
```

`work/shim/safetensors/` is a 60-line format-compatible reader/writer written because
this machine has no pip and installing the real package needs sudo. Round-trip verified.
