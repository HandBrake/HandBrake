# WITHDRAWN 2026-09-08 — the conclusion below is wrong
#
# The opcode census was right: there is no `ex2` in any attention kernel. The
# inference from it was wrong. The exponential is hand-rolled in `f16x2` out of
# fma/max/min plus a shift-and-add on the f16 bit pattern, and `ptx_trace.py` was
# dropping every statement inside a `{ ... }` scope, which is where all of it
# lives. The attention is an ordinary softmax over **clamped** logits.
#
# Superseded by notes/phase5-softmax-found.md. Kept for the record.

# DLSS-NR does not use softmax attention
2026-09-08 — found by mechanical PTX dataflow tracing

## The measurement

```
kernel                                     ex2   rcp   rsqrt   mma
cc_split_swin_16h_qkv_512                   0     4     32     224
cc_vit_attention                            0     2      0     128
cc_vit_1d_attention                         0     2      0     128
cc_tinlayout_fused_swin_1h_32_1             0     4     32     512
cc_tinlayout_fused_swin_4h_128_4            0     4     32     448
cc_tinlayout_fused_swin_8h_256_8            0     2     32     576
cc_tinlayout_fused_post_block_swin_1h_32    2    10     32     528
```

Across all 231 kernels: **ex2 x62** (all in output post-blocks), **lg2 x42**,
**rcp x512**, **rsqrt x3,968**, tanh x0.

**There is no exponent anywhere in the attention path.** A softmax needs one. What is
there instead is normalisation (`rsqrt`, consistent with the QK-normalisation already
established) and reciprocals — the shape of cosine-similarity or linear attention,
where the weights are normalised by a sum rather than exponentiated.

## Why this matters more than anything else found today

`src/ref/hnet_ops.py` and `src/ref/run_frame.py` apply a softmax in every attention
block. That is the core operation of 45+ of the 71 blocks, and it is the wrong
function. It directly explains why the reconstruction degrades the frame (1.43x worse
than the input) despite every structural piece around it checking out: the arithmetic
between the recovered matrices is not the arithmetic the network performs.

## How it was found, and why not sooner

By building `src/tools/ptx_trace.py` — a def/use dataflow walker — rather than reading
PTX by eye. Reading by eye had already produced three wrong conclusions in this
project (the "missing 2^8.5 factor", the shifted-window mask, the leading-region
projection). The tracer needed two fixes before it was trustworthy: vector
destinations in braces, and PTX splitting long instructions such as `mma` across
several lines. Until the second was fixed every `mma` parsed with no operands and the
dataflow stopped dead at the loads feeding it.

## What to do with it

Replace the softmax. The candidate form, consistent with `rsqrt` + `rcp` and with
QK-normalisation already confirmed:

```
q, k normalised to unit length per head        (rsqrt, established)
s = (q . k) * attn_scale[head]                 (attn_scale established)
w = s / sum(s)   or   w = s * rcp(sum(s))      (rcp, to be confirmed)
out = w @ v
```

The `128C` region's role should be revisited under this reading too: it was never
plausible as a softmax bias (near-constant -56.2, no positional structure), and a
different attention form may give it a natural place.
