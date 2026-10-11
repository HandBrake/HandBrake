# Phase 28 — one submission, reusable commands, bounded frame storage

2026-09-10. Phase27 specialized the shaders. This change removes the remaining
host work between blocks without changing their arithmetic.

## Change

`ResidentFrame.run` used 73 command buffers/fence waits per frame, copied five
encoder skips on the CPU, and converted the bottleneck's FP16/FP32 boundary on the
CPU twice per global block. It now records five device copies and sixteen device
conversions, captures one command buffer, and replays it on subsequent inputs.
The graph has 1804 recorded passes instead of 1783; the CPU submits it once.

The runtime allocates a separate command buffer after capture so ordinary recording
(e.g. temporal history) cannot overwrite a saved graph. Command buffers omit
`ONE_TIME_SUBMIT`; submission is synchronous, and transfer/compute barriers cover
skip reads, writes and reuse. Relevant Vulkan contracts:
[command buffer flags](https://docs.vulkan.org/refpages/latest/refpages/source/VkCommandBufferUsageFlagBits.html),
[copy requirements](https://docs.vulkan.org/refpages/latest/refpages/source/vkCmdCopyBuffer.html),
[synchronization examples](https://docs.vulkan.org/guide/latest/synchronization_examples.html).

Graphs are cached by specialization mask, bound to a frame's stable buffers, and
freed before those buffers. Recording failures can be aborted without discarding
completed graphs. The runtime is still a synchronous, single-threaded singleton;
this does not add concurrent inference or dynamic shapes to a captured graph.

Execution modes (`NR_FRAME_MODE`, or `run(..., execution=...)`):

- `block`: original CPU copies and 73 submissions, for exact comparison.
- `single`: device copies and one submission, recorded anew per frame.
- `replay` (default): capture once, then update only input data and submit.

Per-stage capture and timing use `block` because their host reads require fences.
Phase27's specialization controls still work independently.

## Paired measurements

Real input: `pngs/Cyberpunk-2077_01.jpg`, resized to each output size. All modes use
identical allocations and specialized shaders; order alternates each round.
Warm medians include input upload, head readout and Python graph overhead, but not
feature assembly, composition or image I/O.

| Output | Rounds | Block, ms | Single, ms | Replay, ms | Time reduction vs block |
|---|---:|---:|---:|---:|---:|
| 384x384 | 3 | 123.50 | 102.37 | 91.03 | 26.3% |
| 1280x720 (network 1280x768) | 6 | 541.34 | 516.14 | 510.57 | 5.7% |
| 1920x1080 (network 1920x1088) | 4 | 1138.96 | 1111.50 | 1102.67 | 3.2% |

These are incremental improvements over phase27, not a comparison with the old
generic shaders. Between-process placement/clock variation remains; don't subtract
unpaired historical measurements. Large frames remain dominated by GPU execution.
The phase25 fixed-cost fit predates specialization and replay and is not current.

The benchmark reports first runs separately. Its initial block run allocates/uploads
all weights and warms pipelines; later single/replay warmups share this work. Those
three warmup figures are therefore not comparable cold-start measurements.

Every output was bit-identical across modes, including a changed frame index. The
384x384 head SHA-256 is still the phase27/original shader value:
`8c4f8a43bf38e358c13e8016b7a6bcae035258dd4c87bc7c8337a1920539ab7f`.

```sh
python3 src/bench/frame_replay.py --size 720 1280 --pairs 6 --input pngs/Cyberpunk-2077_01.jpg --json work/perf28/720.json
```

Local raw evidence: `work/perf28/{384,720,1080}.json`, `tests.log`,
`frame-execution.log`, `temporal.log`, `clean-build.log`.

## Other completed fixes

- `ResidentBackend` retains one network extent by default. Changing resolution closes
  the old frame before allocating another, preventing accumulation of entire graphs
  and their GPU allocations. `max_cached_frames=N` explicitly opts into a larger LRU
  cache. An evicted frame cannot be reused directly; obtain it from the backend again.
- `image_io` now removes temporary RGB files/directories on both success and failure.
- `.gitignore` anchors `/ref/` to the repository root, exposing the previously hidden
  `src/ref/` source. These files still need inclusion in the next commit, like the other
  new source in this worktree. DLLs/weights/screenshots stay ignored.
- `make` creates its output directory. `notes/reproduce.md` records the tested upstream
  revisions, build requirements and weight setup instead of relying on an undocumented
  `work/shim` directory.

## Validation

All commands in `make test` passed, with local model weights present. New tests cover
transfer/compute barriers, invalid copy ranges, changed replay inputs, capture slot
reuse, cold full-frame capture, all execution modes, 73-to-1 recording count,
resolution eviction, generic/specialized graph coexistence, and recovery after an
injected recording error. CPU graph reference checks remain unchanged and pass;
optional PyTorch checks remain unavailable in the system environment.

A three-frame resident temporal pan rendered and saved successfully: reprojection
error 0, history alpha 0.0083 on frame 0 and approximately 0.695 thereafter. GPU history
recording between captured frames therefore works in the real temporal pipeline too.
All runtime, layer and shaders also built in a fresh temporary output directory using
the documented local headers, with no prebuilt `.so` or `.spv` files.

## Remaining work

Next performance work should remeasure shader costs after specialization and replay,
then test GEMM tile shapes/fusion against that profile. Old register-spill findings
are not sufficient grounds to reject all new variants. An OpenCL/DPAS backend remains
an independent research project, not a prerequisite established by these results.

Photo mode still needs a useful in-game face scene; the existing Proton capture is
only a warning screen. Exact parity with NVIDIA's original inference also lacks an
activation oracle. Neither is established by equality with our previous GPU path.
The Mesa cooperative-matrix epilogue issue has a workaround/reproducer but has not
been reported upstream. This turn sends no external reports.
