# Shared memory on Xe2: a driver quirk, and a cap that cost the staged GEMM half its threads

2026-09-24. The owner asked why window attention has exactly 2 KB of shared memory and what
1 KB or 512 B would do. Answering it turned up two separate things, and **only the second
cost this project anything**:

- **the quirk** — Mesa sizes a core's shared-memory partition from the bytes a shader
  declares, not from what each workgroup is given, so some *smaller* declarations run on
  fewer threads. Real, verified three ways below, and worth nothing measurable in this
  frame: the one kernel it touches, the base GEMM at 512 B, is no faster padded to 1 KB;
- **the cap** — 128 KB of shared memory between a core's workgroups, each share rounded up.
  Ordinary occupancy arithmetic, the same on any driver, and it had the staged GEMM — 41 %
  of a 720p frame — on half its threads. Fixed: 10 % of the frame.

## The quirk — verified, in the source, in the driver's own output, and on the hardware

Mesa 26.2.2, `src/intel/vulkan/genX_shader.c:1183`, fills two fields for every compute
pipeline from the same number, `total_shared` — the bytes the shader declares:

- `SharedLocalMemorySize`, what each workgroup is given: the declaration rounded up to the
  Xe2 allocation table — **1 KB at least**, then 2, 4, 8, 16, 24, 32, 48, 64 ... KB
  (`intel_compute_slm_calculate_size`);
- `PreferredSLMAllocationSize`, the shared-memory partition of each core: *workgroups a
  core's threads can hold* x **the declared bytes**, rounded up to 16, 32, 64, 96, 128 ... KB
  and capped — at 128 KB on this machine, measured below
  (`intel_compute_preferred_slm_calc_encode_size`, `src/intel/common/intel_compute_slm.c`).

So the partition is sized from the declaration and filled with the rounded allocation.
Whenever the two differ, fewer workgroups fit than the threads allow. For a 32-lane
workgroup — 64 a core by threads — resident = min(64, partition / allocation):

| declared | allocation | partition | resident | predicted ms | measured ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| 0 | — | — | 64 | 11 | 11.0 |
| 64 B | 1 KB | 16 KB | 16 | 44 | 41.5 |
| 128 B | 1 KB | 16 KB | 16 | 44 | 41.3 |
| 256 B | 1 KB | 16 KB | 16 | 44 | 41.3 |
| 384 B | 1 KB | 32 KB | 32 | 22 | 21.1 |
| 512 B | 1 KB | 32 KB | 32 | 22 | 21.1 |
| 768 B | 1 KB | 64 KB | 64 | 11 | 11.0 |
| 1 KB | 1 KB | 64 KB | 64 | 11 | 11.0 |
| **1.25 KB** | 2 KB | 96 KB | **48** | 14.7 | **14.2** |
| **1.5 KB** | 2 KB | 96 KB | **48** | 14.7 | **14.2** |
| 1.75 KB | 2 KB | 128 KB | 64 | 11 | 11.0 |
| 2 KB | 2 KB | 128 KB | 64 | 11 | 11.0 |
| 3 KB | 4 KB | 128 KB (cap) | 32 | 22 | 21.1 |
| 4 KB | 4 KB | 128 KB (cap) | 32 | 22 | 21.1 |

The probe is a chain of 4096 dependent FMAs per lane, no memory traffic, 65 536
workgroups, with `shared uint pad[N]` declared and touched — so its time is only how many
workgroups are resident to interleave. Fourteen sizes, every one where the rule puts it,
including the two that look wrong: **1.25-1.5 KB is 29 % slower than 2 KB**, and 768 B is as
fast as 1 KB. Larger, measured earlier: 8 KB 42.4, 16 KB 86.7, 32 KB 181.6 ms — the cap
halving the resident count per doubling.

For a pipeline here the rule reads: **declare exactly an allocation size, and keep
(workgroups a core holds by threads) x size <= 128 KB.** For 32 lanes that is 1 or 2 KB;
for 128 lanes, up to 8 KB. Anything between two allocation sizes, or under 1 KB, pays.

**The driver's own output agrees**, which rules out having read the wrong code:
`INTEL_DEBUG=bat` decodes every dispatch it submits. 256 B, 768 B and 1 KB all go out with
`Shared Local Memory Size: 1 (Encodes 1K)`, and with `Preferred SLM Allocation Size` 16K,
64K and 64K; 1.5, 1.75 and 2 KB with 2K and 96K, 128K, 128K. Of the 126 fields of the
256 B and 1 KB dispatches, the preferred size is the **only** one that differs. Their
shaders differ in the unrolled initialisation — the 1 KB one stores *more* — and the hot
loop is the same instruction for instruction. So the hardware is handed the same
per-workgroup allocation and two partitions, and the timing follows the partition. What a
driver true to its own comment would program for 256 B is 64 x 1 KB = 64K, which is what
the 1 KB pipeline gets: 11 ms instead of 41.

It is a driver bug by that comment — "it estimates how many workgroups will run
concurrently per sub-slice and multiply that per each workgroup SLM size" — which only works
with the size they are given. The fix is one line in `intel_compute_slm.c`: round
`slm_size_per_workgroup` through `intel_compute_slm_calculate_size()` before multiplying,
which covers the compute, task and mesh callers at once. It arrived with MR !28910
(2024-05, "Compute the optimal preferred SLM size per subslice", replacing a partition of
0 KB that hung Xe2) and the Xe2 tables of !30541, and is unchanged in 26.2.3 and in `main`
as of 2026-09-24; no issue or MR about it turned up. It bites small workgroups — a 32- or
64-lane workgroup under 1 KB, a reduction with a few hundred bytes of shared memory, say —
and can bite any workgroup whose declaration falls between two allocation sizes. The same code serves Xe-HPG (Alchemist, Meteor Lake)
with its own tables; untested there.

**The fix, built and tested** (2026-09-24, after the system update to 26.2.3). Mesa 26.2.3
from the release tarball — its SHA-256 the one in Arch's PKGBUILD — built for the Intel
Vulkan driver only, with Arch's `b_ndebug=true`, and loaded from the build tree through
`VK_DRIVER_FILES`, the system driver untouched. Unpatched, it reproduces the system driver
at all fourteen sizes within 0.5 %. With the one-line change it programs a 64K partition up
to 1 KB and 128K from 1.25 KB, and every size up to 2 KB runs at the full rate: 256 B
41.4 -> 11.0 ms, 512 B 21.0 -> 10.9, 1.5 KB 14.2 -> 11.1; 3 and 4 KB unchanged, being the
cap. On this project it changes nothing: head hashes identical, frame time within noise,
`make test` green in both memory modes on the patched driver.

**It has a cost, and a report has to say so.** A larger partition leaves less L1. The
pointer chase at 256 B ran 2.33 ms against 3.62 patched — exactly what its 1 KB version
runs anyway: for L1-bound kernels with very little shared memory the undersized partition
was an accidental win.

`src/probe/slm_occupancy.c` and `.comp` are a standalone reproducer — Vulkan only, the
shared array sized by specialization constant — that shows the same table on the system
driver (64-256 B at 3.8-4.0x the 1 KB time) and a flat one on the patched build. An issue
draft with the patch is kept outside the repository, in `work/mesa-26.2.3/ISSUE.md`;
**not filed** — that needs the owner's account on gitlab.freedesktop.org.

## The cap: the staged GEMM at half its threads

`gemm_staged.comp` is 128 lanes, sixteen workgroups a core by threads, and declared 15.5 KB —
the A and B tiles, 7.5 KB with their padding, and an 8 KB float stage for the epilogue.
Allocated 16 KB each, sixteen would need 256 KB: **eight fit, 32 threads of 64**. This is the
cap and not the quirk — declared as exactly 16 KB it would be the same.

The tiles and the stage are never live at once — the stage is written after the last K
step has read its fragments — so they now share their bytes, as two `shared` blocks
(`GL_EXT_shared_memory_block`, `VK_KHR_workgroup_memory_explicit_layout`, which libxmx now
enables). 8 KB, sixteen workgroups, every thread. One `barrier()` before the stage is the
whole price, and it is load-bearing: without it all three head hashes change and
`test_gemm_qkv.py` fails.

1280x720, paired, two runs each: **staged GEMM 90.5 -> 70.9 ms, device total 219 -> 198.**
Bit-identical — the head hashes of the three reference frames unchanged, `make test` green
in both memory modes. (The tiled GEMM at 2 KB, window attention at 2 KB and the fused FFN at
2 KB were already at 64.)

## And the L1: real, and worth nothing here

Shared memory and the L1 data cache are one array. A chain of 48 dependent loads through a
4 KB table: 2.4 ms at 256 B declared, 3.6 at 1 KB, **10.7 at 2 KB** and above — at 2 KB x 64
the partition is the whole array and the table no longer stays in L1.

Getting it back was tried on the kernels that sit at 2 KB, and did not pay:

- the fused FFN with its hidden chunk and its stage aliased into 1 KB (they are never live
  at once either): 22.1 -> 21.9 ms, inside the noise;
- the tiled GEMM's stage cut to 1 KB, on the float32 path that never touches it: 2-6 % on a
  few memory-bound shapes, the same on the rest.

## What the missing L1 did cost: loads every subgroup made for itself (2026-09-25)

The section above asked whether a kernel *wants* the L1 back, and for these kernels the
answer is still no. The right question was what each of them fetches from L2 because it has
none — and two of them fetched the same bytes many times over:

- **window attention**, one 32-lane subgroup and eight query rows to a workgroup: the eight
  workgroups of a window each loaded its K and V, tile by tile, into their own registers;
- **the narrow blocks' fused feed-forward**, one subgroup and 16 rows to a workgroup: each
  loaded both 8 KB weight matrices, sixteen times the bytes of its own activations.

Taking loads out one at a time — a constant in place of the tile, the output wrong, the
instructions otherwise the same — located it. In window attention K cost 27 % of the pass
and V 17 %; the feed-forward's weights were 45 % of its own (12.3 -> 6.8 ms at level 0 of
1920x1088). Instruction counts pointed elsewhere and were wrong twice: a V load with 17 %
fewer instructions ran 9 % *slower*, and deleting the row sums, 230 instructions, changed
nothing.

Giving the L1 back barely helps: window attention rebuilt into 1 KB — its softmax weights
held in registers while the logits use the kilobyte — gained 1-2 %. **Loading once is what
pays.** Window attention now runs one 256-lane workgroup a window and head: K and V arrive
once, a 16-byte load a lane, and eight subgroups read their tiles from shared memory — 16 KB,
eight workgroups a core. The feed-forward runs sixteen subgroups and 256 rows a workgroup
with its weights staged — 32 KB, four a core. The same 64 threads a core either way:
occupancy was never the trade. Level 0 alone, bit-identical:

| pass | 320x320 | 1920x1088 |
| --- | ---: | ---: |
| window attention | 705 -> 340 us | 13.8 -> 6.6 ms |
| fused feed-forward | 638 -> 430 us | 12.3 -> 7.3 ms |

On the daemon's path, paired, answers byte-identical: 640x360 at 0.5 34.8 -> 30.6 ms,
512x288 at 0.35 34.2 -> 29.7, 1280x720 at 0.35 49.1 -> 43.7, 1920x1080 at 0.3 71.6 -> 63.5,
and 1920x1080 at full scale 433 -> 370. The graph's curve is **8.9 ms + 162 ms per
megapixel** (9.4 + 190 before, measured the same day).

Tried on the way and not kept:

- V stored transposed, so its tiles would load as words like K's: slower, 11.16 -> 11.32 ms;
- V's eight tiles loaded before the softmax, to hide their latency: spills, 10 % slower;
- K's four tiles loaded together before their multiplies: no change;
- the feed-forward's weights transposed as they are staged, for word-sized tile loads:
  60 % slower — every lane of a column-major load on the same shared-memory banks;
- the bias as half, which every one of the model's 62 bias tensors exactly is: 2-4 % of the
  attention pass, for a second copy of every bias and a change to the pass's interface.
  Left for later, not dropped.

## Also measured and dropped

- **The softmax pipeline** (`attention.comp`) declares 8 KB for 32 lanes, a quarter of the
  threads, and the frame only runs its bottleneck softmax, which never touches the stage.
  At 1 KB: 4.44 -> 4.04 ms. Not worth a second pipeline.
- **The base GEMM** (`gemm_resident.comp`, RM = RN = 1) declares 512 B, half the threads.
  Padded to 1 KB: no change; it is one pass a frame.

## Reproduce

The probe is small enough to rewrite: a shader with the window-attention push block, `shared
uint pad[N]` touched by every lane and either the FMA chain or a pointer chase, timed through
`Runtime.window_attention` with `XMX_WINDOW_SPV` pointing at each build. Variants of the
real kernels go in through `XMX_STAGED_SPV`, `XMX_ROW_SPV`, `XMX_GEMM_SPV` and
`XMX_FFN_SPV`; `src/bench/frame_profile.py` times them in a frame.

## Intel Windows: copy the operand bits in 128-bit vectors (2026-10-03)

The interrupted Windows session at 6987c02 had measured the K-loop knockouts requested
by Linux. Those runs suggested looking at the global-to-shared copies: removing the
fragment loads or barriers barely changed the timing, removing the copies did. They are
not a clean additive cost model. In particular, the replacement values `k0 & 7` and
`k0 & 3` are always zero with BK=32, and other variants read uninitialized shared memory
or deliberately race. No wrong-output variant was installed.

Three correct-output loader spellings were tried, leaving the fragment order and all
epilogues alone:

- Eight lanes per 32-half row, four halves per lane: some GEMMs improved, but a 720p
  frame was 182.6 ms against 186.4 ms for the old shader.
- Sixteen lanes per row, two halves per lane: slower on the measured large shapes.
- Four lanes per row, eight halves per lane as one `uvec4`: the retained variant.
  The global vector and an aliased shared `uvec4` view move the bits without conversion.
  `coopMatLoad` still sees the original half array. Both views have the same size and
  B offset, including the BM32/BK64 build.

The last variant improved 4032x256x256 float output from 145-146 to 95-97 us and
320x1024x4096 from 955-1034 to 648-666 us. It is not universally faster: the half output
at 64512x128x64 went from 531-534 to 588 us. Keeping the old loader for K<128 did not
win at frame level (174.9 ms against 169.3), so that hybrid was discarded.

The final shader retains all three loader paths. Specialization constant 2 selects
the packed path only on Intel's proprietary Windows driver by default. It additionally
requires 16-byte pointers and the row stride and current batch offset aligned to eight
halves. Eight-byte
alignment keeps the previous half4 path; other alignment keeps the scalar one. Window
gathers and B transposes are unchanged. `XMX_STAGED_PACKED=0|1`, read once before pipeline
creation, permits an A/B using the same binaries. Linux defaulted to 0 until 2026-10-07; it
is on for every driver since, after Mesa measured the same heads and about 2 % at 720p and
1080p (`notes/HANDOFF.md`, that day).

**Final implementation, off/on/on/off in separate processes, twelve warm replay samples
per process.** Arc 140V, Intel 101.9033, AC, performance power mode, quiet checks before
and after (CPU mean 4-5%; GPU idle before, 3D 1% after). The median of each process, ms:

| output / network | off, first | on, first | on, second | off, second |
|---|---:|---:|---:|---:|
| 320x320 / 320x320 | 26.596 | 24.692 | 24.222 | 25.739 |
| 1280x720 / 1344x768 | 199.634 | 170.151 | 169.133 | 186.464 |

The faster median of each mode is 5.9% and 9.3% lower with packed copies. Earlier runs
of the standalone candidate at 720p were 167.97/169.33 against 186.35/188.40 ms.
These are graph timings with no game running. The full Windows/Mesa K-loop gap remains;
the data do not identify every remaining compiler cost.

Both inputs and all three submission modes keep the pre-change head hashes:

| output | initial head SHA-256 | changed-input head SHA-256 |
|---|---|---|
| 320x320 | e62005b80145b97afe06a0db595c306d0137c83e2278be52f1c909018213bcf5 | e7c7278936c65d9cec9f826ac3da1c3ea985c5aca7b6bc48a31e8fbfca90c157 |
| 1280x720 | c217fd2fdbbe6b79330a7b22990f2c335b75f8c8133a12d9626b83c15ea978c7 | 95f46f6783f34cf106d93408e32735fdaa95bb4698465dba5ea59401e88b70bc |

`test_staged_packed.py` compares 960 output buffers byte for byte with the switch off
and on: dense and padded operands, scalar/64-bit/128-bit alignment (including a branch
change between batches), transpose, all simple epilogues and both output widths, changed
inputs, generic and specialized pipelines, partial rows, and BM64/BK32, BM32/BK32 and
BM32/BK64. It checks guards and verifies dispatches used the staged family. The earlier
standalone candidate comparison covered 452 outputs including fused residuals, QKV,
gathered windows and pooled residuals. SPIR-V validation passed for all candidate builds.
The final CMake/MinGW build passed all 37 CTest checks, excluding the documented
`gpu_window_attention` driver hang. That includes denormal, fused residual/QKV, full
graph, native image, allocator and pipe regressions. The runtime also built with MSVC;
its staged32 regression passed all 62 cases using the final shaders.

Reproduce after building: run `python src/gpu/test_staged_packed.py`, then run
`python src/bench/frame_replay.py --size 720 1280 --pairs 12 --json work/replay.json`
in separate processes with `XMX_STAGED_PACKED=0` and `=1`. Compare both head hashes,
not just time. Local raw logs and generators are in `work/gemm-windows/`; the final
measurements are `integrated-*.json`. A game with the new runtime has not been measured.
