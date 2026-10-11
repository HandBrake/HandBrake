# Phase 23 — integer weights: the weights would survive, the activations would not

2026-09-09. Asked whether the weights could be re-encoded to an integer format. The
XMX units do expose it — config 4 of the cooperative-matrix table is
`sint8 x sint8 -> sint32` at **M=8 N=16 K=32**, twice the K depth of the fp16 config,
so twice the arithmetic per instruction and half the weight bytes.

Three measurements answer it, and they answer it no. Not for the reason expected.

## The weights themselves hold up

`src/bench/int_weights.py` quantises all 256 GEMM weight matrices symmetrically,
dequantises them straight back to fp16 and runs the real graph. The gates,
`attn_scale`, the attention bias and the transition sines are uploaded as float32 and
are left alone — a few thousand structural values.

| weights | relative error | head correlation vs fp16 |
|---|---|---|
| int8, per output channel | 7.56e-03 | 0.9922 |
| int8, per tensor | 1.57e-02 | 0.9931 |
| int6, per output channel | 3.18e-02 | 0.9875 |
| int5, per output channel | 6.59e-02 | **−0.3134** |
| int4, per output channel | 1.39e-01 | 0.9779 |

int5 collapsing to *anti-correlated* while int4 comes back at 0.978 is the chaotic
graph showing itself again — a hundred E4M3 publishes with a 6.25 % step stand between
input and output, and one of them flipping takes the frame with it. It is a reminder
that these correlations are not a smooth quality dial.

On a real 720p frame (`src/bench/int_render.py`), int8 per-channel weights give a
picture that still has pores and lashes and does not look broken — but:

```
fp16 against int8 weights:  mean|d| 0.00860   correlation 0.996561
the effect itself:          mean|d| 0.02538
int8's disagreement is 34 % of what the network does
```

A third of the effect is not a rounding difference. It would end this project's
standing claim that the output is unchanged through every optimisation.

## But the weights cannot go alone

Config 4 is `sint8 x sint8 -> sint32`. **Both** operands are integer; there is no mixed
mode on this hardware. Integer weights force integer activations.

## And the activations are where it dies

`src/bench/int_activations.py` measures the published tensors the graph actually
carries between blocks:

| published | \|max\| | dynamic range | int8 rounds to zero | relative error on the rest |
|---|---|---|---|---|
| l1 | 352 | **180 000x** | **21.5 %** | **32.0 %** |
| l1_in | 22 | 11 264x | 14.7 % | 25.6 % |
| l2 | 448 | 229 376x | 4.2 % | 10.1 % |
| l3 | 448 | 229 376x | 2.1 % | 6.0 % |
| l5 | 448 | 229 376x | 0.4 % | 1.6 % |

E4M3 spends four of its eight bits on the exponent and holds **6.25 % relative
precision across seventeen binades**. int8 lays an absolute grid over one. On the
widest, shallowest level — the one carrying the fine detail this network exists to
synthesise — a per-tensor int8 grid rounds **a fifth of the non-zero values away
entirely** and leaves 32 % relative error on what is left.

This is not a precision downgrade, it is a different *kind* of quantiser, and the
vendor's choice of a floating 8-bit format over an integer one was evidently not
arbitrary. Block-wise scaling (llama.cpp's `Q4_K` shape, one scale per 32 values) would
fix the range problem — but that is not what the XMX integer path consumes; it would
mean dequantising per block into fp16 and arriving back where we started, with extra
work in front.

## And it would buy nothing anyway

Even granting the representation, the arithmetic:

- **Weight traffic is 1.15 GB of the frame's 31 GB.** Halving it is 1.9 % of the total.
- **The GEMMs run at 4.4 % of the fp16 peak.** Doubling the peak changes nothing that
  is not already idle; the frame is latency-bound in both halves — measured twice in
  `notes/phase22-staging-and-storage.md`, once by cutting traffic a third and once by
  staging the operands, both for no change in frame time.

So: a good question with a clean answer. The weights would survive integer encoding on
their own; the hardware will not let them go on their own; the activations will not
survive; and the speedup it was meant to buy is in the half of the machine that is not
the bottleneck.
