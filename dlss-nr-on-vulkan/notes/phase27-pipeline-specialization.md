# Phase 27 — specialize the shader before declaring a register ceiling

2026-09-10. The user asked for better performance. The change is enabled by default
in the existing Vulkan resident path; no weights, network operations, precision,
resolution or block layout changed.

## What changed

The shaders used push-constant flags to choose their operation, transpose, operand
widths and output publish. Those choices were invisible when Mesa compiled the
pipeline. A GEMM that only needs one epilogue still carried code for all of them.

`specialize.glsl` adds one specialization constant for these flags. `libxmx.c`
creates a pipeline on its first use and caches it by shader family and complete flag
word. The graph uses 34 variants at 720p, reused across dimensions and frames. The
cache is bounded at 256 entries; further variants use the generic pipeline.
Dimensions, strides, offsets, pointers and scalar controls stay dynamic.

This uses the normal [Vulkan specialization mechanism](https://docs.vulkan.org/samples/latest/samples/performance/specialization_constants/README.html):
the driver sees the choices during compilation and can remove unused branches.
The untouched cooperative-matrix store through shared memory remains in place, as
do all half/E4M3 rounding points and synchronization barriers.

`XMX_SPECIALIZE=7` is the default: bit 1 for GEMM, 2 for elementwise operations,
4 for the attention reductions. `XMX_SPECIALIZE=0` selects the generic baseline.
`Runtime.specialize(mask)` switches between them on the same buffers, between
command buffers, so placement differences cannot masquerade as an improvement.

## Measurements

`src/bench/specialization.py` assembles real features, warms both variants, then
alternates the order of measurement. Each output is checked against the generic
head with exact array equality. A second frame index also checks that the cached
pipelines consume changing inputs.

The input for the final runs was `pngs/Cyberpunk-2077_01.jpg`, resized to each
requested extent. These times are for `ResidentFrame.run`, including input/output
access and dispatch recording; image decoding, feature assembly and RGB composition
are outside the timed region. They are warm timings, not startup latency.

| Output / network extent | Generic median | Specialized median | Time reduction |
|---|---:|---:|---:|
| 384x384 / 384x384, 3 rounds | 125.69 ms | 114.49 ms | 8.9% |
| 1280x720 / 1280x768, process 1, 6 rounds | 670.45 ms | 549.72 ms | 18.0% |
| 1280x720 / 1280x768, process 2, 6 rounds | 613.70 ms | 535.78 ms | 12.7% |
| 1920x1080 / 1920x1088, 4 rounds | 1456.95 ms | 1179.29 ms | 19.1% |

The between-process spread documented in HANDOFF is still visible. Quote the
paired **13–18% reduction at 720p**, not a single guaranteed speedup. The two 720p
processes produced identical head hashes for both frame indices.
The old 2.9 s 1080p record was not reproduced in this warmed, paired protocol;
count only the reduction from the simultaneously measured 1.46 s baseline.

Raw JSON and compiler output are local artifacts in `work/perf27/`. An independent
384x384 run using saved original SPIR-V files with `--shader-dir work/perf27`
matched both head hashes from the new shaders (frame index 0 and 1). This checks
that the generic fallback itself has not changed relative to the pre-change code.

## What the compiler says

With `MESA_SHADER_CACHE_DISABLE=true INTEL_DEBUG=cs`, the generic 16x32 GEMM still
reports **15:15 spills:fills**, 1195 instructions and 128 GRF. All **33 specialized
variants** compiled for the 384 frame report **0:0 spills:fills**. They still use
SIMD32 and 128 GRF. The register count did not grow; removing runtime alternatives
let the compiler fit the live state.

This withdraws phase26's conclusion that only a second OpenCL backend could improve
performance. The old spill counts were real, but measured the universal shaders,
not an immutable hardware ceiling. OpenCL remains a separate research direction;
this change does not establish a new peak-throughput limit or promise real-time NR.

## What was tried and left out

Specializing dimensions and pitches as well increased the 720p cache from 34 to 274
pipelines. In one paired run, generic / flags / flags-and-shapes were **609 / 540 /
524 ms**. The extra ~3% over flags alone cost a **3.64 s** first shape-specialized
forward, versus 1.03 s for the first flags-only forward in that experiment. This
was not kept: photo mode should not pay hundreds of compiles for that small gain.
Experimental sources and binaries are saved under `work/perf27/dimensions/`.

Even the retained flags-only variants have a first-compilation cost. In the 384
compiler-dump run, with the disk cache explicitly disabled, the first generic
forward was 553 ms and the first specialized one was 1151 ms. Warm timings above
exclude those setup passes; the daemon reuses the pipelines thereafter.

## Verification and reproduction

`make test` passes: fused epilogues; the new specialization regression; resident
operators, layouts, attention, block families and the complete graph; CPU graph,
controls, temporal primitives and HDR codec. The optional torch accelerator is
skipped by the system Python, as before. Full resident vs CPU correlation is
0.981311 on the existing synthetic test, unchanged.

The new GEMM tests cover the fallback 8x16, tiled 16x32 and staged 64x32 paths,
all five epilogues, both output widths, transpose, batches, non-dense pitches and
nonzero offsets. Guard regions must remain untouched. They compare the specialized
output bit-for-bit with the generic GPU output and independently check a plain
GEMM against numpy. Test operands exclude FP16 subnormals because XMX flushes those
to zero; this is the existing hardware contract, not a relaxed tolerance.

```
make
make test
python3 src/bench/specialization.py --size 720 1280 --masks 0,7 --pairs 6 \
    --input pngs/Cyberpunk-2077_01.jpg --json work/specialization-720.json
python3 src/ref/nr_frame.py IN.png OUT.png --resident
XMX_SPECIALIZE=0 python3 src/ref/nr_frame.py IN.png BASELINE.png --resident
```
