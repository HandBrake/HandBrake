# Phase 66 — the present has a test, and the first thing it found was ours

2026-09-19. `nr_layer.c` has said since it was written that `vkQueueWaitIdle` around the
transfer is the blunt way to know a frame is finished, that the proper route is the
present's own semaphores, and that "it has to change before the pass runs every frame". The
pass has run every frame since live mode landed. This is that change — and the harness that
made it safe to make.

## What the queue idle costs

Two `vkQueueWaitIdle` calls per present. Each waits for **everything** the game has
submitted on that queue, not for the one copy the layer cares about, so the game's pipeline
drains twice a frame. On this iGPU the network hides it; on a card where the network is fast
it is the frame. And it ignores `pWaitSemaphores` entirely: a game that renders on one queue
and presents from another can hand over an image that is not finished, and nothing here
would notice.

`NR_LAYER_SYNC=semaphore` instead: the layer's submit waits on the semaphores the present
brought, signals one of its own, and the present is redirected onto that. A ring of four
command buffers, fences and semaphores, so the write-back is never waited for at all — only
the readback stalls, on its own fence, because the host is about to read those pixels.

**Default `idle`.** One environment variable either way until somebody has run `semaphore`
through a real game.

## The test that had never existed

Every other part of the layer is tested: the wire protocol against a stand-in daemon
(`test_exchange` compiles `nr_layer.c` itself), the interface detector on arrays, the
manifest against a loader. The part that runs *inside* `vkQueuePresentKHR` had nothing,
because testing it appeared to need a game.

It does not. `VK_EXT_headless_surface` gives a swapchain with no screen, and Mesa has it.
`src/layer/test_present.c` presents 64x32 frames through the layer with a Python stand-in
daemon behind it, and both directions are observable:

- the first pass clears each image to a colour carrying the frame number, and the daemon
  must be handed exactly that;
- the second pass records nothing, so each image still holds what the layer wrote into it,
  and the daemon must be handed its own answer back.

**The first run crashed.** Not the driver, not the harness: `present_now`, the function
added an hour earlier to redirect the present onto our semaphore, called *itself* in its
own fall-through branch — a search-and-replace that matched the line it had just written.
Every application would have died on its first present, in both sync modes, with a stack
overflow inside what looked like Mesa.

Finding it took the long way round — a GPU backtrace full of repeated frames, `dladdr` on
the stored function pointer, a re-entry guard that did nothing, and finally building the
layer as it stands on master and watching the same test pass. That last step is the one
that mattered: **when a test fails on new code, check the old code against it before
blaming the environment.** Four experiments were spent on Mesa's behalf.

## What it proves, and what it does not

Both modes now present, copy out what the game drew, and put the answer back where the next
frame finds it. The suite runs it, in both memory modes, on every `make test`.

It does **not** prove the wait is load-bearing. Dropping the wait on the game's own
semaphores from the semaphore path leaves every check passing: on this machine the clear is
long finished before the copy is submitted, so the race never shows. The test catches a
present that never returns, a copy that goes nowhere, and an answer that never lands. A
missing wait needs hardware where a copy can outrun a draw — or a game.

So the switch stays off by default, and the three games this project has are the next step:
Dead or Alive 5 (D3D9), Tekken 7 (D3D11), Mortal Kombat 1 (D3D12), each under a different
wrapper, each with its own queue arrangement.
