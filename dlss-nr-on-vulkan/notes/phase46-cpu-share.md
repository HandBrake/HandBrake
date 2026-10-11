# Phase 46 — giving the E-cores part of the network, measured

2026-09-10. Asked whether the four low-power cores could take a share of the graph.
Lunar Lake's layout makes the question reasonable: `lscpu` shows cores 0-3 as Lion Cove
at 4.7-4.8 GHz sharing an L3, and cores 4-7 as Skymont at 3.7 GHz with their own L2 and
**no L3**, on the SoC's low-power island. Eight cores, no SMT.

## The measurement that settles it

Four cores pinned to 4-7, doing nothing but streaming 128 MB arrays — `b += a`, two
reads and a write per element, past every cache. Then the profiled frame, same input,
same everything:

| | device total | wall per frame |
| --- | --- | --- |
| idle E-cores | 479.9 ms | 506.2 ms |
| four E-cores streaming | 514.3 ms | 541.0 ms |
| **cost** | **+34.5 ms, +7.2 %** | +34.8 ms |

**Four busy E-cores slow the GPU frame by 7 %, while contributing nothing.** There is one
memory system on this machine — 136.5 GB/s theoretical, 70-91 measured — and
`notes/phase45` established that every data-moving pass in the frame is already sitting
at that ceiling. Anything the CPU streams comes out of the same pool.

## The ceiling on the other side

What could they contribute if the work were useful? `phase13` measured this CPU at
**214 GFLOP/s** with OpenBLAS, for all eight cores. The four Skymont cores are the
slower half at a lower clock; call their share ~65 GFLOP/s, generously. The GPU's GEMMs
run at about 1.4 TFLOP/s in a frame (`phase21`).

CPU and GPU would run in parallel, so the frame becomes `max(GPU, CPU)` and the CPU can
absorb at most `65 / (1400 + 65)` = **4.4 %** of the GEMM work before it becomes the
critical path itself. That is 4.4 % of 212 ms — **9.3 ms**.

So the arithmetic caps the gain at **+9 ms** and the measured interference is **-34.5 ms**.
Even granting that a cache-blocked CPU GEMM would steal far less bandwidth than a pure
streaming loop — the -34.5 is an upper bound on the harm, deliberately — the +9 is an
upper bound on the good, and it is under 2 % of the frame.

## And the graph will not split anyway

Block *N* consumes block *N-1*, all 71 of them, so there is no coarse parallelism to
hand out. Splitting *within* a block means the GPU waits on the CPU's slice at every
block boundary, 71 times a frame, and the synchronisation is not free even where the
shared address space makes the data transfer free.

**Verdict: no.** Not because it is hard, but because the arithmetic gives at most 2 % and
the memory system takes 7 %.

The finding generalises past this question: on a UMA APU whose GPU passes are already
bandwidth-bound, **the CPU is not spare capacity — it is a competitor for the one
resource that is already exhausted**. That is worth remembering before any future
"use the idle cores" idea.
