# Phase 63 — the first report from someone else's machine: a discrete GPU

2026-09-18. Someone on Reddit ran this on an **Intel Arc B580** (BMG G21, discrete
Battlemage) and reported that the effect made their frame rate worse and that the daemon
ended every frame with `xmx_graph_run: resident submit (-4)`. They also ran
`src/probe/coopmat_probe.c` and `src/gpu/bench.py` and posted both. That output is the
first measurement this project has from hardware other than the Arc 140V it was written on,
and it says something the owner's machine could never have shown.

Everything below about their machine is **reported** — their terminal, not ours. What is
verified here is the code it points at and the numbers on this machine.

## Their hardware is right, and the probe proves it

Their probe output matches ours line for line: `subgroup size 32`, `cooperativeMatrix = 1`,
`supportedStages = compute`, `cooperativeMatrixRobustBufferAccess = 0`, and the same **six**
configs, all scope=subgroup, all M=8 N=16 — including **config 1, `fp16 × fp16 → fp32`**,
which is the path this port takes. `NV_cooperative_matrix2` present with zero flexible
configs, exactly as in `notes/hw-coopmat.md`. Two generations of Xe2, one table.

So their question — "dont really know where to put matrix or if its done automatically" —
has a short answer: nothing to place. The probe only reports. The shaders ask for that
config themselves.

## Their benchmark says the card is 50x slower than an integrated one

Same `src/gpu/bench.py`, same shapes:

| shape | Arc B580 (reported) | Arc 140V (here, measured) |
| --- | --- | --- |
| 512x512 K=512 | 141.1 GFLOP/s | 1269.0 |
| 1024x1024 K=1024 | 52.8 | 3517.2 |
| 2048x2048 K=512 | 49.8 | 3536.3 |
| 4096x1024 K=1024 | 65.0 | 3295.2 |

A discrete Battlemage has several times the matrix hardware of this iGPU and its own
memory. It should win every row. Losing them by 50x is not a slow GPU; it is a GPU reading
its operands from somewhere else.

## Where: `memtype()` asked for host-cached memory on a card that has none

`libxmx.c` chose the memory type for every buffer — weights, activations, scratch — by
preferring `HOST_CACHED` above everything else. That preference is right here and was
measured: on this shared-memory APU the uncached host-visible type ran readback at 80 MB/s
and buried a 1.35 TFLOP/s kernel (`phase8`). Both types are the same physical RAM, so
nothing is lost by asking for the cached one.

On a **discrete** GPU they are not the same memory. `HOST_CACHED` there is system RAM by
construction: the card cannot cache host memory in its own. So every operand sat behind
PCIe and 12 GB of VRAM went unused. The one machine this was written on could not exhibit
it, and no test could have caught it, because the property being asked for is legal and
available on both.

**Changed**: the preference now depends on `VkPhysicalDeviceProperties::deviceType` —
device-local first on a discrete GPU, host-cached first otherwise — with a **1 GiB floor on
the heap**, because with the BAR unresized the host-visible VRAM window is 256 MB, far less
than a frame needs, and preferring it would turn a slow run into a failed allocation. Such a
card falls back to system memory as before.

**Unverified.** There is no discrete GPU here. What is verified is that this machine is
untouched: it is integrated, so it takes the same branch it always took, `bench.py` reads
1269–3709 GFLOP/s before and after, and `make test` is green.

## The benchmark hid the real failure behind an impossible number

Their last row read `8192x512 K=512 … 0.0003 s … 495058.6 GFLOP/s … 0.009 ms`. Half a
petaflop from a card that had just managed 65 GFLOP/s. `bench.py` never checked what
`xmx_gemm` returned, so a call that failed instantly was divided into the FLOP count and
printed as throughput. Whatever went wrong on that shape — and it is the interesting event
in the whole log, since it is where their run stopped working — was reported as a record.

Now checked: a failed call prints `xmx_gemm:` and the library's own message. **A tool that
reports a rate has to check the call before it divides by the time.** This is the second
instance of `phase45`'s lesson: the arithmetic is fine, the input to it was never verified.

## Two more things that came out of the same report

**The daemon now says which GPU it is on** — `model ready in 0.4s on Intel(R) Graphics
(LNL)`. `xmx_init` takes `pds[0]`, the first device Vulkan enumerates, with no vendor check
and until now no record of the choice. On a machine with more than one GPU that is a guess
nobody could audit from the log.

**Their `make test` fails in `test_ui_mask`** with `cannot reshape array of size 0 into
shape (256,384,4)`: the daemon answered nothing, which is what it does when a frame fails.
The harness keeps the daemon's stdout and prints it only if the process exits, so the
message that would name the failure never reached them. The reproducer that does print it
is `python3 src/ref/nr_frame.py in.png out.png --resident` — no sockets, no game, the
exception on the terminal.

## The machine now says where its buffers are

Their next report was the useful kind: **0.2 fps, and the frame time barely moves when the
render scale is lowered.** That second half is a prediction the hypothesis above makes. A
cost that does not follow the extent is a *fixed* cost per frame, and this graph has one —
292 MB of weights, read once per frame whatever the extent, because the render scale shrinks
the activations and not the model. Across PCIe that is the whole frame; in the card's own
memory it is under a millisecond.

So `xmx_memory()` reports the memory type the chooser lands on, and the daemon logs it under
`model ready` while `bench.py` prints it above the table:

```
buffers: card memory: type 1, heap 11.6 GiB, DEVICE_LOCAL HOST_VISIBLE HOST_COHERENT
buffers: SYSTEM MEMORY ACROSS PCIE - resizable BAR is off, or its window is under 1 GiB
```

The second line is the one that matters for them: **the fix is inert with the BAR
unresized**, by design — the 1 GiB floor sends such a card back to system memory rather than
failing to allocate. Asked before the first buffer exists, which is when the daemon logs it,
the answer is still knowable: the same choice runs against every type the device has.

## The buffer that travels the other way

At 4 fps they said it plainly: *140v memory does not get called the same way as my b580,
vram and ram it is not the same as yours, i need some kind of edit on libxmx.c*. Right on
both counts.

Everything the graph touches is read by the device, many times, and belongs in the card's
memory. **One buffer goes the other way.** The head comes back to the host as
`buffer("head", pixels * 16).view(shape=(pixels, 16))[:, :4]` — four of every sixteen
floats, a strided read of 15 MB at a 1024x768 frame to extract 3.8 MB. On a shared-memory
APU that is a memcpy. On a discrete card, in write-combined device memory, it is a strided
uncached read across PCIe, which is the slowest access this hardware has, and it does not
shrink with anything.

So buffer creation takes a `host_read` flag (`xmx_buf_create_kind`), and the chooser gives
such a buffer cached memory even on a discrete card — the device writes it once, streaming,
and the host reads it cached. `ResidentFrame.buffer` sets it for `head` and for nothing
else, because nothing else is read back.

**On this machine it is a no-op and that is the check**: one pool, so both kinds land on
memory type 2, `bench.py` reports both lines identically, 1024x768 at scale 0.55 measures
169 ms against a published 168, and `make test` is green with the output byte-identical.
The report now prints both placements, which is what a discrete card needs to show:

```
buffers: card memory: type 1, …; readback cached: type 3, …
buffers: card memory: type 1, …; READBACK UNCACHED - the host reads the head with a stride …
```

Still their measurement to make. What the hypothesis predicts, if they pull this: the frame
time drops by roughly the head's size over PCIe read speed, and what remains scales with the
extent again rather than sitting flat.

## What a discrete card still pays, after both fixes

Enumerated from the code rather than guessed at, because the next question was "is that
everything?" and the honest answer is no — it is the largest of several.

- **The weights cross the bus again on every extent change.** `ResidentFrame` owns them, and
  a new render scale or window size builds a new frame: ~292 MB written across PCIe before
  the first frame at that extent. The weights do not depend on the extent, so this is
  avoidable — a weight cache outliving the frame — but it is a refactor of who owns and frees
  them, and it is invisible here, where the same upload is a memcpy. Measure steady state,
  not the frame after a knob moves.
- **The features cross it every frame.** Sixteen float32 channels per output pixel, ~15 MB
  at 1024x768, written into card memory. Structural: the input has to get there. Halving it
  is possible (the network reads them as half anyway) and has never been worth doing here.
- **The link itself.** A card in a x4 slot, or on PCIe 3.0, doubles every cost above. That
  is `LnkSta` in `lspci -vv`, not something the code can see.
- **Without resizable BAR both fixes are inert**, by design: the 1 GiB floor sends the card
  back to system memory rather than failing to allocate in a 256 MB window.
- **The kernel's block sizes were tuned on this iGPU** — 16x32, register-bound (`phase26`) on
  64 XMX engines. A card with several times the engines may want a different tile, and that
  is not an environment variable: `XMX_TILE_M`/`XMX_TILE_N` have to match the `-DRM`/`-DRN`
  the SPIR-V was built with, so it is a shader rebuild. Also unmeasured: at a 512x288 network
  extent a large GPU may simply be starved, which would make *bigger* extents scale better
  there than here. A prediction, and theirs to test.

`src/gpu/bench.py`'s own result buffer is now marked `host_read` as well — the benchmark
they will measure with was reading `C` back uncached, which would have understated exactly
the card this is all about.

## Still open

- **The `-4` itself.** A 50x slowdown makes a submission long enough to hit the driver's
  job timeout far more likely, and a reset is exactly what `VK_ERROR_DEVICE_LOST` reports.
  Plausible, not established — their first failure line would settle more than any amount
  of reasoning here.
- **Readback from write-combined VRAM.** Host reads of device-local host-visible memory are
  slow on a discrete card. If the new preference trades compute speed for readback time,
  the answer is a staging copy, which this code has never needed on an APU.
- **They have offered remote access to a B580 or B570.** That is the only way anything in
  this note gets verified. Owner's call.
