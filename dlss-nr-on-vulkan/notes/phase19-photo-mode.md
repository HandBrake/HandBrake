# The loop is closed: a game frame goes through DLSS-NR and comes back
2026-09-09

`notes/phase17-integration-landscape.md` left the layer capturing frames and nothing
else. It now writes them back, and there is a daemon on the other end running the
model, so a frame presented by a real Vulkan application goes out, through the network,
and returns to the swapchain.

```
python3 src/layer/nr_daemon.py            # holds the model; one second, once
src/layer/nr-photo <game>                 # or --steam for the launch option to paste
touch /tmp/nr_trigger                      # enhance and hold
rm    /tmp/nr_trigger                      # back to the game
```

Verified against `vkcube`: `[nr_layer] processed 500x500`, and the daemon reports
`500x500 A2B10G10R10_UNORM_PACK32 in 1.27s change 0.00565`. The written-back frame
carries the model's grain on the flat faces and slightly crisper glyph edges — a small
change, correctly, because a flat-shaded cube has no skin or hair for a model trained
on them to synthesise.

## The shape of it

**A file is the trigger, not a key.** It works the same on X11 and Wayland, needs no
input hooking inside another process's window, and can be driven from a script or a
hotkey daemon. While it exists the enhanced frame is held on screen; the game keeps
running underneath and comes back when it is removed.

**The frame crosses a Unix socket.** The layer is a shared object inside the game and
the implementation is Python; a photo mode is meant to stall, so the round trip is
free. A per-frame pass would need the graph ported to C, and at 1.2 s a frame that is
not the binding constraint.

**Swapchain images get `TRANSFER_DST` as well as `TRANSFER_SRC`**, added to the create
info on the way down — `vkCreateSwapchainKHR` is the only place a layer can do it, and
without both the copies are invalid.

**The daemon decodes by `VkFormat`** — the four 8-bit orderings and
`A2B10G10R10_UNORM_PACK32`, which is what this compositor actually hands out — and
writes the result back into the game's own bytes, leaving alpha alone. An unknown
format passes the frame through untouched rather than guessing.

## Two traps, both silent

**`sun_path` is 108 bytes.** The scratch directory here produced a 106-character socket
path, `snprintf` truncated it, and `connect` failed with the daemon plainly listening.
`nr-photo` refuses a path over 100 characters rather than letting it fail obscurely.

**`pkill -f <pattern>` matches the shell that runs it**, because the pattern is in its
own command line. It killed the very command that was restarting the daemon, twice,
and once took a source edit with it. The bracket trick does not help when the command
line also contains the literal name. Record the PID and kill that.

## What it is and is not

It is a photo mode: freeze, enhance, look, release. On a paused frame 1.2 s is nothing,
and a paused frame is where the model's effect is worth looking at.

It is not a live filter, and `notes/phase18-fusion.md` says why in numbers: 458.6 GFLOP
and roughly 10 GB of activation traffic for a 720p frame, against an iGPU whose measured
ceilings are 1.35 TFLOP/s on the kernel we have and 23 GB/s on memory.

`vkQueueWaitIdle` around the transfer is still the blunt way to know the frame is
finished. For a photo mode the stall is the point; a per-frame pass would have to take
over the present's semaphores instead.
