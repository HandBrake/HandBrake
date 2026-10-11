# Two results from one pip install: the port is exact, and the GPU speedup was not real
2026-09-09

The owner installed pip. Two things it bought, one very good and one that corrects a
headline number this project has been quoting all day.

```
python3 -m venv work/venv
work/venv/bin/pip install --index-url https://download.pytorch.org/whl/cpu torch
work/venv/bin/pip install numpy safetensors        # 983 MB total, work/ is gitignored
```

---

## 1. The port is bit-identical to the PyTorch original

Everything else here validates against *properties* (E4M3 fixed points, unit rows, a
permutation) or against a *specification* (their `weight_spec.json`). `torch` finally
allows the check that was missing: implementation against implementation, the same
operators and the same real weights, one written in numpy by us and one in PyTorch by
`iamwavecut/MLX-DLSS`.

`src/ref/test_against_torch.py`. Every line is **bit-identical**:

- **Precision primitives.** `e4m3` over four magnitude regimes — and this torch build
  has native `float8_e4m3fn`, so our bit arithmetic is being checked against a library
  float8 cast, not against the same trick reimplemented. Plus `quadratic_gate`, the
  activation, the bit-affine softmax, the fragment-tree cosine normalise and publish.
- **Layout.** Window origin for all 71 blocks, all 4096 entries of the attention-bias
  fragment order, which head counts use it, and the bias remap. `partition_windows`,
  `reverse_windows`, `average_pool2`, `pad_spatial_end`, `nearest_upsample2_crop`,
  `learned_upsample2`, `decoder_input_merge`.
- **All four block families on real weights**: `window_block` (block 1),
  `branched_window_block` (block 5), `split_window_block` (block 23), `global_block`
  (block 31), and the two feed-forward cores on their own.
- **The whole 71-block forward at 320x320.**

### What had to be controlled for

numpy's BLAS and torch's ATen reassociate a float32 dot product differently — measured
**2.5e-07 to 6.5e-07** on this graph's shapes, on every shape including K=32. The E4M3
publishes turn that into whole-quantum flips on about **0.0008 %** of cells, and one
flip is a 6.25 % jump. Feed the two implementations the *same* GEMM output and the
gate and publish are bit-identical; let them each call their own BLAS and a global
block drifts 4.9e-03. So the checks route our GEMM through torch, and what is left is
the port.

### The one real error it found

`vendor_cosine_normalize`'s non-32 path — dead in this graph, head_dim is 32 at every
width — did a half `sqrt` and then a half divide, which **rounds twice** where torch's
`rsqrt` rounds once. It drifted 1e-03. I nearly wrote this off as "half reduction
order"; it was not, and short 4- and 8-wide rows disagreed too, which is what exposed
it. Fixed, now bit-identical at every width. Also ported `learned_upsample2`, which was
missing entirely — it is unreachable in the recovered execution order, but it is part
of the operator set.

### And it confirms phase9 against an implementation we did not write

Letting each side use its own BLAS, the full graph diverges by **11.27 % of the head's
sd**. `notes/phase9-numerics.md` predicted that any correct float32 implementation
would land on the 9-12 % avalanche floor. It does.

---

## 2. The GPU speedup was measured against a crippled BLAS

**The system numpy is linked against the netlib reference BLAS at ~3 GFLOP/s. The pip
wheel bundles OpenBLAS and does 214 GFLOP/s — 67x.** Every "CPU baseline" in
`notes/phase8-xmx-graph.md` and `notes/phase11-what-is-left.md` used the slow one.

384x384 face crop, the same code:

| | CPU only | with XMX |
|---|---|---|
| system numpy (netlib reference) | 38.0 s | **17.0 s** |
| venv numpy (OpenBLAS) | **17.5 s** | 18.3 s |

So the honest statement is: **the fastest configuration on this machine is the CPU
alone with a proper BLAS, and our XMX path only matches it.** The 45.1 s -> 17.1 s and
"2.68x" quoted earlier are a GPU rescuing a bad CPU baseline, not a GPU win. Threads
are not the explanation either — single-threaded OpenBLAS is 17.7 s.

### Why, exactly

Per GEMM shape the graph issues, OpenBLAS against our XMX path and against the
cooperative-matrix kernel measured on its own:

| M x K x N | GFLOP | OpenBLAS | XMX path | kernel alone | |
|---|---|---|---|---|---|
| 147456 x 32 x 96 | 0.91 | **8.9 ms** | 45.4 ms | 1.24 ms | block 0 qkv |
| 147456 x 32 x 128 | 1.21 | **10.2 ms** | 41.0 ms | 1.41 ms | block 0 ffn |
| 36864 x 32 x 96 | 0.23 | **3.9 ms** | 16.0 ms | 0.26 ms | blocks 1-3 |
| 9216 x 64 x 192 | 0.23 | **1.3 ms** | 5.3 ms | 0.18 ms | blocks 5-7 |
| 2304 x 128 x 384 | 0.23 | **0.9 ms** | 3.4 ms | 0.18 ms | blocks 9-13 |
| 576 x 256 x 768 | 0.23 | **1.0 ms** | 2.5 ms | 0.17 ms | blocks 15-21 |
| 576 x 512 x 1536 | 0.91 | 12.0 ms | **4.3 ms** | 0.71 ms | blocks 23-30 |
| 36 x 1024 x 4096 | 0.30 | 2.2 ms | **1.3 ms** | 0.33 ms | blocks 31-38 |

**The kernel wins every row** — 0.17 to 1.4 ms against 0.9 to 12 ms. The *path* loses
most of them, at 3 to 45 ms, because a dispatch writes A as float16 and reads C back as
float32 across the host boundary. Wide, shallow GEMMs are the worst: block 0's
147456x32x128 is 1.2 GFLOP against a 75 MB result.

The discriminator is arithmetic intensity, `K*N/(K+N)` — multiply-accumulates per
element moved. Break-even against OpenBLAS sits between 192 and 384; against netlib
there is none, because that CPU is 20x slower and the GPU wins everywhere.

### What was changed

`nr_xmx` now routes on intensity as well as size, and `calibrate()` picks the threshold
from one 512-cube sgemm at install time: under 20 GFLOP/s means a reference BLAS and
everything above `MIN_MACS` goes to the GPU; above it means 256. Measured: the venv
picks 256, the system python picks 0, and each lands on the better of its two options
(18.3 s and 17.0 s). `NR_MIN_INTENSITY` overrides.

That recovers 21.7 s -> 18.3 s under OpenBLAS, but it does not make the GPU a win.
**It cannot**, while every activation round-trips through host memory.

### What this does to the roadmap

`notes/phase11-what-is-left.md` listed GPU residency first because 71 % of a frame is
elementwise numpy. That was right for the wrong reason. Residency is not an
optimisation on top of a working GPU path — it is the **precondition for the GPU path
being worth anything at all** on a machine with a normal BLAS. Until activations stay
in device buffers between blocks, the correct advice on this hardware is to run the CPU
reference under OpenBLAS.

The ceiling in phase11 is unaffected: the kernel is 1348 GFLOP/s, a 720p frame is
458.6 GFLOP, so 340 ms is still the floor and full-frame real-time is still 20x out of
reach.
