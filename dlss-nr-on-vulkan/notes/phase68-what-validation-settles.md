# Phase 68 — what the validation layer settles about the present, and what it does not

> **Status, 2026-09-24 — restored, and partly history.** Written on 2026-09-21 and lost
> when `master` was reset to `origin/master` on 09-23, while `src/layer/test_present.c`
> went on citing it. The two sync modes it compares are gone: since `d22ed9d` (09-22) the
> layer has one path — it waits on the present's own semaphores and finishes its copies on
> private fences (`notes/improve-present-fences.md`) — so "the default stays `idle`" below
> is history. The harness bugs and the two traps still hold.

2026-09-21. `vulkan-validation-layers` is installed now, which was the one thing standing
between `phase66`'s headless present test and an answer about `NR_LAYER_SYNC=semaphore`.
It answers less than expected, and the way it fails to answer is the useful part.

`NR_TEST_VALIDATION=1 python3 src/layer/test_present.py` turns it on, with
**synchronization validation** — hazards, not only VUIDs. Off by default: it needs a
package this project does not require, and it roughly doubles each present.

## Both sync modes are clean

With the harness's own bugs fixed (below), `queue idle` and `semaphore` both produce
**no validation errors or warnings at all**, synchronization validation included, over
three rounds of presents in both memory modes. That is new, and it is worth having: the
semaphore ring, the redirected present, the inherited wait list and the write-back signal
are all free of invalid API usage.

It is not the thing that was wanted.

## The negative control: the test still cannot prove the wait is load-bearing

`phase66` ended by saying the test does not prove the wait on the game's semaphores
matters — remove it and every check still passes. Validation does not change that. Built
with one line altered,

    uint32_t waits = 0;    /* NEGATIVE CONTROL: drop the wait */

the layer produces **exactly the same output as the correct one: nothing.** Same in both
sync modes, against a baseline that is otherwise zero.

The mechanism is the test's own pacing. `test_present.c` ends every frame with
`vkQueueWaitIdle`, so no two submissions ever overlap and there is no hazard for
synchronization validation to find. A dependency that is redundant in practice is
invisible to a tool that reports what actually happened.

**Removing the pacing does not fix it either**, and the reason is worth writing down: the
harness carries one `acquired`/`rendered` semaphore pair for the whole run, so as soon as
frames overlap *it* becomes the invalid one — 14 `VUID-vkAcquireNextImageKHR-semaphore-01779`
and 6 `VUID-vkQueueSubmit-pSignalSemaphores-00067`, **identically with the correct layer and
with the one that drops the wait**. A test that can tell them apart needs per-frame
semaphores and fences first. That is a piece of work, not a flag.

So `phase66`'s conclusion stands unchanged, now with evidence rather than suspicion:
**the default stays `idle` until a real game runs the other one.** Validation clears the
semaphore path of invalid usage; it does not show that the wait does anything.

## Two real bugs in our own harness, and a zero baseline

Both were in `src/layer/test_present.c`, both present since it was written, both invisible
without validation, and both are fixed:

- **A layout transition outside the wait.** The submit waits on `acquired` with
  `pWaitDstStageMask = TRANSFER`, but the barrier into `TRANSFER_DST_OPTIMAL` was issued at
  `TOP_OF_PIPE` — before the stage the semaphore gates. Synchronization validation calls it
  a `WRITE_AFTER_READ` against `vkAcquireNextImageKHR`, and it is right: nothing ordered the
  transition after the acquire.
- **An undrawn image presented in `VK_IMAGE_LAYOUT_UNDEFINED`.** The first pass drew into
  "the first `image_count` frames" on the assumption that acquire hands each image over
  once. It does not have to, and here it did not: one image was acquired twice and another
  never, so an image that had never been transitioned was presented —
  `VUID-VkPresentInfoKHR-pImageIndices-01430`. It now draws until every image has been
  drawn, with a guard so a driver that withholds one ends the test rather than hangs it.

With those fixed the no-layer baseline is **completely clean**, which is what makes any
message in a layer run attributable to the layer.

## Two traps worth carrying

**The validation layer sits above this one, so it never sees the patched `imageUsage`.**
`nr_CreateSwapchainKHR` adds `TRANSFER_SRC|TRANSFER_DST` on the way down; validation
recorded the application's original flags and then flagged all 36 transfer barriers on
swapchain images as invalid usage. The images really do have the bits. `NR_TEST_TRANSFER_USAGE`
makes the test ask for them itself, which is the only way to tell that artefact from a real
finding — so turning validation on now implies it.

**The check was briefly one that could not fail.** It first read `got.stderr`; the Khronos
layer's default output is **stdout**. Pointed at a build known to produce twenty VUIDs it
reported "no errors or warnings" and passed. It now reads both streams, and it was
re-verified the only way that means anything — against that known-bad build, where it
fails and names the VUIDs. `notes/HANDOFF.md` already carries the general form of this:
a test that exercises the mechanism around the thing under test proves nothing about it.
