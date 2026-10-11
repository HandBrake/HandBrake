# Phase 39 — what the second review found in the layer, and what it cost

Date: 2026-09-10. Branch `review-layer`, eight findings, all acted on in 6829bc4.

## The one that was live

`exchange()` took a single `payload_size` and used it for both loops:

```c
for (size_t sent = 0; sent < payload_size; ) write(...);
for (size_t got  = 0; got  < payload_size; ) read(...);
```

That is correct only while the request and the answer are the same size, which is
exactly what stopped being true when the interface mask went in the same day. A masked
request is colour plus one byte a pixel — `w*h*5` — and the answer is colour alone,
`w*h*4`. The read loop therefore waited for a fifth more bytes than the daemon would
ever write, the daemon closed, `read` returned 0, `exchange` returned −1, and the layer
printed *"the daemon did not answer; frame unchanged"*.

So the feature shipped that morning had never once worked in a game. It passed its own
tests because those tests sent and received the same size. **The bug lived in the gap
between two components that were each tested alone.**

`test_exchange`/`test_daemon` now have a `masked` mode where the server reads
`bytes + pixels` and answers with `bytes`. Verified failing against the pre-fix
binary — `AssertionError: ('masked', 1, b'')` — before the fix was kept.

## The seven latent ones

None fired on Dead or Alive 5, and every one is reachable from something this project
already plans to run:

| finding | why it matters here |
| --- | --- |
| swapchain format assumed 4 bpp | staging is `w*h*4`; an HDR (`R16G16B16A16_SFLOAT`) swapchain is 8, and `vkCmdCopyImageToBuffer` takes its extent from the image, so the driver writes past the buffer |
| no `vkDestroySwapchainKHR` | eight slots, one consumed per resize or fullscreen toggle; a reused handle then matches a stale entry with destroyed VkImages |
| slot exhaustion silent | photo mode just goes quiet with no way to tell why |
| device taken as `devices[0]` | wrong with two devices; Proton with a DXVK and a VKD3D device in one process is two |
| family taken from `pQueueCreateInfos[0]` | VKD3D-Proton asks for a dedicated present family; a pool of the wrong family submitted anyway is invalid usage |
| `NR_LAYER_UI_MASK` exported for `--steam` | a Steam-launched game is a child of the client and inherits none of this shell's environment; it has to be in the printed launch line |
| `A && B` under `set -e` | `A` false makes the whole line false and `set -e` kills the script |

The queue table is the substantive one: `vkQueuePresentKHR` is handed a `VkQueue` and
nothing else, so the device and the family have to be recorded when the queue is handed
out. `vkGetDeviceQueue` and `vkGetDeviceQueue2` are now intercepted for that. On a family
change the pool is *destroyed* and rebuilt, not dropped — command buffers are allocated
and freed per transfer so nothing outlives it, and dropping would leak one pool per change.

## What to take from this

Two of the three real bugs this project has had in the layer were **size or ownership
assumptions that held for the single case under test and broke on the second case**:
one size for two directions, one device for a process. Both were invisible to unit tests
of either side. When a component's contract has two ends, the test has to hold both.
