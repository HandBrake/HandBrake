# Reproducing the resident path

Read `notes/HANDOFF.md` first for the current model contract and limitations. The tested
machine is Intel Arc 140V (Xe2) with Mesa ANV. The runtime requires Vulkan cooperative
matrices; having a Vulkan device alone is insufficient.

The repository contains source. `work/` contains local dependencies, builds and
user-supplied weights; it is intentionally ignored. `src/ref/` is source and must
be included when committing changes (the old unanchored `ref/` ignore hid it).

## Dependencies

Install a C compiler, Vulkan loader/development linker files, glslangValidator,
Python with NumPy, and ImageMagick's `magick` command. The tested interpreter is
Python 3.14.7 with NumPy 2.5.2; a fresh Python environment was not validated here.
Pillow/OpenCV are optional for optical flow or non-unit processing scale; the still
frame and engine-motion temporal paths below do not require them.

The tested external source revisions are pinned below. This avoids silently using
a changed upstream feature/composition contract. The headers are needed even on
this machine, which has the Vulkan loader but no system Vulkan headers.

```sh
mkdir -p work
git clone https://github.com/KhronosGroup/Vulkan-Headers.git work/vulkan-headers
git -C work/vulkan-headers checkout f226aea0e17c3715baa1e2c9a4d927282725dd4b
git clone https://github.com/iamwavecut/MLX-DLSS.git work/mlx-dlss
git -C work/mlx-dlss checkout 06a3e11a8b68817127406ace5c764463543f699b
make
```

A clean runtime/layer/shader build in a temporary directory was checked using these
local headers and the installed compiler, glslang and loader. No prebuilt `.so` or
`.spv` files were reused.

## Weights

Supply the logical, 649-tensor file at `work/mlxw/dlssnr-logical.safetensors`, or pass
`--weights PATH` to the frame CLI. The reader verifies `fully_logical=true`.
The old `hnet_weights.py` output is not a substitute.

To extract from your own DLSS-NR DLL, the pinned upstream tools need NumPy and the
`safetensors` Python package. Run these with an interpreter where both are installed:

```sh
mkdir -p work/mlxw
python3 work/mlx-dlss/python/mlxdlss/tools/extract_dlssnr_weights.py /path/to/nvngx_dlssnr.dll work/mlxw/dlssnr-packed.safetensors
python3 work/mlx-dlss/python/mlxdlss/tools/unpack_dlssnr_weights.py work/mlxw/dlssnr-packed.safetensors work/mlxw/dlssnr-logical.safetensors
```

The older handoff's `PYTHONPATH=work/shim` is a local workaround, not a repository
dependency. The DLL and all derived weights stay outside commits.

## Run and verify

```sh
make test
python3 src/ref/nr_frame.py INPUT.png OUTPUT.png --resident
python3 src/ref/nr_temporal.py INPUT.png work/sequence --resident --frames 3
python3 src/bench/frame_replay.py --size 720 1280 --pairs 6 --input INPUT.png --json work/replay.json
```

The benchmark resizes its input, alternates mode order on the same allocations and
requires bit-identical outputs, including a changed frame. First-use allocation,
compilation and capture are separate from warm measurements. `make test` skips the
weight-dependent checks if local weights are absent; a skip is not full validation.

`NR_FRAME_MODE=block` restores the pre-replay submission strategy;
`NR_FRAME_MODE=single` records and submits once per frame; default `replay` captures
once and reuses the commands. `XMX_SPECIALIZE=0` additionally disables phase27's
shader specialization. Per-stage capture/timing always uses block mode.
