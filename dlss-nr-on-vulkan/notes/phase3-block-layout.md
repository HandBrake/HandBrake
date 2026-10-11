# Internal layout of a fused Swin block — solved 2026-09-07

The last structural unknown. Solved by combining three independent signals: constant
offsets in the PTX, a boundary scan of the standard deviation, and a classification of
parameter *values* into three kinds.

## The three kinds of value, and how they identify themselves

| class | signature | what it is |
|---|---|---|
| weights | sd 0.001 – 0.06, mean ~ 0 | ordinary trained matrices |
| log-bias | mean ~ **-7**, sd 1 – 6, min to -14 | additive attention bias; `exp(-7)` is ~1e-3 and the tail underflows |
| gate | values in **[0, 1]**, mean 0.5 – 0.75, sd 0.3 – 0.5 | `attn_cos_skip` / `ffn_cos_skip` |
| scale | sd 40 – 400, isolated short runs | per-channel scales |

## The boundary, located exactly

Scanning the standard deviation across candidate boundaries `4C^2 + xC`:

```
C=64    4C^2+64C = 20,480    sd before 0.017   after 2.91    ratio  169x
C=128   4C^2+64C = 73,728    sd before 0.0073  after 3.17    ratio  433x
C=256   4C^2+64C = 278,528   sd before 0.0033  after 3.19    ratio  978x
```

Neighbouring candidates (`+0C`, `+32C`, `+96C`, `+128C`) all give ratios near 1.0.
The weight region ends at **`4C^2 + 64C`**, and the attention bias that follows is
**`128C` = heads x 64 x 64** — one 64x64 table per head for the 8x8 window.

## The universal form

```
fused Swin block = 4C^2 + 64C  |  128C  |  q*(C^2/2)  |  r*(C^2)  |  2C + c
                   weights        bias     weights      transition   gates
```

- `q` = 0 at C = 32, 1 at C = 64, 128, 256
- `r` = 1 for the eight stage-transition blocks (4, 8, 14, 22, 48, 56, 62, 66), else 0
- `c` is a small residue: 32 normally, 40 at C = 256, and 544 / 280 / 144 / 80 / 64
  for blocks 0, 48, 56, 62, 66

**This fits 45 of the 46 fused blocks exactly**, with zero remainder. The one
exception is `block39`, which is not a fused block at all — it is the
`dec_input_upsample`, `Conv2d1x1<512,512> + 512` = 262 656, bound separately.

## Where the gates sit

The `[0,1]` gate runs appear *inside* the first weight region and again at the very
end — at C=256 a 1.5C run at offset 704C and a 1C run at 1345C; at C=128 a 2C run at
384C. Two gates per block, matching the two named parameters `attn_cos_skip` and
`ffn_cos_skip`.

## Status

`notes/MODEL-SPEC.txt` now prints the ordered internal layout for every one of the
71 blocks, and still balances to all 73 841 889 parameters. Regenerate with
`src/tools/model_spec.py`; it exits non-zero if the accounting ever fails to close.
