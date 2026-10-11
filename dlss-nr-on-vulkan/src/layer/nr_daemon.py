#!/usr/bin/env python3
"""
nr_daemon — runs DLSS-NR for the Vulkan layer.

The layer is a shared object inside the game's process and the implementation is
Python, so the frame crosses a Unix socket rather than a function call. For a photo
mode that costs nothing: the game is meant to stall while the frame is being looked
at. A per-frame pass would need the graph ported to C.

    python3 src/layer/nr_daemon.py                        # then, to trigger:
    touch /tmp/nr_trigger        # hold the enhanced frame
    rm /tmp/nr_trigger           # release it

    ENABLE_NR_LAYER=1 VK_LAYER_PATH=... VK_INSTANCE_LAYERS=VK_LAYER_dlssnr_intel \\
    NR_LAYER_SOCKET=/tmp/nr_layer.sock NR_LAYER_TRIGGER=/tmp/nr_trigger <game>

Protocol: 16 bytes of header — magic 'NRN0', width, height, VkFormat — then
width*height*4 bytes of pixels; the same number of bytes come back.
"""
from __future__ import annotations

import argparse
import json
import math
import os
import pathlib
import re
import socket
import stat
import struct
import subprocess
import sys
import time

import numpy as np

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parent.parent
# --root has to be honoured before nr_frame is imported (it reads NR_ROOT at import
# time to find work/), so scan argv here and let the parser accept the flag later.
for _i, _arg in enumerate(sys.argv):
    if _arg == "--root" and _i + 1 < len(sys.argv):
        os.environ["NR_ROOT"] = sys.argv[_i + 1]
        break
    if _arg.startswith("--root="):
        os.environ["NR_ROOT"] = _arg.split("=", 1)[1]
        break
if os.environ.get("NR_ROOT"):
    ROOT = pathlib.Path(os.environ["NR_ROOT"])
sys.path.insert(0, str(ROOT / "src" / "ref"))
sys.path.insert(0, str(ROOT / "src" / "gpu"))
sys.path.insert(0, str(ROOT / "src"))
import nr_build  # noqa: E402

import nr_frame  # noqa: E402
import xmx  # noqa: E402  (for the device name, once the model is up)
from xmxres import DeviceLost  # noqa: E402
try:
    import nr_image  # the host passes in C, when they are built
except ImportError:  # pragma: no cover - the NumPy path is the fallback
    nr_image = None

MAGIC = 0x304E524E
MASK_COVERAGE_LIMIT = 0.55   # above this the mask is the scene, not the interface
MAGIC_MASKED = 0x314E524E     # the same, with a one-byte-per-pixel interface mask after the colour

# The swapchain formats a compositor or VKD3D actually hands out. A game writes
# sRGB-encoded values into a UNORM swapchain just as it does into an SRGB one, so both
# decode the same way; what differs is only how the display reads them back.
FORMATS = {
    37: ("rgba8", "R8G8B8A8_UNORM"),
    43: ("rgba8", "R8G8B8A8_SRGB"),
    44: ("bgra8", "B8G8R8A8_UNORM"),
    50: ("bgra8", "B8G8R8A8_SRGB"),
    64: ("a2b10g10r10", "A2B10G10R10_UNORM_PACK32"),
}


def decode(raw, width, height, vk_format):
    kind, _ = FORMATS[vk_format]
    if kind == "a2b10g10r10":
        packed = np.frombuffer(raw, dtype=np.uint32).reshape(height, width)
        channels = [((packed >> shift) & 0x3FF).astype(np.float32) / np.float32(1023.0)
                    for shift in (0, 10, 20)]
        return np.stack(channels, axis=-1)
    if nr_image is not None:
        native = nr_image.decode8(raw, width, height, kind == "bgra8")
        if native is not None:
            return native
    pixels = np.frombuffer(raw, dtype=np.uint8).reshape(height, width, 4)
    # `pixels[..., [2, 1, 0]]` gathers into a new array, then `.astype` copies it again,
    # then the divide copies a third time — 170 ms for a 1080p frame. A reversed slice is
    # a *view*, and one ufunc does the widening and the scale together: 3 passes to 1.
    channels = pixels[..., 2::-1] if kind == "bgra8" else pixels[..., :3]
    # `divide`, not a multiply by 1/255: that reciprocal is not representable and the two
    # disagree in the last bit, which a test caught.
    return np.divide(channels, np.float32(255.0), dtype=np.float32)


def encode(image, raw, vk_format):
    """Write `image` back into a copy of `raw`, leaving alpha as the game left it."""
    kind, _ = FORMATS[vk_format]
    if kind == "a2b10g10r10":
        image = np.clip(image, 0.0, 1.0)
        packed = np.frombuffer(raw, dtype=np.uint32).copy().reshape(image.shape[:2])
        quantised = (image * np.float32(1023.0) + 0.5).astype(np.uint32)
        packed &= np.uint32(0xC0000000)
        for index, shift in enumerate((0, 10, 20)):
            packed |= np.minimum(quantised[..., index], 1023) << np.uint32(shift)
        return packed.tobytes()
    if nr_image is not None:
        native = nr_image.encode8(image, raw, kind == "bgra8")
        if native is not None:
            return native
    pixels = np.frombuffer(raw, dtype=np.uint8).copy().reshape(*image.shape[:2], 4)
    # The old form built an int32 intermediate — four bytes a channel for a value that
    # ends up in one — and then copied each channel separately, seven full-frame passes
    # for 424 ms at 1080p. In place, into a strided view, is three.
    quantised = np.multiply(image, np.float32(255.0), dtype=np.float32)
    np.add(quantised, np.float32(0.5), out=quantised)
    np.clip(quantised, 0.0, 255.0, out=quantised)
    target = pixels[..., 2::-1] if kind == "bgra8" else pixels[..., :3]
    target[:] = quantised.astype(np.uint8)
    return pixels.tobytes()


# The buffers a frame arrives in, kept for the next frame of the same size. A fresh one every
# frame is fresh pages every frame — 3.7 MB at 1280x720, page faults on Windows and a new mmap
# on Linux — and nothing holds a request past its answer: the daemon answers one frame at a
# time, decode and encode make arrays of their own, and the answer written into the request's
# bytes is gone, sent, before the next frame arrives.
_INBOX = {}


def receive(connection, count, probe_ok=False, keep=None):
    # Straight into one buffer where the socket allows it: collecting the chunks and
    # joining them copied the whole frame once more, 0.3 ms at 1280x720.
    into = getattr(connection, "recv_into", None)
    buffer = None
    if into is not None:
        buffer = _INBOX.get(keep)
        if buffer is None or len(buffer) != count:
            buffer = bytearray(count)
            if keep is not None:
                _INBOX[keep] = buffer
    view = memoryview(buffer) if buffer is not None else None
    chunks, got = [], 0
    while got < count:
        if into is not None:
            size = into(view[got:], min(1 << 20, count - got))
        else:
            chunk = connection.recv(min(1 << 20, count - got))
            size = len(chunk)
        if not size:
            if probe_ok and not got:
                # Connected and closed without a byte: that is `nr-ctl` or `nr-toggle`
                # asking whether anything is listening. Every status line and every press
                # of the toggle does it, and reporting each as a failed frame buries the
                # daemon's own log in noise. A truncated frame still reports.
                return None
            raise EOFError("the layer closed the connection")
        if into is None:
            chunks.append(chunk)
        got += size
    return buffer if buffer is not None else b"".join(chunks)


SOLID_RADIUS = 3


class Settings:
    """The knobs, re-read from a JSON file whenever it changes.

    The daemon holds the model, so restarting it to try another profile costs a second
    and loses the layer's connection. These are all post-network or extent choices —
    nothing here invalidates the weights — so they can move between frames. `nr-ctl`
    writes the file; anything may, it is one flat object.
    """

    KNOBS = ("profile", "intensity", "detail_strength", "colour_strength", "render_scale",
             "temporal", "cut_limit", "hold", "release", "min_extent")

    def __init__(self, args):
        self.path = args.settings
        self.stamp = None
        for knob in self.KNOBS:
            setattr(self, knob, getattr(args, knob))

    def refresh(self):
        if not self.path:
            return
        try:
            stamp = os.stat(self.path).st_mtime_ns
        except OSError:
            return
        if stamp == self.stamp:
            return
        self.stamp = stamp
        try:
            with open(self.path) as handle:
                given = json.load(handle)
        except (OSError, ValueError) as error:
            print(f"settings: {error}; keeping the current ones", flush=True)
            return
        if not isinstance(given, dict):
            print("settings: expected a JSON object; keeping the current ones", flush=True)
            return
        changed = []
        for knob in self.KNOBS:
            if knob not in given:
                continue
            value = given[knob]
            if knob == "profile":
                if value not in nr_frame.PROFILES:
                    print(f"settings: no profile {value!r}", flush=True)
                    continue
            else:
                try:
                    value = float(value)
                except (TypeError, ValueError):
                    print(f"settings: {knob} is not a number", flush=True)
                    continue
                if not math.isfinite(value):
                    print(f"settings: {knob} must be finite", flush=True)
                    continue
                if knob == "render_scale":
                    if not 0.05 <= value <= 1.0:
                        print("settings: render_scale must be between 0.05 and 1", flush=True)
                        continue
                elif knob in ("temporal", "cut_limit", "hold"):
                    # `temporal` is the vendor's history confidence, which multiplies a
                    # gate that is already in [0, 1]; above 1 it would extrapolate past
                    # the previous frame, which is ringing, not stability.
                    if not 0.0 <= value <= 1.0:
                        print(f"settings: {knob} must be between 0 and 1", flush=True)
                        continue
                elif knob == "release":
                    # levels of 255; 0 is off
                    if not 0.0 <= value <= 255.0:
                        print("settings: release must be between 0 and 255", flush=True)
                        continue
                elif knob == "min_extent":
                    # the graph's own floor is 128; above 4096 nothing is left to pad
                    if not 128.0 <= value <= 4096.0:
                        print("settings: min_extent must be between 128 and 4096", flush=True)
                        continue
                elif not 0.0 <= value <= 2.0:
                    # the vendor's own panel stops at 2 (notes/phase30-control-atlas.md)
                    print(f"settings: {knob} must be between 0 and 2", flush=True)
                    continue
            if getattr(self, knob) != value:
                setattr(self, knob, value)
                changed.append(f"{knob}={value}")
        if changed:
            print("settings: " + ", ".join(changed), flush=True)


def resample(image, size):
    """Bilinear resize of (H, W, C) to `size`, area-averaging when it divides evenly.

    Pure numpy because this runs per frame: ImageMagick through a subprocess, which is
    what `image_io.load` uses, costs more than the network does at these extents.

    Separable — one pass down the rows, then one across the columns. The obvious
    two-dimensional form, `image[y0][:, x0] * ... + image[y1][:, x1] * ...`, materialises
    four full-size gathers and measured **193 ms** on an 854x480 head against 20 ms for
    this, which made it 55 % of the whole frame. Splitting the axes leaves two gathers
    of the intermediate size instead.

    Downscaling by a whole factor takes the area mean instead of point-sampling, since
    the alternative feeds the network aliasing it would then try to enhance.
    """
    height, width = image.shape[:2]
    new_height, new_width = int(size[0]), int(size[1])
    if (new_height, new_width) == (height, width):
        return image
    if (new_height and new_width and height % new_height == 0 and width % new_width == 0
            and height > new_height and width > new_width):
        # The area mean as NumPy's own `mean((1, 3))` computes it — each block's samples
        # added one at a time in row-major order, then divided by their count — written as
        # whole-image adds. Byte-identical to it (test_daemon.py), and 0.5 ms at 640x360
        # where the multi-axis reduction took 4.8 — 1.9 against 16.5 at 1024x768 — which
        # every scale of exactly 0.5 paid.
        fy, fx = height // new_height, width // new_width
        if nr_image is not None:
            # The same adds in C, one pass instead of five: 1.5 -> 0.2 ms at 640x360.
            native = nr_image.area_mean(image, (fy, fx))
            if native is not None:
                return native
        blocks = np.asarray(image, np.float32).reshape(new_height, fy, new_width, fx, -1)
        total = blocks[:, 0, :, 0].copy()
        for dy in range(fy):
            for dx in range(fx):
                if dy or dx:
                    total += blocks[:, dy, :, dx]
        total /= np.float32(fy * fx)
        return total

    if nr_image is not None:
        # The bilinear branch only: the area mean above is a different filter on purpose,
        # and is what keeps aliasing out of what the network is asked to enhance.
        native = nr_image.bilinear(image, (new_height, new_width))
        if native is not None:
            return native

    def axis(source, count, along):
        """One bilinear pass along `along` (0 rows, 1 columns)."""
        extent = source.shape[along]
        if extent == count:
            return source
        centres = (np.arange(count, dtype=np.float32) + 0.5) * (extent / count) - 0.5
        low = np.clip(np.floor(centres), 0, extent - 1).astype(np.int32)
        high = np.clip(low + 1, 0, extent - 1)
        weight = np.clip(centres - low, 0.0, 1.0).astype(np.float32)
        weight = weight.reshape((-1, 1, 1) if along == 0 else (1, -1, 1))
        return (np.take(source, low, along) * (1.0 - weight)
                + np.take(source, high, along) * weight)

    return axis(axis(np.asarray(image, np.float32), new_height, 0), new_width, 1)


LETTERBOX_TOLERANCE = np.float32(2.0 / 255.0)


def active_region(colour, tolerance=LETTERBOX_TOLERANCE, limit=0.45):
    """The rows and columns a letterboxed game did not draw into.

    Dead or Alive 5's smallest window is 1024x768, but it renders 16:9 inside that and
    leaves two black bars — a quarter of the frame. Every stage downstream is measured at
    the *output* resolution (`notes/phase51`), so those bars cost feature assembly,
    composition, the head upscale and the codec for pixels that carry nothing.

    A bar is a run of rows or columns whose brightest channel is at or below `tolerance`
    everywhere. Scanning stops at `limit` of the extent, so a genuinely dark scene cannot
    eat the frame, and the bars are only trusted when both sides agree to within a row —
    a letterbox is symmetric and a dark sky is not.
    """
    height, width = colour.shape[:2]
    rows = colour.max(axis=(1, 2))
    columns = colour.max(axis=(0, 2))

    def run(values, extent):
        cap = int(extent * limit)
        lead = tail = 0
        while lead < cap and values[lead] <= tolerance:
            lead += 1
        while tail < cap and values[extent - 1 - tail] <= tolerance:
            tail += 1
        # asymmetric bars are not a letterbox; a single row of slack covers odd extents
        return (lead, extent - tail) if lead and tail and abs(lead - tail) <= 1 else (0, extent)

    top, bottom = run(rows, height)
    left, right = run(columns, width)
    # A black frame — a fade, a loading screen — is bars all the way in from both sides
    # and would leave a sliver in the middle. 16:9 inside 4:3 keeps 75 %; anything under
    # half is not a letterbox, it is a dark frame.
    if (bottom - top) * (right - left) < height * width // 2:
        return 0, height, 0, width
    return top, bottom, left, right


class Letterbox:
    """The bars, found once and afterwards only checked.

    `active_region` reduces the whole frame twice, and once everything around it went
    native that made it the most expensive host pass in the frame — 21 ms of a 233 ms
    frame, more than the feature assembly, the composition and both resizes together
    (`notes/phase57`). A letterbox does not move: the game chose it when it opened the
    swapchain.

    What is checked each frame is eight lines. A bar that stopped being black, or a first
    active line that started being black, means the answer has changed and the full scan
    runs again. Either mistake is a slower frame, never a wrong crop.
    """

    def __init__(self):
        self.key = None
        self.bounds = None

    def region(self, colour, key):
        if self.key == key and self.bounds is not None and self._holds(colour):
            return self.bounds
        self.key = key
        self.bounds = active_region(colour)
        return self.bounds

    def _holds(self, colour):
        top, bottom, left, right = self.bounds
        height, width = colour.shape[:2]
        # The key is the caller's word that this is the same swapchain; the bounds fitting
        # is ours. Without this a mismatched key reads other rows and crops to them.
        if bottom > height or right > width:
            return False

        def dark(line):
            return float(line.max()) <= LETTERBOX_TOLERANCE

        for line, wanted_dark in ((colour[top - 1] if top else None, True),
                                  (colour[bottom] if bottom < height else None, True),
                                  (colour[:, left - 1] if left else None, True),
                                  (colour[:, right] if right < width else None, True),
                                  (colour[top], False), (colour[bottom - 1], False),
                                  (colour[:, left], False), (colour[:, right - 1], False)):
            if line is not None and dark(line) != wanted_dark:
                return False
        return True


def solid_regions(held, radius=SOLID_RADIUS, majority=0.75):
    """Keep only the parts of the interface mask that sit inside a solid held-still block.

    The layer marks single pixels, and that is right for an interface: a health bar is a
    slab of pixels that do not move. But a nearly static scene — a round transition, a
    slow replay — makes the *subject* half-hold too, and the mask comes back as a fine
    speckle over the character. Composing that interleaves original and re-rendered
    pixels, and since the pass moves skin by 20-30 levels the two populations differ by
    up to 100, which reads as mottled crust. Measured in `notes/phase43`.

    A majority filter separates them: an interface keeps ~85% of its pixels across a
    whole window and survives, speckle at ~60% or less does not. It is a majority rather
    than an erosion because a real interface does lose scattered pixels — an animated
    shine, bloom from the fighters — and an all-or-nothing test would erase the bar along
    with the noise.
    """
    r = int(radius)
    if r < 1:
        return held
    height, width = held.shape
    padded = np.zeros((height + 2 * r, width + 2 * r), np.int32)
    padded[r:r + height, r:r + width] = held
    integral = np.pad(padded.cumsum(0).cumsum(1), ((1, 0), (1, 0)))
    k = 2 * r + 1
    counted = (integral[k:k + height, k:k + width] - integral[0:height, k:k + width]
               - integral[k:k + height, 0:width] + integral[0:height, 0:width])
    return counted >= majority * k * k


class History:
    """The previous frame the game was handed, kept for the network's temporal path.

    The flicker this fixes is not noise in the input: 2.3 % of a live frame is
    byte-identical between presents, and the network still moves those pixels by 3.3
    levels of 255, while pixels that actually moved come back amplified 1.01x. The
    graph is global — five downsamples into a ViT-1D bottleneck whose attention sees
    the whole frame — so a fighter moving in the middle moves the decoder's answer over
    a crowd that did not move at all. Vendor stability comes from the temporal path,
    not from the network (`notes/phase53-why-it-flickers.md`).

    Held at the output extent and resampled down at use, so a render-scale change needs
    no invalidation. Dropped when the geometry moves or the shot cuts, because identity
    reprojection is a claim about a *continuing* shot.
    """

    def __init__(self):
        self.key = None
        self.output = None      # (active_h, active_w, 3), what the game last received
        self.source = None      # (inner_h, inner_w, 3), the game's own last frame
        self.pixels = None      # (active_h, active_w, 3), the game's own last frame, whole
        self.inner = None       # `output` at the network's input extent, taken ahead
        self.frames = 0
        self.cut = 0.0

    def take(self, key, source, limit):
        """History for this frame: at the network's extent, at the output's, and the
        game's own frame behind it. `(None, None, None)` when there is nothing to trust."""
        previous, self.cut = self.output, 0.0
        if key != self.key or previous is None or self.source.shape != source.shape:
            self.key, self.output, self.source, self.pixels, self.frames = key, None, None, None, 0
            self.inner = None
            return None, None, None
        self.cut = float(np.abs(source - self.source).mean())
        if self.cut > limit:
            # A round transition, a replay cut or a menu. The gate rejects wrong history
            # per pixel, but it was characterised at full scale on a pan (phase12); a
            # whole-frame replacement is the one case worth refusing outright.
            self.output = self.source = self.pixels = self.inner = None
            self.frames = 0
            return None, None, None
        if self.inner is not None and self.inner.shape[:2] == source.shape[:2]:
            inner = self.inner
        else:
            inner = previous if previous.shape[:2] == source.shape[:2] else resample(previous, source.shape[:2])
        return inner, previous, self.pixels

    def keep(self, key, output, source, pixels):
        self.key, self.output, self.source, self.pixels = key, output, source, pixels
        self.frames += 1
        # The next frame's history at the network's extent, resampled now — the daemon
        # calls this after the answer has gone, while the game draws its next frame —
        # instead of on that frame's own path: the same resample of the same array, so the
        # same bytes. A different extent next time falls back to resampling then.
        self.inner = (output if output.shape[:2] == source.shape[:2]
                      else resample(output, source.shape[:2]))


# Over how many levels of 255 the hold lets go. A pixel the game handed back unchanged
# has provably correct history; one level of change is still almost certainly the same
# surface, and by four it is something else and the model's own gate decides alone.
HOLD_RAMP = nr_frame.HOLD_RAMP


# The floor's definition lives in nr_frame now, beside the gate it bounds, so that the
# native composition and the NumPy one read the same thing; kept here by name.
hold_floor = nr_frame.hold_floor


class Meter:
    """How much the network is inventing, live, over pixels the game did not move.

    The same statistic as `src/bench/flicker.py`, but from inside a running game, because
    a replay cannot reproduce one thing that matters: `--dump` drops the daemon to about
    1.28 fps, so consecutive frames in a capture are four times further apart in game time
    than they are in play, and the history is correspondingly less correct. Keeps its own
    two frames rather than the temporal path's, so it reads the same with the path off.
    """

    def __init__(self):
        self.input = None
        self.output = None
        self.seen = 0
        self.still = 0.0
        self.invented = 0.0
        self.moving = 0.0

    def add(self, colour, output):
        if self.input is not None and self.input.shape == colour.shape:
            moved = np.max(np.abs(colour - self.input), axis=2)
            change = np.mean(np.abs(output - self.output), axis=2) * np.float32(255.0)
            still = moved == 0
            if still.any():
                self.seen += 1
                self.still += float(still.mean())
                self.invented += float(change[still].mean())
                self.moving += float(change[~still].mean()) if (~still).any() else 0.0
        self.input, self.output = np.array(colour), np.array(output)

    def report(self):
        if not self.seen:
            return "  meter: no two frames alike yet"
        return (f"  meter: {100 * self.still / self.seen:.0f}% of the frame held still, "
                f"invented {self.invented / self.seen:.2f} levels there, "
                f"{self.moving / self.seen:.2f} where it moved  "
                f"({self.seen} frames)")


DEVICE_LOST = (
    "GPU lost, stopping: the Vulkan device went away under the model ({error}), normally "
    "the driver resetting a hung GPU, and nothing on it can run again in this process. "
    "Turn the effect off and on to start a fresh daemon. If it keeps happening, lower the "
    "render scale and the game's window size, and look for a hang or reset in `sudo dmesg`.")


# Vulkan says what went wrong as a number, and the person playing sees only a frame that did
# not change. These are the two a knob can cause.
FAILURES = {
    -1: ("  — out of host memory. The daemon and the game share one pool on an integrated "
         "GPU: lower the render scale, or the game's resolution."),
    -2: ("  — the buffers for this extent do not fit in the GPU's memory. Lower the render "
         "scale, or the game's resolution; the cost follows the extent."),
}


def explain(error):
    """A sentence for the failures a player can do something about."""
    code = re.search(r"\((-\d+)\)\s*$", str(error))
    return FAILURES.get(int(code.group(1)), "") if code else ""


def serve(server, backend, args):
    """Answer frames until interrupted, or until the GPU is gone.

    A frame that fails costs that frame: the game keeps its own picture and the next one is
    tried. A lost device is not that. Every frame after it fails the same way, at the full
    cost of the copy, and the log fills with one line repeated — so it is said once and the
    daemon exits, which also lets `nr-toggle` and the panel see that there is no model.
    """
    # On Windows a frame's large arrays go back to the system as they are freed, and the next
    # frame pays a page fault for every 4 KB of them again; this keeps them (nr_alloc.py).
    # Here, in the thread that runs the frames, because NumPy's allocator is per context.
    keeping = None
    if os.name == "nt":
        import nr_alloc
        keeping = nr_alloc if nr_alloc.install() else None
    if keeping:
        print("keeping NumPy's large blocks from frame to frame (NR_KEEP_BLOCKS=0 to stop)",
              flush=True)
    while True:
        connection, _ = server.accept()
        try:
            connection.settimeout(args.timeout)
            process_connection(connection, backend, args)
        except DeviceLost as error:
            print(DEVICE_LOST.format(error=error), flush=True)
            raise SystemExit(1) from error
        except (EOFError, OSError, ValueError, RuntimeError) as error:
            print(f"frame rejected/failed; game keeps original: {error}{explain(error)}",
                  flush=True)
        finally:
            connection.close()
            if keeping:
                keeping.frame_done()


def process_connection(connection, backend, args):
    """One request. Reject invalid extents before allocating/receiving the body.

    Closing a rejected exchange makes the Vulkan layer retain its original frame.
    """
    header = receive(connection, 16, probe_ok=True)
    if header is None:
        return
    magic, width, height, vk_format = struct.unpack("<4I", header)
    if magic not in (MAGIC, MAGIC_MASKED):
        raise ValueError(f"bad magic {magic:#x}")
    if not width or not height or width * height > args.max_pixels:
        raise ValueError(f"rejected extent {width}x{height}; limit {args.max_pixels} pixels")
    payload = receive(connection, width * height * 4, keep="frame")
    interface = (receive(connection, width * height, keep="interface")
                 if magic == MAGIC_MASKED else None)
    if vk_format not in FORMATS:
        print(f"unsupported VkFormat {vk_format}; passing the frame through",
              flush=True)
        connection.sendall(payload)
        return

    live = args.live
    live.refresh()
    clock = time.perf_counter()
    whole = decode(payload, width, height, vk_format)
    # A letterboxed game — DoA5's smallest window is 1024x768 with 16:9 inside it — leaves
    # a quarter of the frame black, and every stage below is measured at the output
    # resolution (`notes/phase51`). Working on the active region alone skips that, and the
    # bars are handed back untouched: `encode(decode(v)) == v` for all 256 values, so
    # leaving them in the output array is byte-exact.
    top, bottom, left, right = args.letterbox.region(whole, (width, height, vk_format))
    boxed = (top, bottom, left, right) != (0, height, 0, width)
    colour = whole[top:bottom, left:right] if boxed else whole
    active_height, active_width = colour.shape[:2]
    # The network's cost follows the extent it is given and nothing else, so a smaller
    # internal frame is the only lever that changes the frame rate (notes/phase37,
    # phase45). What comes back up is the *head* — the detail the network drew — which
    # is then composed against the full-resolution original, so the game's own pixels
    # are never resampled and only the synthesised part is interpolated.
    inner = colour
    if live.render_scale < 1.0:
        # The scale's own frame, or the largest that lands on the same network field — the
        # same cost, more of the picture and less of its mirror image (`render_extent`).
        extent = nr_frame.render_extent(active_width, active_height, float(live.render_scale),
                                        int(live.min_extent))
        if extent != (active_width, active_height):
            inner = resample(colour, (extent[1], extent[0]))
    geometry = nr_frame.network_geometry(inner.shape[1], inner.shape[0],
                                         minimum=int(live.min_extent))
    # `inner` is a view of the decoded frame at scale 1; the history keeps it for the next
    # frame's cut test, and aliasing the decode buffer through it has caused two bugs in
    # this function already.
    shot = (width, height, vk_format, top, bottom, left, right, live.profile)
    history_inner, history_full, history_pixels = args.history.take(
        shot, inner, live.cut_limit if live.temporal > 0 else -1.0)
    # Built in the graph's own mapped input where the backend offers it, so nothing is
    # copied on the way in — on Linux that is measured and clean (test_input_fp16.py).
    #
    # Off on a discrete card under Windows: on the B580 the features built into the mapped
    # half buffer come back through the graph as NaN - every frame, reproducibly, while the
    # same features built on the host and copied in give a correct picture (measured
    # through the deployed layer: 163 clean frames with the host path, an all-black answer
    # through the mapped one, at the same resolution and scale). Discrete-card memory on
    # that driver is not behaving like the integrated one this was written on, and until
    # that is pinned down the host copy is the choice that works there. The Arc 140V under
    # Intel's own driver builds into it cleanly (notes/phase71), and there it also spares a
    # fresh features array a frame, which on Windows is page faults. NR_INPUT_VIEW=1 or 0
    # decides either way.
    discrete = getattr(getattr(getattr(backend, "runtime", None), "lib", None), "xmx_discrete", None)
    default = "0" if os.name == "nt" and discrete is not None and discrete() else "1"
    input_view = (getattr(backend, "input_view", None)
                  if os.environ.get("NR_INPUT_VIEW", default) == "1" else None)
    features = nr_frame.build_features(
        inner, geometry=geometry, history=history_inner,
        out=input_view(geometry.network_height, geometry.network_width) if input_view else None,
        **nr_frame.PROFILES[live.profile])
    head = geometry.crop(backend.run_features(features))
    # The fourth channel is the temporal gate. Without history it reaches nothing —
    # carrying it through the upscale would be a quarter of that pass for nothing
    # (notes/phase48) — and with history it is what decides the blend, per pixel.
    channels = 4 if history_full is not None else 3
    # The style's colour grade, which the vendor runs after the network, and where the
    # prediction the vendor keeps as the next frame's history is written — before the
    # grade, the intensity, the strengths and the interface restore
    # (`notes/phase70-post-process.md`).
    grade = nr_frame.grade_for(**nr_frame.PROFILES[live.profile])
    neural = (np.empty((active_height, active_width, 3), np.float32)
              if live.temporal > 0 else None)
    control = None
    held = None
    if interface is not None:
        # Red scales the blend per pixel, so an interface pixel comes back exactly as
        # the game drew it. The network still runs over the whole frame — masking the
        # composition rather than the input keeps the numerical path untouched.
        held = solid_regions(
            np.frombuffer(interface, np.uint8).reshape(height, width) > 127)
        if boxed:
            held = held[top:bottom, left:right]
        # An interface is a small part of the frame. When the held region is most of it,
        # the scene itself is standing still — a round transition, a replay pause — and
        # what is being protected is the subject, not the HUD. Composing that leaves the
        # character in patches of two different exposures, which is far worse than a
        # softened health bar. Measured in `notes/phase43`: 43% held gave a clean frame,
        # 69% and 75% both blotched. So the mask is dropped rather than trusted.
        if held.mean() > MASK_COVERAGE_LIMIT:
            print(f"  interface mask covers {100 * held.mean():.0f}% of the frame; "
                  f"that is the scene holding still, not a HUD — mask dropped",
                  flush=True)
            held = None
    if held is not None:
        control = np.ones((active_height, active_width, 3), np.float32)
        control[held, 0] = 0.0
    # What the game itself did to each pixel — the only motion signal a present-time
    # layer has, and an exact one.
    # The floor is computed inside the composition now, from the game's previous frame
    # (`nr_frame.compose`, `history_previous`); only its share for the log line is taken
    # here, on every eighth pixel of every eighth row — exactly the values the full floor
    # has there.
    previous = (history_pixels if history_pixels is not None
                and (live.hold > 0 or live.release > 0) else None)
    # The head's upscale, the composition and the encoder as one pass where the knobs
    # leave `compose_detail` a no-op: separately they were three passes over the full
    # frame, ~100 MB of memory traffic at 1280x720 where this moves half, and the bytes
    # are the same (`nr_frame.compose_encode`, `test_native_image.py`). The answer is
    # written into the request's own buffer when nothing needs the request's bytes
    # afterwards — the interface restore does.
    kind = FORMATS[vk_format][0]
    fused = None
    if (kind in ("bgra8", "rgba8") and live.detail_strength == 1 and live.colour_strength == 1
            and head.shape[0] <= active_height and head.shape[1] <= active_width):
        answer = (payload if isinstance(payload, bytearray) and held is None
                  else bytearray(payload))
        fused = nr_frame.compose_encode(
            head[..., :channels], colour,
            np.frombuffer(answer, np.uint8).reshape(height, width, 4),
            top=top, left=left, bgra=kind == "bgra8", intensity=live.intensity,
            control_mask=control, history=history_full, history_confidence=live.temporal,
            history_previous=previous, history_hold=live.hold,
            history_release=live.release, samples=8, grade=grade, neural=neural)
    full = None
    if fused is not None:
        composed, samples = fused
        encoded = answer
    else:
        if head.shape[:2] != colour.shape[:2]:
            head = resample(head[..., :channels], colour.shape[:2])
        samples = head[::8, ::8]
        composed = nr_frame.compose(head, colour, intensity=live.intensity,
                                    detail_strength=live.detail_strength,
                                    colour_strength=live.colour_strength,
                                    control_mask=control, history=history_full,
                                    history_confidence=live.temporal,
                                    history_previous=previous, history_hold=live.hold,
                                    history_release=live.release, grade=grade,
                                    neural=neural)
        if boxed:
            # A copy, not a write into `whole`: aliasing the input and the output through
            # one array has now caused two bugs in this function — `change` printed
            # 0.00000, and a `--dump` saved the composed frame as both the before and the
            # after. One frame copy is a millisecond against the round's 160.
            full = whole.copy()
            full[top:bottom, left:right] = composed
        encoded = encode(full if full is not None else composed, payload, vk_format)
    # Measure the composition itself, not what the write-back makes of it: putting the
    # result into `whole` and then differencing against `whole` compares an array with
    # itself, which reported change 0.00000. On every fourth row, like the log's other
    # figures: the whole frame took 5 ms of a 1080p frame for a number printed to five
    # places, and a quarter of the rows moves it by about a part in five hundred. Taken
    # after the answer has gone where nothing writes into `composed` before then.
    changed = None
    if held is not None:
        if full is None:
            # the restore below writes into the composition itself
            changed = float(np.abs(composed[::4] - colour[::4]).mean())
        # The control mask reaches `compose_head`, but `compose_detail` runs *after* it
        # and re-weights the whole frame: with either strength away from 1 it moved 89.6%
        # of the masked pixels, so the interface protection silently stopped working the
        # moment a knob was touched. Restoring the original wire bytes here is exact by
        # construction — it survives every later stage and does not depend on the codec
        # round-tripping. Taken from the parallel ProjectsCodex tree, which had it.
        in_place = isinstance(encoded, bytearray)
        protected = np.frombuffer(encoded, np.uint8)
        protected = (protected if in_place else protected.copy()).reshape(height, width, 4)
        original = np.frombuffer(payload, np.uint8).reshape(height, width, 4)
        if boxed:
            protected[top:bottom, left:right][held] = original[top:bottom, left:right][held]
        else:
            protected[held] = original[held]
        if not in_place:
            encoded = protected.tobytes()
        # so a --dump shows what was actually sent
        (full[top:bottom, left:right] if full is not None else composed)[held] = colour[held]
    connection.sendall(encoded)
    if changed is None:
        changed = float(np.abs(composed[::4] - colour[::4]).mean())
    # What the game was handed at the active region, for the dump below.
    active = full[top:bottom, left:right] if full is not None else composed
    if live.temporal > 0:
        # What is carried forward is the vendor's history, the prediction before anything
        # after the network — not what the game received, which the intensity, the grade,
        # the strengths and the interface restore have all moved. Kept as they are, not
        # copied: each is this frame's own — decode, the resample and the composition all
        # hand back fresh arrays, nothing writes them from here on, and History only reads
        # what it holds. Three frame copies a frame were 0.4 ms at 640x360 and 2-3 at
        # 1280x720.
        args.history.keep(shot, neural, inner, colour)
    if args.dump:
        import image_io
        try:
            destination = pathlib.Path(args.dump)
            destination.mkdir(parents=True, exist_ok=True)
            # Numbered, so walking through a game and pressing the trigger repeatedly
            # keeps every shot instead of overwriting the last one.
            index = 1 + max((int(path.stem.split("_")[0]) for path in destination.glob("*_in.png")
                             if path.stem.split("_")[0].isdigit()), default=0)
            shown = full
            if shown is None and boxed:
                shown = whole.copy()
                shown[top:bottom, left:right] = active
            image_io.save(whole, destination / f"{index:03d}_in.png")
            image_io.save(shown if shown is not None else active,
                          destination / f"{index:03d}_out.png")
            print(f"  -> {destination}/{index:03d}_{{in,out}}.png", flush=True)
        except (OSError, subprocess.CalledProcessError, ValueError) as error:
            print(f"frame returned, but dump failed: {error}", flush=True)
    if args.meter is not None:
        args.meter.add(colour, active)
    note = "" if held is None else f"  interface {100 * held.mean():.0f}% left alone"
    if live.temporal > 0:
        if history_full is None:
            note += f"  no history, cut {args.history.cut:.4f}"
        else:
            gate = float(nr_frame.history_weight(samples).mean())
            hold = "" if previous is None else (
                f", held {100 * float((hold_floor(colour[::8, ::8], previous[::8, ::8], live.hold) > 0).mean()):.0f}%")
            if previous is not None and live.release > 0:
                kept = nr_frame.release_factor(colour[::8, ::8], previous[::8, ::8],
                                               nr_frame.release_slope(live.release))
                hold += f", released {100 * float((kept == 0).mean()):.0f}%"
            note += f"  gate {gate:.3f}{hold}, cut {args.history.cut:.4f}"
    box = "" if not boxed else (f"  letterbox {height - (bottom - top)}px of rows and "
                               f"{width - (right - left)}px of columns skipped")
    # in+run+out: the two host transfers around the graph. They are a memcpy where the GPU
    # shares this memory and a trip across PCIe where it does not, and a single frame time
    # cannot tell those apart — which is exactly the question on a discrete card.
    carried, ran, read = getattr(backend, "split", (0.0, 0.0, 0.0))
    split = f"  gpu {1000 * carried:.0f}+{1000 * ran:.0f}+{1000 * read:.0f}ms" if ran else ""
    print(f"{width}x{height} {FORMATS[vk_format][1]} in "
          f"{time.perf_counter() - clock:.2f}s  "
          f"change {changed:.5f}{note}{split}{box}"
          f"  network {geometry.network_width}x{geometry.network_height}"
          f" scale {live.render_scale:g}"
          + (f" (runs as {inner.shape[1] / active_width:.3g})"
             if inner.shape[1] != round(active_width * live.render_scale)
             and live.render_scale < 1.0 else ""), flush=True)
    if args.meter is not None:
        print(args.meter.report(), flush=True)


def check_half_rounding(tolerance=0):
    """Fail loudly if the driver stopped rounding float32 to half where the graph needs it.

    `float(float16_t(x))` is correct only while the compiler keeps the conversion. Mesa
    folds it away - 360 428 of 360 704 values came back unrounded when this was written
    (src/bench/half_probe.py) - and every vendor rounding point in publish.glsl then moves,
    silently, per frame. The graph has been through that once; a driver update must not be
    able to put it back without anyone noticing.

    Three spellings are compared against numpy's float16 on values that cover ordinary
    numbers, half subnormals, overflow and the edges, and the one libxmx compiles into
    `half_round` for this driver (`xmx_half_by_cast`, `XMX_HALF_ROUND` to override) has to
    match exactly. `NR_SKIP_HALF_CHECK=1` skips it for a machine where the probe
    itself misbehaves, and says so in the log.

    Runs on whatever XMX_UNARY_SPV names, the same entry point the graph's own unary
    passes use, so it exercises the real path rather than a special case.
    """
    if os.environ.get("NR_SKIP_HALF_CHECK") == "1":
        print("half rounding: NOT CHECKED (NR_SKIP_HALF_CHECK=1)", flush=True)
        return
    spv = probe_shader()
    if spv is None:
        # Not fatal: a deployment that left the probe out still runs the graph.
        print(f"half rounding: NOT CHECKED (no {nr_build.shader('half_probe.spv')})", flush=True)
        return
    # In a child process, deliberately. libxmx.c keeps its Vulkan state in process
    # globals, so a probe that builds a runtime inside this one resets state the graph is
    # about to build on - on the B580 every frame then comes back NaN (measured here:
    # all-black answers through the daemon, clean with NR_SKIP_HALF_CHECK=1, which is how
    # this was found). A child keeps the probe's Vulkan lifetime out of the graph's.
    # Its stdin is NUL: a daemon the layer spawned has no valid one, and asking Windows for
    # the parent's then fails with WinError 6 before the probe starts.
    r = subprocess.run(
        [sys.executable, str(pathlib.Path(__file__).resolve()), "--half-probe"],
        env={**os.environ, "NR_ROOT": str(ROOT),
             # the same build as this process (NR_BUILD_DIR, else the newest: nr_build)
             "NR_BUILD_DIR": str(nr_build.BUILD_DIR),
             "PYTHONPATH": os.pathsep.join([str(ROOT / "src" / "gpu"),
                                            str(ROOT / "src" / "ref"),
                                            str(ROOT / "src" / "layer"),
                                            str(ROOT / "src")])},
        stdin=subprocess.DEVNULL, capture_output=True, text=True, timeout=180,
        creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    for line in (r.stdout or "").splitlines():
        print(line, flush=True)
    if r.returncode != 0:
        raise SystemExit((r.stderr or "the half-rounding probe failed").strip())


# What the probe's three outputs are on each runtime (src/bench/half_probe.comp,
# src/gpu/metal/half_probe.metal, src/gpu/d3d12/half_probe.hlsl): the bit-twiddled rounding,
# the pack-and-unpack round trip, and the cast. `xmx_half_by_cast()` says which of the last
# two the runtime's `half_round` is.
PROBE_SPELLINGS = {
    "vulkan": ("bit-twiddled", "packHalf2x16", "float16_t"),
    "metal": ("bit-twiddled", "as_type<half2>", "float(half(x))"),
    "d3d12": ("bit-twiddled", "f32tof16", "float(half(x))"),
}


def probe_shader():
    """The half probe as libxmx takes a shader: its bare name when the runtime carries the
    module compiled in (the CMake build; libmetalmx and libd3dmx always), else the file this
    build wrote, or None where there is neither."""
    import ctypes
    import xmx
    lib = xmx.native_library("xmx")
    embedded = getattr(lib, "xmx_embedded_shader", None)
    if embedded is not None:
        embedded.argtypes = [ctypes.c_char_p]
        embedded.restype = ctypes.c_size_t
        if embedded(b"half_probe.spv") > 0:
            return "half_probe.spv"
    path = nr_build.shader("half_probe.spv")
    return str(path) if path.exists() else None


def half_probe_child():
    """The probe as its own process: compare, print, exit 0 or 1."""
    spv = probe_shader()
    if spv is None:
        print(f"half rounding: NOT CHECKED (no {nr_build.shader('half_probe.spv')})", flush=True)
        return 0
    import xmxres
    os.environ["XMX_UNARY_SPV"] = spv
    probe = xmxres.Runtime()
    rng = np.random.default_rng(3)
    values = np.concatenate([
        rng.standard_normal(1 << 16).astype(np.float32) * 4.0,      # ordinary
        rng.standard_normal(1 << 14).astype(np.float32) * 1e-5,     # half subnormals
        rng.standard_normal(1 << 12).astype(np.float32) * 1e-7,     # far below
        rng.standard_normal(1 << 12).astype(np.float32) * 1e5,      # overflow range
        np.float32([0.0, -0.0, 65504.0, 65520.0, 65519.0, 6.09e-05, 5.96e-08, 2.98e-08]),
    ]).astype(np.float32)
    n = (values.size + 255) // 256 * 256
    padded = np.zeros(n, np.float32)
    padded[:values.size] = values
    src = probe.buffer_from(padded)
    outs = [probe.buffer(n) for _ in range(3)]
    probe.begin()
    probe.unary(0, src, outs[1], n, second=outs[0], third=outs[2])
    probe.submit()
    with np.errstate(over="ignore"):
        want = padded.astype(np.float16).astype(np.float32)
    names = PROBE_SPELLINGS[nr_build.backend()]
    mismatches = {}
    for name, buf in zip(names, outs):
        # download, not view: on a card whose memory the host cannot map (XMX_STAGING=1
        # here) a graph buffer has no view, and the daemon died at its first start
        got = buf.download(np.float32, n)
        bad = ~((got == want) | (np.isnan(got) & np.isnan(want)))
        mismatches[name] = int(bad.sum())
    print("half rounding: " + ", ".join(f"{k} {v}/{n}" for k, v in mismatches.items()),
          flush=True)
    # What the runtime's half_round runs on this driver: libxmx chose it when it built the
    # runtime above, from the driver (xmx_init); libmetalmx and libd3dmx each have one.
    used = names[2] if probe.lib.xmx_half_by_cast() else names[1]
    if mismatches[used] > 0:
        print(f"FAIL: {used} disagrees with float16 on {mismatches[used]} of {n} values; "
              "every rounding point in publish.glsl would move, per frame, silently.",
              flush=True)
        return 1
    if mismatches[names[1]] > 0 and used != names[1]:
        # The round trip is folded on this driver, and half_round uses the cast here. The
        # attention shaders' softmax uses packHalf2x16 only as a bit trick on the packed
        # word, which the fold does not touch (publish.glsl).
        print(f"  note: {names[1]}'s round trip is folded on this driver "
              f"({mismatches[names[1]]}/{n} values); half_round uses {used} here",
              flush=True)
    return 0


def end_with_game():
    """End this daemon when the game that started it exits.

    On Windows the layer starts the daemon from inside the game and puts the game's process
    id in NR_LAYER_SPAWNED. Steam counts every process a game starts as the game, so a daemon
    that outlived it kept the game running for Steam: Restore Steam refused, and the model
    stayed in memory until something killed the daemon. A daemon started any other way has
    no id there and runs until it is stopped, as before; so does Linux's, whose layer marks
    its child with 1.
    """
    try:
        pid = int(os.environ.get("NR_LAYER_SPAWNED", ""))
    except ValueError:
        return None
    if os.name != "nt" or pid <= 1:
        return None
    import ctypes
    import threading
    from ctypes import wintypes
    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel32.OpenProcess.restype = wintypes.HANDLE
    kernel32.OpenProcess.argtypes = (wintypes.DWORD, wintypes.BOOL, wintypes.DWORD)
    kernel32.WaitForSingleObject.restype = wintypes.DWORD
    kernel32.WaitForSingleObject.argtypes = (wintypes.HANDLE, wintypes.DWORD)
    synchronize, infinite, no_such_process = 0x00100000, 0xFFFFFFFF, 87
    # The handle is held from here on, so the id cannot be reused under it.
    handle = kernel32.OpenProcess(synchronize, False, pid)
    if not handle:
        error = ctypes.get_last_error()
        if error == no_such_process:
            print(f"the game that started this daemon (process {pid}) has already exited; "
                  "stopping", flush=True)
            os._exit(0)
        print(f"cannot watch the game that started this daemon (process {pid}, WinError "
              f"{error}); it runs until it is stopped", flush=True)
        return None

    def wait():
        kernel32.WaitForSingleObject(handle, infinite)
        print(f"the game that started this daemon (process {pid}) has exited; stopping",
              flush=True)
        os._exit(0)

    print(f"ends with the game that started it (process {pid})", flush=True)
    thread = threading.Thread(target=wait, name="end-with-game", daemon=True)
    thread.start()
    return thread


def main():
    if len(sys.argv) == 2 and sys.argv[1] == "--half-probe":
        sys.exit(half_probe_child())

    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--socket", default="/tmp/nr_layer.sock")
    parser.add_argument("--root", default=None,
                        help="deployment root: work/ and src/ are looked for under it "
                             "(also read from NR_ROOT before this parser runs)")
    parser.add_argument("--profile", default="standard", choices=sorted(nr_frame.PROFILES))
    parser.add_argument("--intensity", type=float, default=1.0)
    parser.add_argument("--detail-strength", type=float, default=1.0)
    parser.add_argument("--colour-strength", type=float, default=1.0)
    parser.add_argument("--settings", default=None,
                        help="a JSON file of knobs, re-read whenever it changes; "
                             "`nr-ctl` writes it")
    parser.add_argument("--render-scale", type=float, default=1.0,
                        help="run the network on this fraction of each side; the head is "
                             "scaled back and composed against the full-size frame")
    parser.add_argument("--temporal", type=float, default=1.0,
                        help="how much of the model's own history gate to trust, 0-1; "
                             "0 turns the temporal path off entirely")
    parser.add_argument("--hold", type=float, default=1.0,
                        help="how hard to hold pixels the game handed back unchanged, "
                             "0-1; this is the floor under the model's own gate")
    parser.add_argument("--release", type=float, default=24.0,
                        help="levels of 255 of change in the game's own pixel by which none "
                             "of the model's history gate survives there, 0-255; 0 is off")
    parser.add_argument("--min-extent", type=float, default=320.0,
                        help="the smallest side the network's frame is padded to, 128-4096; "
                             "320 is what NVIDIA's own driver does, the graph runs down to 128")
    parser.add_argument("--cut-limit", type=float, default=0.15,
                        help="mean absolute frame-to-frame change above which the shot is "
                             "taken to have cut and the history is dropped")
    parser.add_argument("--max-pixels", type=int, default=1 << 22,
                        help="refuse frames larger than this, rather than thrash")
    parser.add_argument("--dump", help="write each frame in and out as PNG, for a look")
    parser.add_argument("--meter", action="store_true",
                        help="report, every frame, how much the network invents over "
                             "pixels the game did not move; costs about 5 ms")
    parser.add_argument("--timeout", type=float, default=60,
                        help="socket inactivity timeout in seconds")
    args = parser.parse_args()
    if args.max_pixels <= 0 or args.timeout <= 0:
        parser.error("--max-pixels and --timeout must be positive")
    if not 0.05 <= args.render_scale <= 1.0:
        parser.error("--render-scale must be between 0.05 and 1")
    for knob in ("temporal", "cut_limit", "hold"):
        if not 0.0 <= getattr(args, knob) <= 1.0:
            parser.error(f"--{knob.replace('_', '-')} must be between 0 and 1")
    if not 0.0 <= args.release <= 255.0:
        parser.error("--release must be between 0 and 255")
    if not 128.0 <= args.min_extent <= 4096.0:
        parser.error("--min-extent must be between 128 and 4096")
    # Before the model loads: a game that is closed during the load takes the daemon with it.
    end_with_game()
    args.live = Settings(args)
    args.history = History()
    args.letterbox = Letterbox()
    args.meter = Meter() if args.meter else None

    if not nr_frame.WEIGHTS.exists():
        # The first thing a new clone hits, and a traceback is a poor way to say it.
        raise SystemExit(
            f"no weights at {nr_frame.WEIGHTS}.\n"
            "They are NVIDIA's and are not distributed here: extract them from your own "
            "copy of nvngx_dlssnr.dll as the README's Build section describes.")
    started = time.perf_counter()
    # Check the rounding the graph depends on before any frame is processed. Runs first,
    # while XMX_UNARY_SPV can still select the probe shader - the graph's own runtime is
    # built below and picks its shader then.
    check_half_rounding()
    backend = nr_frame.ResidentBackend()
    # which GPU, because the library takes the first Vulkan device and a machine can
    # have more than one, or a different one than the person assumes
    print(f"model ready in {time.perf_counter() - started:.1f}s"
          f" on {xmx.device_name()}", flush=True)
    print(f"buffers in {xmx.memory_note()}", flush=True)
    print(f"gemm on {xmx.path_note()}", flush=True)
    rt = backend.runtime
    print("runtime options " + " ".join(
        f"{name}={int(getattr(rt, attr))}" for name, attr in (
            ("NR_BATCH_FFN", "batch_ffn"), ("NR_FUSE_QK", "fuse_qk"),
            ("NR_JOINT_QKV", "joint_qkv"), ("NR_INPUT_FP16", "input_fp16"),
            ("NR_COMPACT_HEAD", "compact_head"), ("NR_FUSE_RESIDUAL", "fuse_residual"),
            ("NR_FUSE_WINDOW_RESIDUAL", "fuse_window_residual"),
            ("NR_FUSE_WINDOW_ATTENTION", "fuse_window_attention"),
            ("NR_FUSE_ATTENTION_MERGE", "fuse_attention_merge"),
            ("NR_QKV_EPILOGUE", "qkv_epilogue"), ("NR_FUSE_GLUE", "fuse_glue"),
            ("NR_FUSE_FFN", "fuse_ffn"), ("NR_FUSE_BRANCHED_FFN", "fuse_branched_ffn"),
            ("NR_FUSE_PARTITION", "fuse_partition"),
            ("NR_FUSE_TRANSITION", "fuse_transition"),
            ("NR_FUSE_MERGE_FFN", "fuse_merge_ffn"), ("NR_FUSE_STEM_FFN", "fuse_stem_ffn"),
            ("NR_FUSE_POOL", "fuse_pool"), ("NR_FUSE_WINDOW_BLOCK", "fuse_window_block"),
            ("NR_FUSE_HEAD", "fuse_head"),
            ("NR_FUSE_GLOBAL_ATTENTION", "fuse_global_attention"))),
          flush=True)

    if os.name == "nt":
        # Windows has no AF_UNIX in CPython, so the layer and the daemon meet on a
        # named pipe instead. nr_pipe gives the same accept/recv/sendall shape a
        # socket has, so serve() below does not care which one it got.
        import nr_pipe
        server = nr_pipe.NamedPipeServer(args.socket, backlog=4, timeout=None)
        print(f"listening on {args.socket}", flush=True)
        try:
            serve(server, backend, args)
        except KeyboardInterrupt:
            pass
        finally:
            server.close()
            backend.close()
        return

    if os.path.lexists(args.socket):
        if not stat.S_ISSOCK(os.lstat(args.socket).st_mode):
            backend.close()
            raise SystemExit(f"socket path is occupied by a non-socket: {args.socket}")
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as probe:
            probe.settimeout(0.2)
            try:
                probe.connect(args.socket)
            except ConnectionRefusedError:
                os.unlink(args.socket)
            else:
                backend.close()
                raise SystemExit(f"a daemon is already listening at {args.socket}")
    server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    server.bind(args.socket)
    server.listen(4)
    print(f"listening on {args.socket}", flush=True)

    try:
        serve(server, backend, args)
    except KeyboardInterrupt:
        pass
    finally:
        server.close()
        backend.close()
        if os.path.exists(args.socket):
            os.unlink(args.socket)


if __name__ == "__main__":
    main()
