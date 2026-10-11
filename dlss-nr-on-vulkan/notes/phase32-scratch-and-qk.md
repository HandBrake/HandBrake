# Phase 32 — shared scratch, fused Q/K and game-launch preparation

*Numbered 32 rather than 30: this and `phase30-control-atlas.md` were written in
parallel by two workers on copies of the same tree, and both claimed 30.*

2026-09-10, Intel Arc 140V. Both optimizations are now enabled by default. The
71-block graph, resolution and numerical publication rules are unchanged.

## Memory

Previously every block shape retained its own intermediate allocations. Blocks
execute sequentially, so their scratch can share storage. `ScratchArena` plans the
maximum byte count for each role before command recording, freezes that plan, then
materializes only roles actually used. Captured addresses therefore stay stable.
The frame owns the arena; graphs are destroyed before buffers on close/eviction.

Within each block, additional roles have disjoint lifetimes and share allocations:

| Allocation | Roles used at different times |
|---|---|
| projection | hidden16, proj, attended, attention, transition padded/projected |
| branch/value | branch, v16 |
| input/key | value16, win16, ffn16, k16, transition pooled/projected16 |
| query/probability | heads16, core16, q16, probs16, merged16 |
| scores/context | scores, context, transition upsampled |
| residual | ffn, transition scaled |

Global value/output and the frame's input, output, level values and encoder skips
remain separate. In particular, FFN residuals must survive until the final attention
residual. Existing barriers separate every reuse; independent dispatch groups write
different allocations. New scratch roles or larger extents after sealing raise an
error rather than invalidating captured pointers. Unused plain-block head/core
buffers are also omitted even with the arena disabled.

At **1280x720**, live native resident buffers decrease from **5,286,202,792 bytes to
2,415,205,800 bytes**, or **5041.3 to 2303.3 MiB: 54.3% less**. This measures buffer
sizes, including resident weights, not total process RAM or all driver allocations.
Layouts run sequentially and release all resident buffers between runs. Their heads
are bit-identical. Sequential timing differences are not an isolated speed claim.

At **1920x1080**, the compact layout uses **4,792,606,120 bytes / 4570.6 MiB**.
Three warm frames took 1084.8, 1110.4 and 1088.7 ms. The head SHA-256 matches the
previous GPU baseline:
`f658b55765c83c88a047ad83e6ee1cff97e7656bc50f5d18f5aa86e81c871837`.
An earlier uncompact 1080p run caused substantial memory pressure and swap activity;
its unstable 2–3-second timings are excluded from speed comparisons.

## Q/K fusion

Q and K were split into FP16 buffers, then read again for cosine normalization.
`record_qkv` can now normalize directly from the projected QKV layout, retaining
exactly the split's half-rounding point and the previous reduction/publication order.
V still uses its separate split. For token counts divisible by 32, a workgroup stays
inside one head, allowing address division to be hoisted out of the gather loop.
The general gather handles other token counts.

This removes **140 dispatches: 1804 → 1664 per frame**, still one captured submission.
The graph cache key includes the fusion switch, so toggling it selects the correct
recorded commands. The attention shader also fixes partial-workgroup behavior:
all lanes participate in gather/scatter; only valid rows perform normalization.
Previously an early return could omit data needed by a valid row.

Eight paired rounds at 720p, with the compact arena and the same allocations:

| Q/K path | Median graph time |
|---|---:|
| Separate split/normalize | 508.85 ms |
| Fused | 490.14 ms |

**3.7% less frame time**, with bit-identical heads, including changed input. The 720p
head hash remains `9df02c102cead1dcfcc8c6678f731467eac93c8a27dd648d2b72ca07c18471bc`.
These measurements were on battery and exclude image I/O, feature assembly and
composition. Do not subtract absolute timings from earlier AC-powered runs.
This is approximately **2 graph frames/second**, far short of the requested 20 FPS
(50 ms including surrounding work). No blocks or resolution were removed to inflate
that figure.

## Layer and daemon

The daemon rejects invalid dimensions before receiving large bodies, limits socket
inactivity, keeps serving after failed requests, creates dump directories, and treats
dump errors as nonfatal. It refuses to replace a live daemon or a non-socket path.
Unsupported bounded pixel formats pass through unchanged. The native exchange uses
`MSG_NOSIGNAL`, retries EINTR and sets read/write timeouts, so an early daemon close
does not kill the game with SIGPIPE. A failed exchange retains the original frame.

DOA5's installed `game.exe` is **32-bit PE and imports D3D9**, correcting the previous
D3D11 assumption. Both 32- and 64-bit Vulkan layer libraries/manifests are prepared
for Proton. `library_arch` in manifest version 1.2.1 lets the loader choose the ABI;
see the [Vulkan loader interface specification](https://github.com/KhronosGroup/Vulkan-Loader/blob/main/docs/LoaderLayerInterface.md).
Both native loader harnesses create a Vulkan instance with the layer and enumerate
this GPU. This proves library loading, not yet DOA5 rendering.

The launcher searches Proton and prefixes in all Steam libraries, preserving spaces,
validates the executable and supports `--check-proton` without launching a game.
`NR_PROTON` selects a specific runtime. Steam launch-option paths are shell-escaped.
The real DOA5/Proton paths pass the check. See `notes/phase34-doa5.md` for the remaining
interactive validation; no game was launched during this phase.

## Reproduction and validation

```sh
make test test-proton
python3 src/bench/qkv_fusion.py --size 720 1280 --input INPUT.png --pairs 8 --json work/qk.json
python3 src/bench/scratch_memory.py --size 720 1280 --input INPUT.png --json work/memory.json
python3 src/bench/scratch_memory.py --size 1080 1920 --input INPUT.png --compact-only --repeats 3
```

`NR_FUSE_QK=0` disables fusion; `NR_SCRATCH_ARENA=0` restores separate allocations
for newly created frames. The defaults are both `1`. `NR_FRAME_MODE` and
`XMX_SPECIALIZE` keep their existing meanings.

`make test test-proton` passes with local logical weights: 32 Q/K layout cases,
partial-row CPU references, FP16/FP32 output and guards; exact compact/separate graph
comparisons at two extents, changed inputs, both fusion settings and all execution
modes; buffer cleanup/frozen plans; existing reference/temporal/replay tests; daemon
header/codec/alpha/disconnect tests; launcher paths and both Vulkan loader ABIs.

Local raw evidence: `work/perf30/tests.log`, `fusion-compact-720.json`,
`memory-aliased-720.json`, `memory-aliased-1080.json`, with corresponding logs.
Earlier `fusion-1080.json` is explicitly not valid speed evidence because of memory
pressure. `fusion-hoisted-720.json` is the same-buffer comparison before compaction.
