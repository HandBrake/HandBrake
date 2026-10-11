# Phase 51 — the numpy either side of the network, and a measurement that was measuring swap

2026-09-11. `phase47` found the bottleneck had left the graph and named the culprits:
feature assembly, composition, the head upscale, the codec — all at *output* resolution,
none shrinking with the render scale. This is the first pass at them, and it contains a
lesson about the measurements as much as about the code.

## Three fixes, all verified bit-identical

Measured at 1920x1080 with `--render-scale 0.27`:

| stage | before | after |
| --- | --- | --- |
| `encode` | 423.8 ms | **31.0 ms** |
| `decode` | 169.6 ms | **49.8 ms** |
| whole round | 1531 ms | 1004 ms |

**`decode`** was `pixels[..., [2,1,0]].astype(np.float32) / 255`: a fancy-index gather into
a new array, a second copy to widen, a third to scale. A reversed slice is a *view* and one
ufunc does the widening and the divide together — three passes to one.

**`encode`** was worse: it built an `int32` intermediate — four bytes for a value that ends
in one — then `np.minimum`, then copied each channel separately in a Python loop. Seven
full-frame passes. In place, into a strided view, is three.

**The head upscale** carried four channels when `compose` reads three; the fourth is the
temporal gate and neither game mode supplies history (`phase48`). A quarter of that pass
was for nothing.

A test caught one real error on the way: writing `decode` as a multiply by `1/255` instead
of a divide by `255` changes the last bit, because the reciprocal is not representable.
`np.divide` keeps the structure and the arithmetic.

## And then the numbers stopped making sense

After the fixes the profile said `compose` 364 ms and the head upscale 298 ms. Timed on
their own, on the same shapes, the same two calls are **73 ms and 58 ms**. Four times.

The machine was swapping. Watching `/proc/meminfo` while the resident graph is built at
three extents in one process:

```
before the model      4728 MiB available   5555 MiB swap
after the model       4389                 5618
after  512x288        3742                 5758      first run 1.34 s
after 1280x720        2051                 6095      first run 2.17 s
after 1920x1080        776 MiB             7507 MiB  first run 8.43 s
```

Swap was **already at 5.5 GiB before the measurement started** — a second model's session
was running its own daemons and a game on the same machine — and the 1080p graph pushed it
to 7.5 of 7.6 GiB. Every array the pipeline touched was being faulted back in.

**So the 364 ms and the 298 ms are the cost of swapping, not the cost of composing.** The
optimisations above are still real: they were verified bit-identical and `encode` improved
by the same factor in both conditions. But the *ranking* they were meant to inform is not
established, and the remaining work at output resolution needs re-measuring on a machine
that is not paging.

### The trap, for the list

**On a 15 GiB UMA machine the resident graph is not a background process.** 2.3 GiB of
device buffers at 720p, more at 1080p, out of the same pool as everything else. A
measurement taken while anything else holds memory — another session's daemon, a running
game, three graph extents alive in one process — is measuring the page fault rate.

`free -h` before trusting a number, and one extent per process.


## Re-measured on a clean machine (2026-09-11)

The owner updated and rebooted; swap went to **zero**, 9.5 GiB available, nothing else
running. One extent per process, as the trap above says. The numbers move a great deal:

| | 512x288, scale 1.0 | 1920x1080, scale 0.27 |
| --- | --- | --- |
| **whole round** | **112 ms (8.9 fps)** | **328 ms (3.0 fps)** — was 1004 |
| network | 89.5 ms (80 %) | 115.6 ms (35 %) |
| compose | 3.2 | 73.7 |
| head upscale | 0.0 | 60.3 |
| features | 17.6 | 23.1 |
| encode / decode / downscale | 2.2 | 55.8 |

`compose` and the head upscale land at **73.7 and 60.3 ms** — the figures they gave when
timed alone yesterday, not the 364 and 298 the swapping profile reported. The isolated
numbers were right and the pipeline profile was wrong, which is the opposite of the usual
direction and worth remembering.

**And the conclusion changes.** At the extent the live mode actually runs — 512x288 — the
network is **80 %** of the daemon's work and everything around it is 22 ms. There is
nothing left to win there; the graph is register- and bandwidth-bound (`phase45`) and it
*is* the frame again. `phase47`'s "the bottleneck left the network" holds only for a large
output, where the numpy is 213 ms of 328.

So the live mode at a small extent is finished at roughly 9-10 fps, and the remaining
question is not speed but what the picture looks like once something stretches 512x288 to
the panel. That is an upscaler's job, not ours.
