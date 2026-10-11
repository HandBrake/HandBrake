# Phase 64 — auditing the README against the machine

2026-09-18. `bench.py` was found publishing the parameter count that `phase61` withdrew two
days earlier — to a stranger on other hardware, who read it and quoted it back
(`phase63`). The owner's question followed: is the README itself current, or are there more
of these? Every checkable claim on the page was put against the program behind it.

**Seven were wrong.** Two of them a reader hits in the first five minutes.

## 1. The build's second line could not work for anybody

```
git -C work/vulkan-headers checkout f226aea0e17c3715baa1e2c9a4d927282725dd4b
```

That commit does not exist in `KhronosGroup/Vulkan-Headers`. The GitHub API answers *No
commit found for SHA* — so every reader following the README from a clean checkout stopped
at line 3 of the build. Nothing local could have contradicted it: `work/vulkan-headers` here
has no `.git` directory, so there was no local hash to compare against and no failure to
notice. It has presumably never worked.

Now pinned by tag, `--branch v1.4.321`, which fails at clone time if it is wrong rather than
producing a confusing `not a tree`. Verified by cloning it fresh into a scratch directory:
commit `2cd90f9d20df57eac214c148f3aed885372ddcfe`, `VK_HEADER_VERSION 321` — the version in
the tree here. The MLX-DLSS pin *is* real: it matches the local clone's HEAD and the API
resolves it.

## 2. The frame-time table was a third too slow, and contradicted the page around it

The published table was measured on 2026-09-10 (`phase47`). The host passes moved to C on
2026-09-12 (`phase57`, 74 → 28 ms), and nobody went back. The same page said Tekken 7 runs
at **10.5 fps at 640x360** four screens above a table claiming **8.6 fps** there.

Re-measured through the socket with `src/bench/live_rates.py` (new, and the point of it is
that the number now has a program): daemon + unix socket, one connection per frame as the
layer does, median of five, two frames discarded while the extent's buffers are built.

| swapchain, scale | published | measured 2026-09-18 |
| --- | ---: | ---: |
| 512x288, 0.35 | 99 ms, 10.1 fps | **72 ms, 13.9 fps** |
| 512x288, 0.50 | 108 ms, 9.2 | **72 ms, 14.0** |
| 640x360, 0.35 | 117 ms, 8.6 | **74 ms, 13.5** |
| 854x480, 0.50 | 180 ms, 5.6 | **105 ms, 9.5** |
| 1024x768, 0.55 | ~215 ms, 4.7 | **168 ms, 6.0** |
| 1920x1080, 0.55 | ~850 ms, 1.2 | **412 ms, 2.4** |

At 1080p the page was wrong by a factor of two, in the pessimistic direction — it was
telling people the thing they most want to do is half as possible as it is. `nr-ctl rates`
carried a second copy of the same stale numbers, with two extents that are not in the
README at all.

**One table now**: `nr_knobs.RATES`, which `nr-ctl` reads and `src/tools/knob_doc.py`
renders into the README beside the knob table. Same mechanism that has kept the knobs honest
since `phase56`.

These are the daemon's own numbers with nothing else on the GPU, and the page says so: a
game adds its own frame, which is why Tekken's in-game 10.5 fps sits below the bench's 13.5
at the same extent.

## 3. Four more, smaller

- **The extent curve.** `17 ms + 488 ms per megapixel` in the knob text and in
  `src/bench/upstream.py`, after `phase60` re-measured **15 + 449** across three processes.
  Now "about 15 ms + 450".
- **OpenCV.** "Without it the two strength knobs cost 7x more." `phase48` measured 110 ms
  against 31.8 at 854x480 — **3.5x**. The 7x was that pass's cost *relative to the
  composition*, a different quantity, copied across as if it were the ratio.
- **`1920x1080 is 1.2 fps`** in troubleshooting, from the same stale table.
- **The probe could not be run as written.** "run `src/probe/coopmat_probe.c`" — a C file,
  with no compile line, and on this machine the obvious `gcc … -lvulkan` fails because there
  are no system Vulkan headers here at all. The line is now written out and verified,
  including the `-I work/vulkan-headers/include` that a fresh clone needs.

## 4. What is checked from now on

Three additions to `make test`, because the rot above was invisible to every check the
project had:

- **The rates table is generated**, like the knobs. `knob_doc.py` grew a second block and a
  `--check` that fails when the page and the table disagree.
- **`src/…` paths in the published pages must exist.** The link check in `test_toggle.py`
  covered markdown links and note citations; it now also covers the programs the page tells
  you to run — `nr-panel`, `nr-ctl`, `nr-photo`, `nr-toggle`, `nr_daemon.py`,
  `coopmat_probe.c`, `nr_frame.py`.
- **`src/tools/claims_check.py`**: a withdrawn claim may not appear outside the notes that
  withdrew it, unless the retraction stands beside it. It found **eight** the day it was
  written — the 27 % subnormal figure twice in the live `src/gpu/xmx.py`, the GQA split four
  times in `hnet_model.py` / `hnet_ops.py`, one in a superseded GPU test, and one false
  positive in `test_nr_model.py`, which asserts the *correct* claim and taught the checker
  what saying the true thing looks like.

The two `hnet_*` files now say at the top that they are superseded — which `HANDOFF` has
said about them since 2026-09-09, in a file they do not link to. That is the shape of this
whole phase: the correction existed, in a place the reader of the wrong thing never reached.

## What this does not cover

Prose. "Everything the pass adds in one place it takes from another" is a summary of
measurements, not a number a test can compare. So are the game-by-game readings in *What it
looks like*. They were re-read against `phase59`, `phase62` and the measurement tables and
are consistent, but nothing enforces that, and a future one will drift the same way.
