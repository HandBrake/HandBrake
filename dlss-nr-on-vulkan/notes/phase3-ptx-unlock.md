# The PTX unlock — 2026-09-07

The single biggest step of the project, and it came from the owner's suggestion to
go look at what the AMD community had already done instead of grinding deeper into
the disassembler.

## Two AMD-side projects, very different value

- **`danielblnc/DLSS-NR-on-AMD`** — 816 stars, *no source code at all*: the repo holds
  only LICENSE, README and THIRD_PARTY. Releases ship a closed `dlssnr_on_amd_setup.exe`.
  Nothing to learn from. It does independently confirm the build we hold:
  it requires "`nvngx_dlssnr.dll` version **310.8.0.0**", exactly ours.
- **`skchen17/dlssnr-amd-lab`** — 1149 files of genuine, carefully-evidenced RE work.
  Their capture methodology needs an RTX 50 host and so does not transfer to us,
  but one of their scripts carries the decisive fact.

## The fact: the containers are Zstandard, not a proprietary codec

I had assumed the 15 fatbin containers in `.data` held NVIDIA-compressed SASS and
wrote the decompression off as too expensive. **Wrong.** Inside each `0xBA55ED50`
container is a plain **Zstandard frame**, and the payload is **PTX source**.

Method credit: `skchen17/dlssnr-amd-lab`, `scripts/extract_runtime_modules.py`.
Reimplemented independently against our own DLL as `src/tools/extract_modules.py`.
Python 3.14's stdlib `compression.zstd` is enough — nothing to install.

One trap: the container has padding after the frame, so the one-shot
`zstd.decompress()` reads the tail as a second frame and dies with
*"Unknown frame descriptor"*. Use the streaming `ZstdDecompressor` instead.

Result on our binary:

```
15 / 15 containers -> PTX, .target sm_120
231 entry points (223 unique names)
37,129,930 bytes of readable PTX
```

This exactly matches the entry count the AMD lab reported independently.

## What PTX hands us: the complete architecture, from NVIDIA's own templates

Each kernel declares shared memory under a mangled C++ symbol that carries the
**entire layer configuration as template parameters**. Demangled with `c++filt` (`notes/ptx-demangled.txt`, 304 symbols). The namespace is
`tin3_1` — the "tinlayout" of the kernel names.

The raw list is kept **deliberately**, as the evidence the analysis rests on. It is
symbol names — facts about a binary, regenerable in a minute by anyone holding the same
file with the commands above — not code and not a transcription of anything executable.
The worked form is `notes/ptx-kernel-configs.md`, which maps each kernel to its layer
class and template configuration, and `notes/MODEL-SPEC.txt`, the specification that came
out of it. Read those; this file is for checking them.

Example, from `cc_vit_1d_qkv`:

```
tin3_1::Conv1dQKVLayer<
    tin3_1::Conv1dQKVConfig<32, 32, 1024, 128, 32, 2, 2, 2, false, false, 128, ...>,
    tin3_1::LayerOutputOOB<3u, ...>,
    (tin3_1::DType)1>::forward_impl(...)
```

### Layer classes (count = distinct instantiations)

| class | n |
|---|---|
| `Conv2d1x1Layer` | 108 |
| `CrazyCuckooFusedSwin2d4HLayer` | 52 |
| `Conv1d1x1Layer` | 32 |
| `Attention2dLayer` | 24 |
| `CrazyCuckooFusedSwin2d2HLayer` | 20 |
| `Conv2dQKVLayer` | 12 |
| `Attention1dLayer` | 12 |
| `Conv1dQKVLayer` | 8 |
| `FusedSwin2d1HLayer` | 4 |

Config classes add `FusedSwin2dFfwdConfig` (24) and `FusedSwin2dQKVAttnConfig` (8).

**`CrazyCuckoo*` confirms the `crazy-cuckoo` tag found in `.rdata`** — the network
config name is literally baked into the layer template names.

### Channel wiring, read off `Conv2d1x1Config<in, out, H, W, ...>`

```
512  -> 512   tile 8x8
512  -> 1024  tile 4x4
1024 -> 512   tile 8x8
1024 -> 1024  tile 16x8
1024 -> 4096  tile 16x8     FFN expand, ratio 4
4096 -> 1024  tile 16x8     FFN contract
```

`Attention2dConfig<32, 32, 16, 8, 8, 8, 32, 2, ...>` carries an **8x8 window**,
matching the shifted-window Swin reading.

### DType settles the FP16/FP8 question for good

```
(tin3_1::DType)1  x152
(tin3_1::DType)2  x152
```

Exactly 152 instantiations each — every kernel exists in both. DType 1 is FP16 and
matches the `dtype = 1` field in every one of the 153 weight records; DType 2 is the
`_fp8` variant. **Stored weights are FP16; FP8 is a runtime execution choice.** Our
Phase 3 conclusion holds, now confirmed from NVIDIA's own type parameters.

## Consequence

The remaining blocker — which named parameter occupies which slice of a packed blob —
is no longer a statistics problem. The PTX shows each kernel's address arithmetic
against its 80-byte parameter struct directly. Read it out of the PTX.

## Config parameter order, decoded

`Conv2d1x1Config<OUT, IN, H, W, ...>` — **output channels first**. Cross-checked
three ways, all agreeing:

- `cc_vit_ffn_expand` = `<4096, 1024, 16, 8, ...>` and `cc_vit_ffn_contract` =
  `<1024, 4096, 16, 8, ...>` — expand must be 1024 -> 4096, so the 4096 is the output;
- `cc_dec_input_upsample_1024_512` = `<512, 1024, 4, 4, ...>` — the kernel name says
  1024 -> 512, the config lists 512 first;
- `cc_split_swin_16h_final_head_512` = `<1024, 512, 8, 8, ...>` — 512 -> 1024.

Other shapes that fall straight out:

```
CrazyCuckooFusedSwin2d2HConfig<64,  32, 4, 8, 8, 2, ...>   C=64,  head_dim=32, window 8x8, 2 heads
CrazyCuckooFusedSwin2d4HConfig<128, 32, 4, 8, 8, 4, ...>   C=128, 4 heads
CrazyCuckooFusedSwin2d4HConfig<256, 32, 4, 8, 8, 8, ...>   C=256, 8 heads
FusedSwin2d1HConfig<32, 4, 8, 8, 1, 1, 8, 8, ...>          C=32,  1 head
Attention2dConfig<32, 32, 16, 8, 8, 8, 32, 2, ...>         head_dim 32, grid 16x8, window 8x8
Attention1dConfig<32, 32, 128, 64, 32, 2, ...>             head_dim 32, seq 128, window 64
Conv1dQKVConfig<32, 32, 1024, 128, 32, 2, 2, 2, ...>       head_dim 32, C=1024, seq 128
Conv2dQKVConfig<32, 32, 1024, 16, 8, 32, 2, 2, 2, ...>     head_dim 32, C=1024, grid 16x8
```

**Head dimension is 32 everywhere**, confirming the earlier read from kernel names.
The `_upsample` variants promote the stage: `Swin2d2H(64)` -> `4H` with output 128 on
a 4x4 tile, `4H(128)` -> 256, `4H(256)` -> 512 — that is the encoder downsampling
ladder, expressed as an output-channel doubling with a quartered tile.

Full table: `notes/ptx-kernel-configs.md`.

## Confirmation from the kernel body

In `cc_vit_1d_qkv` the grid's z dimension strides the weight pointer by
`6144 * 512 = 3 145 728` bytes = **1 572 864 elements = exactly 1.5 C^2 at C = 1024** —
the size of one bottleneck QKV tensor, with `ctaid.z` selecting which of the eight
identical bottleneck blocks is being evaluated. The same prologue adds a bare `+128`
bytes, i.e. it steps over the 64-element header we had already isolated statistically.
Two independent methods, same answer.
