# Phase 49 — seven guards and one real bug, taken from the parallel tree

2026-09-10. The owner keeps a second working copy, `ProjectsCodex`, driven by a different
model. Its git history is behind ours and its own note says the live mode was *ported from
here* — so the interesting part is not what overlaps. It is what that tree has and this one
does not, and there was more of it than expected.

This is the second time reading it has been worth it. The first found that `.gitignore`
had been hiding the entire CPU reference from thirteen commits.

## The one that was a bug, not a hardening

**The interface mask stopped working the moment a strength knob moved.** The control mask
reaches `compose_head`, but `compose_detail` runs *after* it and re-weights the whole
frame. Measured on a synthetic frame with a solid masked band:

| strengths | masked pixels changed |
| --- | --- |
| 1.0 / 1.0 | **0.0 %** |
| 1.2 / 0.8 | **89.6 %** |

So the protection was exact at the defaults and silently absent otherwise — and `nr-ctl`,
built the same evening, invites exactly that change. Our tests never saw it because they
only ever ran at the defaults.

The other tree restores the **original wire bytes** after encoding instead of masking
inside the composition:

```python
protected = np.frombuffer(encoded, np.uint8).copy().reshape(height, width, 4)
protected[held] = original[held]
```

That is exact by construction: no later stage can undo it, and it does not depend on the
codec round-tripping. Ported, with a test that runs a second daemon at 1.2/0.8 and
**fails without the fix**.

## Six guards for running inside somebody else's process

All present there, all absent here, all reachable from a real game:

- **A present queue need not support graphics, compute or transfer.** Some drivers expose
  a present-only family, and recording `vkCmdCopyImageToBuffer` on it is invalid. Now
  checked per queue at `vkGetDeviceQueue`, and a protected queue
  (`VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT`) is never a capture source since protected
  memory cannot reach a host-visible buffer.
- **Our own fallback was leaving us issuing an illegal copy.** We patch `TRANSFER_SRC` and
  `TRANSFER_DST` onto the swapchain create info, and retry with the original if that
  fails — which keeps the game alive but means the images lack the usage the copy needs.
  We tracked the swapchain anyway. Now the fallback path marks it untracked.
- **`imageArrayLayers != 1` and protected swapchains** are skipped; the copy reads one
  2-D image.
- **`vkGetSwapchainImagesKHR`'s result was ignored.** `VK_INCOMPLETE` means more images
  than the array holds, and `pImageIndices` may then name one past the end — an
  out-of-bounds read every present. Now refused, and the index is bounds-checked anyway.
- **Nothing was ever released.** The pointer to `vkDestroyDevice` was stored and never
  used, so a staging buffer, its device memory and its host mapping leaked on every
  device teardown. `nr_DestroyDevice` is now intercepted and gives it all back, along
  with the swapchain and queue records for that device.
- **`NR_LAYER_UI_MASK=0` now means off**, not on.

Their daemon also validates settings harder — a JSON array instead of an object, a
non-finite number, a strength outside the vendor's own 0..2 range — and that is ported too.

## What we have that they do not

For the record, since the traffic goes both ways: the GPU timestamp profiler and the
per-pass frame accounting (`phase45`), the bank-conflict probe (`phase45`), the separable
resample that was 55 % of a frame (`phase47`), the diffusion-noise cache (`phase48`), the
mask's coverage limit and majority filter (`phase43`), and the notes index.

## The lesson, which is not about either model

Both trees had a correct-looking mask. Ours was exact under its tests and wrong under a
setting the tests never used; theirs was exact by construction because it restored bytes
rather than reasoning about the pipeline. **The difference is where the guarantee lives.**
A guarantee that depends on every later stage behaving is a guarantee that a new stage
quietly removes — and `compose_detail` had been in that path the whole time.
