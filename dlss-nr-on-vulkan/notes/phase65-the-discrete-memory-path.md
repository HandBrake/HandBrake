# Phase 65 — a memory path for a card that does not share the host's

2026-09-19. `phase63` found why an Arc B580 ran 50x slower than this iGPU: every buffer was
asking for host-cached memory, which on a discrete card is system RAM. The fixes that
followed — device-local where the host does not look, cached where it does — are only half
an answer, because they still require the card's memory to be *mappable*. Without resizable
BAR the host-visible window is 256 MB, a frame needs more, and the code falls back to system
memory. This is the other half: **the graph's buffers do not have to be addressable by the
host at all.**

Written and proved on hardware that cannot exhibit the problem, which is the point of the
flag below.

## What the host actually touches

Counted, not assumed. In the replayed path — the one a game runs — the host touches exactly
**two** buffers per frame:

- `features`, written once: sixteen float32 channels per output pixel;
- `head`, read once: four of every sixteen floats, a strided read.

Everything else — weights, activations, the scratch arena, every intermediate — is the
device's, written and read by the GPU alone, many times per frame. The weights are touched
once per extent, not once per frame.

So the memory split follows the code rather than a rule of thumb: `GRAPH`, `HOST_READ`,
`HOST_WRITE`. Kinds 1 and 2 are always mapped, and cached for the read. Kind 0 follows the
device: mapped where the host can see the card's memory, **device-local and unmapped where
it cannot**, and then the host reaches it only through explicit copies.

## Three decisions

**A buffer either is addressable or says so.** `Buffer.view()` on an unmapped buffer raises,
naming the alternatives. The other way — a host shadow that someone must remember to upload
— was tried by an outside port of this project (`notes/phase64` for that tree) and produced
frames rendered from data that never left the host, with no error anywhere. A loud failure
is worth more than a fast one.

**Clearing is immediate, never recorded.** The first version recorded `vkCmdFillBuffer` into
the graph when a buffer was cleared during recording. It ran on every replay, and
`buffer_from` clears a weight buffer's padding *before* uploading the weights — so the graph
wiped its own weights on the way past, every frame. The head still came back, the numbers
were plausible, and the hash was wrong: sd 6.703 against 6.438. **A recorded command is a
command that happens again.**

**A clear that is not a whole number of words is not a fill.** `vkCmdFillBuffer` works in
words and may not run past the buffer, so a 6-byte buffer had its last two bytes left alone.
Fresh device memory reads as zero, so nothing failed — the check that found it had to write
values in first. Sizes that are not multiples of four now clear through a copy instead.

**Transfers get their own command buffer.** The weights are created lazily, inside the
recording of the frame that first needs them, so a staged upload cannot borrow the recording
command buffer. A second command buffer and fence make a transfer independent of whatever is
being recorded.

## Proving it without the hardware

`XMX_STAGING=1` forces the unmapped path on a machine whose memory is all one pool. Where
the buffers physically live cannot change what the graph computes, so the iGPU can run the
code a card without resizable BAR would run — and every existing test becomes a test of it.

| check | mapped | forced staging |
| --- | --- | --- |
| one frame, 320x192, sha256 of the head | `9e1e37d981fbcf01` | **`9e1e37d981fbcf01`** |
| `test_resident` against the CPU reference | passes | passes |
| 512x288 at scale 0.35, through the socket | 73 ms | 73 ms |
| 1024x768 at scale 0.55, through the socket | 173 ms | 168 ms |
| first frame at an extent (weights uploaded) | 346 ms | 508 ms |

The head is **bit-identical**, which is the claim that matters: this is a change of address,
not of arithmetic. Steady-state frame time is unchanged here because both kinds of memory
are the same memory on this machine; the first frame costs more because the weights now go
through a staging copy instead of a memcpy.

The runtime says which path it took, because on a card this is the whole question:

```
buffers: card memory, not host-visible: the host reaches it by copies; readback cached: …
buffers: card memory, unmapped by choice (staging forced): …          (what this machine prints)
buffers: shared memory (one pool): …                                   (the iGPU's own path)
```

`make test` runs green in both modes. Five low-level tests wrote and read device buffers
directly and now go through `xmxres.host_view` / `host_write`, which work either way — worth
doing for its own sake: on a card without resizable BAR, `make test` is how its owner would
debug anything at all.

One test knows the difference. In block mode the five skip copies are host copies kept as a
reference; unmapped they become device copies with a recording each, so the submission count
is 78 rather than 73. That is the only number in the suite that depends on where memory is.

## Two things that follow from it

**The frame reports its three parts.** A frame is a host write, the graph, and a host read,
and a single total cannot tell them apart — which is the only question on a card, where the
outer two are PCIe and the middle one is not. The daemon's line now ends with
`gpu 1+144+1ms`: at 1024x768 on this machine the transfers are a millisecond each against
144 in the graph, and the rest of the 170 ms frame is the host passes in C.

**The weights stop crossing the bus for a knob.** They belonged to the frame, and a frame
belongs to one extent, so moving the render scale re-uploaded all of them. Nothing in them
depends on the extent, so they moved to a `DeviceWeights` the backend owns:

| | before | after |
| --- | ---: | ---: |
| first frame at the first extent | 335 ms | 335 ms |
| first frame at a **second** extent | ~335 ms | **82 ms** |

On a discrete card that difference is ~292 MB of PCIe traffic every time somebody drags the
scale, which is exactly the kind of thing that makes a tool feel broken while measuring
fine.

## What this does not settle

- **It has never run on a discrete GPU.** Everything above is the same code on the same
  iGPU, taking the other branch. What it removes is the dependency on resizable BAR; what it
  cannot show is the PCIe cost of the two per-frame transfers on a real card.
- **The layer still calls `vkQueueWaitIdle` twice a frame.** `nr_layer.c` says in its own
  comment that the proper route is the present's semaphores and that this "has to change
  before the pass runs every frame". It does run every frame. On a slow iGPU the network
  hides it; on a fast card it is a stall of the game's whole pipeline, and on a game that
  presents from a different queue than it renders on it is a correctness hole.
