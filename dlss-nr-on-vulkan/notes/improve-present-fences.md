# Present waits, private fences, and a real MK1 check

The separate-queue headless harness and queue inventory were taken from the local
upstream review work (`d927cd8`, `bcdffdd`, following `3fbc233` — never pushed; their
findings are `phase68` and `phase69`). That harness exposed
the old default's read-before-render problem. Its attempted fix drained all device
queues, which introduces host access to queues the game's other threads may be using.
This branch instead consumes the present's dependencies and waits only for its own work.

## One synchronization path

- Every first copy waits on the full `VkPresentInfoKHR` semaphore list, at TRANSFER.
  The list is not capped or partially consumed. Layout transitions are ordered after
  that wait; explicit transfer/host barriers cover the coherent staging buffer.
- A private fence completes each readback and writeback. Present then receives an empty
  wait list because the original waits have already completed on the host. If no copy
  runs, the original present is forwarded untouched.
- No queue-idle operation, access to foreign queues, or ring of presentation semaphores
  remains. The old `idle` and `semaphore` environment values are compatibility aliases.
- Allocation, begin/end, submit and fence-wait errors are checked. A failed fence wait
  prevents further capture on that device and retains potentially pending objects until
  teardown, rather than freeing/reusing them as if the copy completed.
- Layer-owned present state is serialized per device. Pool changes happen after completed
  private submissions. Growing staging storage releases old allocations; shrinking updates
  the used size independently of capacity. Held photo frames are tied to their swapchain
  and invalidated on destruction. The unsupported format 58 is no longer captured.

This is a correctness-first synchronous path, not an asynchronous pipeline optimization.
It does not claim to support every ownership/sharing arrangement of every Vulkan program.

## Reproducible checks

```sh
make work/libnr_layer.so work/test_present
NR_TEST_VALIDATION=1 python3 src/layer/test_present.py
NR_TEST_VALIDATION=1 python3 src/layer/test_present.py --negative-control
```

The validation mode deliberately issues one invalid zero-size buffer creation in a
separate probe, requires its known VUID, and reads both stdout and stderr. Missing
validation is a failure when explicitly requested. Normal baseline/layer runs must be
clean. The negative control compiles a temporary library with the wait removed and
requires wrong captured pixels in three runs of each legacy alias, not just a crash.
It exits 77 (CTest skip) when separate presentation is unavailable. Production artifacts
are not overwritten. The imported harness also supports a same-queue control.

## Direct Proton launch

The previous launcher selected a prefix but did not provide Steam application IDs.
Direct runs now export all three IDs, log the selected runtime, warn about ambiguous
automatic selection, and print the Proton log location when logging is requested.
A stand-in Proton test verifies the actual environment passed by `--proton`.

The corrected launch reached an MK1 window through Proton Hotfix and VKD3D-Proton.
That confirms the tested launch works; it does not identify the sole cause of every
earlier silent exit, for which a complete Proton log was unavailable.

## MK1 result on Intel LNL / Arc 140V

An initial 1920x1200 run reached character selection and was killed by the kernel's
global OOM killer. This was not a VK_ERROR_DEVICE_LOST finding. A pre-existing NR daemon
was still holding resources while a separate review daemon was being started. The review
daemon was stopped, then the old daemon was stopped with its launch parameters saved;
available memory increased by roughly 3 GiB.

A lower-resolution, windowed run created a 960x720 B8G8R8A8_UNORM swapchain with three
images. The layer logged queue families {0,1,2}, presenting on family 0. This inventory
alone does not identify which queue wrote each image.

A bounded real-model run processed at least 20 frames at the character-selection screen,
with the altered image visible in the game. The daemon used render scale 0.1, temporal 0,
network extent 320x320, and detected 180 letterbox rows. Frame log times were approximately
80-110 ms, graph wait about 59-65 ms, with nonzero image changes and no rejected frames or
device-lost report. These are daemon times, not game FPS, and this is not a long combat or
discrete-GPU stability test. The review trigger and review daemon were stopped afterward.

Screenshots, raw logs, local paths and game settings backups stay under ignored `work/`;
none are included in the commit. The user subsequently requested windowed 1280x720,
which was saved after the test. The original daemon was restored with its original
settings after the review daemon and game had stopped.

## Regression result

All 27 CTest entries passed (96.84 seconds), including the automated negative control.
An explicit synchronization-validation run passed the real-layer and no-layer baseline
checks, and rejected the missing-wait library by wrong image data in all six attempts.
The final strengthened check examines every pixel of the uniform test image and requires
every untouched frame to carry the reply. The launcher test checks all three Steam IDs
in a real subprocess, external libraries, spaces, and the explicit Proton override.
`make test-proton` also passed: both 64-bit and 32-bit layer libraries loaded through
their real Vulkan loaders on the Intel device.
