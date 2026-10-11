# Phase 68 — `nr_frame.py` as a C library, and a first frame on Apple silicon (2026-09-19)

## What was asked

The frame path in C: `nr_frame.py` — features in, the 71-block graph, the composition
out — as a library a program can call to update a frame, with no Python in the process.

## What exists now

`src/ref/nr_frame.c` and `nr_frame.h`, built as `work/libnr_frame.so`; `nr_frame_main.c`,
built as `work/nr_frame`, is the `nr_frame.py` command itself — every flag, the same
printed lines, PNG in and out through **libpng** (it first shelled out to ImageMagick as
`image_io.py` does; the owner asked for the library instead, and the bytes are the same:
`byte / 255` in, `value * 255 + 0.5` out, a 16-bit or grey PNG reduced as `-depth 8` reduces
it — the neutral render is identical file for file between the two builds), one network run
and a composition per ladder step. `--size` is the one place the C parts from the Python:
its resample is this project's bilinear (`nr_resize_axis`, the daemon's) where the Python
takes ImageMagick's filter, so a resized run differs in the resampled pixels — 67 %
identical pixels on the neutral profile, against 100 % unresized; `nr_frame_native.py` binds the library for
NumPy so it can be run beside the Python it transcribes; `test_nr_frame_c.py` does that,
and `test_nr_frame.c` does it again without Python.

The library is three files of Python in one of C, in their order:

1. **the weights** — a safetensors reader (`load_logical`), refusing anything that does not
   declare `fully_logical=true`, and the layout work `nr_resident.py` does on the way to the
   device: the attention-bias fragment swizzle for the 1- and 16-head blocks, the fused
   branched feed-forward `(C, 128)` / `(128, 32)` per output head, the `sqrt(32)` folded
   into the global scale in float32, the head padded to a `(32, 16)` tile;
2. **the graph** — `ResidentFrame._run` and every `record_*`, call for call, flag for flag,
   against libxmx's C entry points. libxmx is reached by `dlopen` from the library's own
   directory (it has no header), which also means a Python process that loads both shares
   one copy of it. The scratch arena is the same ten roles with the same aliases; the plan
   is the recording code run once in a mode where nothing records, so it cannot be short;
3. **the frame** — `build_features` on `nr_image.c`'s pass, the noise generator in float32,
   `compose` and `compose_detail`, and the temporal composition with its floor.

The API is one struct of knobs and a handful of calls: `nr_frame_open`, `nr_frame_update`
(colour in, output out, an optional history and previous frame, an optional head back),
`nr_frame_update_masked` with the per-pixel control mask, and the halves —
`nr_frame_features[_masked]`, `nr_frame_run_features`, `nr_frame_compose` — for a caller
that wants to sit between them, which the command does to compose a ladder from one
network run. The automatic mask (skin and automatic-mask structure) is in the knobs. Every
way of filling channels 10-14 is checked bit for bit against `make_features`, and the
masked composition against `compose_head` at intensities below and above one. A new extent rebuilds the graph and keeps the weights; the graph is
captured once and replayed.

## What was measured

All on the Apple M3 through MoltenVK, on the **real weights** (the owner extracted them
on this machine during the session — `source_sha256 4c269d15…`, 649 tensors, 291.6 MB
logical).

**The head is bit-identical.** `test_nr_frame_c.py` feeds the same features to the Python
resident backend and to the C library and compares the `(320, 320, 4)` head byte for byte:
equal, and equal again on replay, and equal after a second extent and back. The still
composition is bit-identical at intensity 0, 0.5, 1 and 1.66, because it is the same C.
The floor and the temporal composition are exact against NumPy's own gate through the
shared C. Twenty checks; `make test` runs them.

**Where C and NumPy part, by design and by a last bit:**

| place | why | measured |
| --- | --- | --- |
| noise channels | `logf`, `sqrtf`, `cosf`, `sinf` against NumPy's | 5 of 307 200 values, max 4.9e-04 (one half ulp at that magnitude) |
| detail blur kernel | `expf` in the Gaussian weights | max 6e-08 on the output |
| temporal gate | `expf` in the sigmoid | 0 differing pixels on this frame; can differ |

Those five noise values are what make a C-composed frame differ from a Python-composed
one: the graph is chaotic (`phase9`), so a half-ulp change in one input channel moves the
picture by its whole floor. Mean |d| between the two 720p renders of the same photo was
**0.0015** against a change of 0.018 from the input; the head test above is the exact one,
because it holds the features fixed.

**A first frame on Apple silicon.** 1280x720 (network extent 1280x768) through the
library's command line (then the PPM runner, since replaced by `work/nr_frame`), portable
GEMM, no cooperative matrix:

| | |
| --- | --- |
| weights to device | 0.9 s |
| first frame, graph recorded | 1.78 s |
| second frame | **0.90 s**: write 4.8 ms, graph 887.6 ms, read 0.0 ms |
| high-frequency energy, output / input | 1.12 |

Against the Xe2 machine's 0.49 s with the matrix units (`phase45`). The estimate in
`phase67` — "about a second" — was right. The picture was a cartoon (the only image on the
machine), so nothing is claimed about faces; the pass ran, moved the frame by 1.8 %, and
added high-frequency content.

**The rest of the suite, with the weights present:** every test that used to skip runs
and passes on the M3 — `test_resident`'s chain, `test_scratch_arena`, `test_frame_execution`,
`test_temporal_controls`, `test_nr_model`'s graph checks — 41, 23, 20, 36 and 6 checks
respectively where they count them. `make test` as a whole stops at `publish_check`,
which is a repository matter rather than a code one: see below.

## The test, in C too

`src/ref/test_nr_frame.c` (`work/test_nr_frame`) is `test_nr_frame_c.py` for a process with
no Python in it: the reader's refusals, the feature contract channel by channel transcribed
from `features.py`, the graph's determinism byte for byte across replays and extents, the
composition against its own formula at three intensities, the detail split, the temporal
gate and floor, the two-extent rebuild — 33 checks on random weights of the real shapes
(written in C from `weight_spec.json`; `NR_TEST_SYNTHETIC=1` forces that), 35 on the real
ones. What only Python can supply is the other side of the bit-identity claim, so the
Python test's `--reference REF.bin` writes the features it used and the head the Python
backend computed, and the C test given that file requires the same bytes back: **0 of
409 600 values differ** on this M3. `make test` runs the pair in that order.

**The command against the command.** `work/nr_frame` and `nr_frame.py --resident` on one
640x360 picture with the same flags: `--profile neutral` gives **identical files** (the
network switched off, the noise's last bits have nothing to amplify); `--control-mask`
91.6 % identical pixels; the default, the ladder at 0.5 and 1.66, `--skin-structure 2
--auto-mask -1 --size 320x480` all agree on the change to within 0.0002 of a 0.017 mean,
and differ pixel-wise by the chaotic amplification of five noise values, as above.

## Traps

- **`git ls-files` is the publish check's universe, and it moved.** Two commits made on
  this machine outside the session (`a43df7c`, `45a9e23`) removed `/work/` and `/ref/` from
  `.gitignore` and committed 205 files under `work/`: the MLX-DLSS clone, the layer
  manifest, the ICD manifest, six test binaries and a test log. No weights and no DLL —
  `*.safetensors` and `*.dll` are still ignored — but `publish_check` now fails on the six
  binaries, and will fail on every build that writes a binary into `work/`. Either put
  `/work/` back in `.gitignore` and `git rm -r --cached work`, or teach the check that
  `work/` is a build directory. **That is the owner's call and was not made here.**
- **A safetensors header's first entry may be `__metadata__`.** The C parser counted
  tensors to decide whether a comma was due, saw none after the metadata object, and read
  a file of 649 tensors as empty — "the edge weights are missing", on a file that had
  them. Count entries, not tensors.
- **Buffer ids start at zero.** A struct zeroed by `calloc` holds a valid id in every
  field; "none" has to be -1 from the first line, or `close` frees somebody else's buffer.
- **`~/Pictures` on a Mac may hold cartoons.** The measured 1.12x is a number about a
  drawing.
