# Phase 69 — a headless stand that can fail, and the bug it found on its first run

> **Status, 2026-09-24 — restored, and its fix never shipped.** Lost with `phase68` and
> cited by `src/layer/test_present.c`, whose stand is the one described here. The finding
> stands — the old default read the image before a draw on another queue had finished —
> but `wait_device_queues()` did not survive: draining every queue touches queues the
> game's other threads may be using. Since `d22ed9d` the layer consumes the present's
> semaphores once and waits only on its own fences (`notes/improve-present-fences.md`).

2026-09-21. `phase68` ended with the headless present test unable to tell a correct layer
from one that never waits on the game's semaphores: both passed, and so did synchronization
validation. This is the rebuild that fixes that, and the defect it found immediately —
which was not in the semaphore path everyone was watching, but in the **default** one.

## Three things were missing, and all three were needed

**Frames in flight.** One command buffer, one semaphore pair and `vkQueueWaitIdle` every
frame meant nothing ever overlapped. Now `FRAMES_IN_FLIGHT` slots, each with its own
command buffer, fence and acquire semaphore, and no queue idle anywhere.

**A render-finished semaphore per image, not per frame.** A present waits on one and never
says when it is done with it — no fence, no callback. The first attempt reused it on the
next frame and lit up with `VUID-vkAcquireNextImageKHR-semaphore-01779` and
`VUID-vkQueueSubmit-pSignalSemaphores-00067` — *identically with the correct layer and the
broken one*, which is how a harness defect masquerades as a result. Keyed on the image
index it is safe: untouched until that image comes round again, behind the fence that last
wrote it.

**Work that is genuinely unfinished when the capture starts.** The draw clears the image to
the **wrong** colour, runs 2048 serial clears of a 1024x1024 scratch image — about 20 ms —
and only then clears it to the right one. A copy that does not wait reads the wrong colour,
and the stand-in daemon says so.

**And a queue the copy can actually race.** None of the above is enough on one queue:
submissions to a single queue are not reordered, so a missing wait stays invisible however
long the stall. That was measured, not assumed — `NR_TEST_PRESENT_QUEUE=same` still passes
everything, with and without the wait. This device has three queue families, so the test
now **presents from a different family than it draws on** (family 1, COMPUTE|TRANSFER,
which the layer's own `family_can_capture` accepts), swapchain CONCURRENT across both.
That is the arrangement `nr_layer.c` singles out as dangerous and the one VKD3D-Proton
produces.

## What it found: the default was the broken one

First run of the finished stand, with the layer exactly as shipped:

| | `NR_LAYER_SYNC=idle` (default) | `semaphore` |
|---|---|---|
| present from another family | **the daemon is handed the pre-clear colour** | correct |
| present from the drawing queue | correct | correct |

`vkQueueWaitIdle(queue)` inside `vkQueuePresentKHR` waits on the queue the game *presented*
with. When the frame is drawn on a different queue that is a statement about the wrong one,
and the copy runs while the draw is still going. The daemon received `(0, 0, 255, 255)` —
the deliberate wrong colour — and `(0, 0, 0, 0)`, an image never written at all.

`src/layer/nr_layer.c` has carried the comment predicting exactly this since the layer was
written: *"a game that presents from a queue other than the one it renders on can hand us
an unfinished image."* It was right, and nothing had ever executed it.

**Fixed**: the blunt path is now bluntly correct. `wait_device_queues()` drains every queue
the device has handed out — the layer already records them all with their families, for the
command-pool problem — instead of only the presenting one. On a single-queue game it is
exactly what it was. `NR_LAYER_SYNC=semaphore` does the same job properly by waiting on the
semaphores the present brought.

This inverts the standing reading. The semaphore path was the one under suspicion; it was
the correct one all along, and the default was quietly wrong for any game with a separate
present queue. **Mortal Kombat 1 under VKD3D-Proton is the title here most likely to have
been affected** and is worth re-running.

## Acceptance, and what still does not work

| | result |
|---|---|
| baseline, no layer | passes, and clean under validation |
| the layer as shipped, both sync modes | passes, 19 checks, **five runs out of five** |
| the layer with the semaphore wait removed | **fails, three runs out of three**, 4 checks each |

The negative control fails only in `semaphore` mode, which is correct: it removes the wait
from the ring path, and the idle path is independently correct after the fix.

**Validation still says nothing about the missing wait.** The only message in a negative-
control run is the probe's own deliberate one. What catches it is the *data* check — the
daemon comparing the colour it was handed against the colour the frame was drawn with.
`phase68`'s conclusion about synchronization validation stands; the discrimination comes
from the picture, not from the tool.

## Two things the test now proves about itself

- **Validation is really in the chain.** A silent run and a run with no validation layer
  loaded look identical, so `NR_TEST_VALIDATION_PROBE=1` makes the binary issue one
  deliberate, harmless invalid call — a zero-size `vkCreateBuffer` — and the run fails
  unless `VUID-VkBufferCreateInfo-size-00912` comes back. Measured either way: two hits
  with validation on, zero with it off.
- **One clean run is not evidence.** Each sync mode runs `NR_TEST_REPEAT` times, default 2,
  labelled `#1`/`#2` so a flaky pass is visible rather than averaged away. The thing under
  test is a race, and a race that did not happen looks exactly like one that cannot.
