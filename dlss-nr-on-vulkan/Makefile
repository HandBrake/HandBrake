# Build the resident Vulkan runtime, game layer and compute shaders.
# The shaders are plain GLSL compiled to SPIR-V; libxmx keeps the Vulkan context alive
# across calls so a block records as one command buffer.

GLSL    := glslangValidator --target-env vulkan1.3 -Isrc/gpu
# The interpreter the tests run under: `make test PYTHON=python3.12` to pick another.
PYTHON  ?= python3
# The CPU the host passes are built for. Upstream's floor is x86-64-v3 — every CPU since
# Haswell and Zen 1 — whose F16C makes each `_Float16` conversion one instruction; without it
# each is a call into libgcc, and a frame was 5-13 % slower. On Lunar Lake v3 is as fast as
# -march=native and byte-identical, and a release built with it runs on other machines. This
# tree also builds on arm64 (Apple silicon, Android), where that flag means nothing, so the
# floor is the host's architecture: v3 on x86-64, and nothing on the rest. arm64 needs none:
# FCVT between half and single is base ARMv8.0, so `_Float16` is one instruction there from
# any -march, and a compiler without the type takes NEON's by hand (nr_portable.h).
# CMakeLists.txt uses the same rule (build_check.py); `make NR_IMAGE_ARCH=-march=native`
# overrides it.
UNAME_M := $(shell uname -m)
ifneq ($(filter x86_64 amd64,$(UNAME_M)),)
NR_IMAGE_ARCH ?= -march=x86-64-v3
else
NR_IMAGE_ARCH ?=
endif
# This build lands in work/; CMake lands in its build directory. The Python finds either
# through `src/nr_build.py`, newest build first, and this pins it for the targets below so
# `make test` runs what `make` built even when a CMake build is more recent.
export NR_BUILD_DIR ?= $(CURDIR)/work

# Where the Vulkan headers and the loader are. Linux: the pinned clone in `work/`, or the
# distribution's. macOS: MoltenVK, which vcpkg or a MoltenVK install put under one of the
# prefixes below — override VK_PREFIX (or VK_INC / VK_LIBDIR / MVK_LIBDIR one by one).
UNAME_S := $(shell uname -s)
# Shared libraries carry the platform's suffix: .dylib on macOS, .so elsewhere. The Python
# loaders, the layer manifest and the frame library's dlopen make the same choice.
ifeq ($(UNAME_S),Darwin)
SO := .dylib
# an executable finds the dylib through its rpath from any directory, not only the root
INSTALL_NAME = -Wl,-install_name,@rpath/$(notdir $@)
else
SO := .so
INSTALL_NAME :=
endif
ifeq ($(UNAME_S),Darwin)
VK_PREFIX ?= $(firstword $(foreach d,$(HOME)/Downloads/vcpkg/installed/arm64-osx /usr/local /opt/homebrew $(VULKAN_SDK),$(if $(wildcard $(d)/include/vulkan/vulkan.h),$(d),)))
VK_INC    ?= $(VK_PREFIX)/include
VK_LIBDIR ?= $(VK_PREFIX)/lib
MVK_LIBDIR ?= $(firstword $(foreach d,/usr/local/lib $(VK_LIBDIR) /opt/homebrew/lib,$(if $(wildcard $(d)/libMoltenVK.dylib),$(d),)))
# The compute runtime talks to MoltenVK directly: no loader, no ICD file, no portability
# enumeration to arrange. The layer and its tests go through the loader by definition,
# and the loader finds MoltenVK through the ICD manifest `all` writes into work/.
XMX_LIBS := -L$(MVK_LIBDIR) -lMoltenVK -Wl,-rpath,$(MVK_LIBDIR)
VK_LIBS  := -L$(VK_LIBDIR) -lvulkan -Wl,-rpath,$(VK_LIBDIR)
# libmetalmx: libxmx's entry points on Metal directly, with the shaders compiled by the
# Metal compiler into one metallib that bin2c puts inside the library. Apple only — the
# other platforms have no Metal and never see these targets. `NR_GPU_BACKEND=metal`
# makes the Python and libnr_frame load it instead of libxmx.
METAL      := xcrun -sdk macosx metal -std=metal3.1 -fno-fast-math -ffp-contract=off -Wall -Isrc/gpu/metal
METALLIB   := xcrun -sdk macosx metallib
# Since Xcode 15 the Metal compiler is a separate download (`xcodebuild -downloadComponent
# MetalToolchain`); without it the Vulkan build still goes through and libmetalmx is skipped.
HAVE_METAL := $(shell xcrun -sdk macosx -f metal 2>/dev/null)
ifneq ($(HAVE_METAL),)
PLATFORM_EXTRA := work/MoltenVK_icd.json work/libmetalmx$(SO) work/nr_shaders.metallib
PLATFORM_TESTS := test-metal
else
$(warning no Metal compiler (xcrun -f metal): libmetalmx is not built; install the Metal toolchain for it)
PLATFORM_EXTRA := work/MoltenVK_icd.json
PLATFORM_TESTS :=
endif
DL_LIBS :=
else
VK_INC   ?= work/vulkan-headers/include
XMX_LIBS := -lvulkan
VK_LIBS  := -lvulkan
PLATFORM_EXTRA :=
PLATFORM_TESTS :=
DL_LIBS := -ldl
endif
CFLAGS  := -O2 -fPIC -Wall -Wextra -Wno-unused-parameter -I$(VK_INC)
SHADERS := work/gemm_resident.spv work/gemm_tiled.spv work/gemm_staged.spv \
           work/gemm_staged32.spv work/gemm_staged32_deep.spv work/attention_rows.spv \
           work/resident.spv work/attention.spv \
           work/history.spv work/gemm_coopmat.spv work/gemm_batched.spv \
           work/window_attention.spv work/window_attention_portable.spv \
           work/attention_ab.spv \
           work/ffn_fused.spv work/ffn_fused_portable.spv \
           work/gemm_f16acc.spv work/gemm_coopmat_int8.spv work/gemm_staged_int8.spv \
           work/gemm_staged_int8_portable.spv \
           work/window_block.spv work/window_block_portable.spv \
           work/global_attention.spv work/global_attention_portable.spv \
           work/gemm_portable.spv work/gemm_portable_tiled.spv work/gemm_portable_wide.spv \
           work/gemm_portable_desc.spv work/gemm_portable_batched.spv

# half_probe.spv is built by `all` because the daemon needs it: check_half_rounding()
# runs it at start-up and refuses to run the graph if the driver's float16 conversion is
# not what the build compiled in. It was only reachable through `bench`, so a plain
# `make` left the daemon to start with the probe skipped.
#
# The 32-bit layer is for 32-bit games under Proton (Dead or Alive 5 is one). Only
# `nr-photo` used to build it, so a game started from Steam kept running whatever layer was
# built last. `all` builds it wherever a 32-bit compiler and a 32-bit Vulkan loader are
# installed (Linux; macOS has had no 32-bit toolchain since 10.15). LAYER32=0 leaves it out;
# LAYER32=1 builds it regardless, and fails loudly where it cannot.
LAYER32 ?= auto
ifeq ($(LAYER32),auto)
LAYER32_TARGET := $(shell printf 'int main(void) { return 0; }' | $(CC) -m32 -x c - -o /dev/null -lvulkan >/dev/null 2>&1 && echo work/libnr_layer32$(SO))
else ifeq ($(LAYER32),1)
LAYER32_TARGET := work/libnr_layer32$(SO)
endif

all: work/libxmx$(SO) work/libnr_layer$(SO) $(LAYER32_TARGET) work/libnr_image$(SO) work/libnr_frame$(SO) work/nr_frame work/nr_frame_rates work/gemm_runner work/half_probe.spv $(SHADERS) $(PLATFORM_EXTRA)

work:
	mkdir -p $@

$(SHADERS) work/libxmx$(SO) work/libnr_layer$(SO) work/half_probe.spv work/attention_ab.spv: | work
work/libnr_image$(SO) work/test_exchange work/test_settled work/libnr_layer32$(SO) work/test_layer_loader work/test_present work/test_layer_loader32: | work

work/libxmx$(SO): src/gpu/libxmx.c
	$(CC) $(CFLAGS) -shared -o $@ $< $(XMX_LIBS)

# The one-shot runner behind src/gpu/test_gemm.py: its own instance and device per call.
work/gemm_runner: src/gpu/gemm_runner.c | work
	$(CC) $(CFLAGS) -o $@ $< $(XMX_LIBS)

# The Metal runtime (Apple only; see PLATFORM_EXTRA). Each .metal compiles to AIR, the
# seven link into one metallib, bin2c writes it as a C array, and the array is compiled into
# libmetalmx. The metallib file is kept beside it for XMX_METALLIB= experiments.
METAL_SOURCES := resident history attention gemm_portable gemm_simd window_attention ffn_fused \
                 window_block global_attention gemm_int8 half_probe
METAL_AIR     := $(patsubst %,work/metal/%.air,$(METAL_SOURCES))
work/metal:
	mkdir -p $@
work/metal/%.air: src/gpu/metal/%.metal src/gpu/metal/nr_metal.h src/gpu/metal/nr_epilogue.h | work/metal
	$(METAL) -c -o $@ $<
work/nr_shaders.metallib: $(METAL_AIR) | work
	$(METALLIB) -o $@ $(METAL_AIR)
work/bin2c: src/tools/bin2c.c | work
	$(CC) -O2 -o $@ $<
work/metal/nr_metallib.h: work/nr_shaders.metallib work/bin2c | work/metal
	work/bin2c $< $@ nr_metallib
# The same sources as Metal 4 (NR_METAL4: the GEMMs' K loop on matmul2d), a second library
# libmetalmx loads where the device and OS have Metal 4 — when this SDK's compiler takes
# -std=metal4.0. METAL4=0 builds the Metal 3.1 library alone.
METAL4     ?= $(shell printf '\043include <metal_stdlib>\nkernel void p(device float *a [[buffer(0)]]) { a[0] = 1; }\n' \
                | xcrun -sdk macosx metal -std=metal4.0 -x metal -c -o /dev/null - 2>/dev/null && echo 1)
ifeq ($(METAL4),1)
METAL4_AIR    := $(patsubst %,work/metal4/%.air,$(METAL_SOURCES))
METAL4_HEADER := work/metal4/nr_metallib4.h
METAL4_FLAGS  := -DNR_EMBEDDED_METALLIB4=1 -Iwork/metal4
work/metal4:
	mkdir -p $@
work/metal4/%.air: src/gpu/metal/%.metal src/gpu/metal/nr_metal.h src/gpu/metal/nr_epilogue.h | work/metal4
	xcrun -sdk macosx metal -std=metal4.0 -DNR_METAL4 -fno-fast-math -ffp-contract=off -Wall -Isrc/gpu/metal -c -o $@ $<
work/nr_shaders4.metallib: $(METAL4_AIR) | work
	$(METALLIB) -o $@ $(METAL4_AIR)
work/metal4/nr_metallib4.h: work/nr_shaders4.metallib work/bin2c | work/metal4
	work/bin2c $< $@ nr_metallib4
endif
work/libmetalmx$(SO): src/gpu/libmetalmx.m src/gpu/xmx.h work/metal/nr_metallib.h $(METAL4_HEADER) | work
	$(CC) $(CFLAGS) -fobjc-arc -DNR_EMBEDDED_METALLIB=1 $(METAL4_FLAGS) -Isrc/gpu -Iwork/metal -shared $(INSTALL_NAME) \
	      -o $@ $< -framework Metal -framework Foundation

# The GPU tests again, on libmetalmx. Apple only; `make test` runs it there.
test-metal: work/libmetalmx$(SO)
	NR_GPU_BACKEND=metal $(PYTHON) src/gpu/test_portable.py
	NR_GPU_BACKEND=metal $(PYTHON) src/gpu/test_denorm.py
	NR_GPU_BACKEND=metal XMX_PORTABLE=1 $(PYTHON) src/gpu/test_portable.py
	NR_GPU_BACKEND=metal XMX_STAGING=1 $(PYTHON) src/gpu/test_graph.py
	NR_GPU_BACKEND=metal $(PYTHON) src/gpu/test_epilogue.py
	NR_GPU_BACKEND=metal $(PYTHON) src/gpu/test_specialization.py
	NR_GPU_BACKEND=metal $(PYTHON) src/gpu/test_softmax_pack.py
	NR_GPU_BACKEND=metal $(PYTHON) src/gpu/test_qkv_fusion.py
	NR_GPU_BACKEND=metal $(PYTHON) src/gpu/test_scratch_arena.py
	NR_GPU_BACKEND=metal $(PYTHON) src/gpu/test_graph.py
	NR_GPU_BACKEND=metal $(PYTHON) src/gpu/test_frame_execution.py
	NR_GPU_BACKEND=metal $(PYTHON) src/gpu/test_resident.py
	NR_GPU_BACKEND=metal $(PYTHON) src/gpu/test_staged_packed.py
	NR_GPU_BACKEND=metal XMX_PORTABLE=1 $(PYTHON) src/gpu/test_staged_packed.py
	NR_GPU_BACKEND=metal $(PYTHON) src/ref/test_nr_frame_c.py --reference work/nr_frame_reference_metal.bin
	NR_GPU_BACKEND=metal work/test_nr_frame --reference work/nr_frame_reference_metal.bin
	NR_GPU_BACKEND=metal $(PYTHON) src/ref/test_nr_frame_live.py

# For the Vulkan loader on macOS, which finds no driver on its own: point VK_DRIVER_FILES
# (older loaders: VK_ICD_FILENAMES) at this file. The tests that go through the loader do.
work/MoltenVK_icd.json: Makefile | work
	printf '{\n  "file_format_version": "1.0.0",\n  "ICD": {\n    "library_path": "%s/libMoltenVK.dylib",\n    "api_version": "1.4.0",\n    "is_portability_driver": true\n  }\n}\n' "$(MVK_LIBDIR)" > $@

# The host passes in C, for `NR_IMAGE_ARCH` above. The FP16 casts and the separate multiply
# and add are the contract — fused multiply-add or fast maths would change the last bit and
# the output must be byte-identical to the NumPy it replaces (src/ref/test_native_image.py).
# Each pass is split by rows across nr_image.c's own thread pool (upstream uses OpenMP, which
# Apple's clang lacks); no row reads another's result, so the bytes do not depend on it.
# `-fno-trapping-math` changes no value — only whether a comparison may raise a floating-
# point exception flag nobody reads — and it is what lets the compiler turn the clamps into
# selects and vectorise the fused composition's pixels (`compose_encode_row`).
#
# `work/nr_image.arch` holds the flags the library was last built with, so one built with
# other flags (NR_IMAGE_ARCH given by hand) is rebuilt by the next `make` rather than called
# up to date by its time. That is how a release build left a checkout's library without F16C.
work/nr_image.arch: FORCE | work
	@echo '$(NR_IMAGE_ARCH)' | cmp -s - $@ || echo '$(NR_IMAGE_ARCH)' > $@
FORCE:
work/libnr_image$(SO): src/ref/nr_image.c src/ref/nr_image.h Makefile work/nr_image.arch | work
	$(CC) -O3 $(NR_IMAGE_ARCH) -fPIC -Wall -Wextra -ffp-contract=off -fno-fast-math \
	      -fno-trapping-math -pthread -shared -o $@ $< -lm

# nr_frame.py as a C library: the feature assembly, the whole graph recorded against
# libxmx (reached by dlopen from this directory), and the composition. The image passes
# are the same object as libnr_image; the frame code has the same no-FMA contract.
work/nr_image.o: src/ref/nr_image.c src/ref/nr_image.h work/nr_image.arch | work
	$(CC) -O3 $(NR_IMAGE_ARCH) -fPIC -Wall -Wextra -ffp-contract=off -fno-fast-math -fno-trapping-math -pthread -c -o $@ $<
work/nr_frame.o: src/ref/nr_frame.c src/ref/nr_frame.h src/ref/nr_image.h | work
	$(CC) -O2 -fPIC -Wall -Wextra -Wno-unused-parameter -ffp-contract=off -fno-fast-math -c -o $@ $<
work/libnr_frame$(SO): work/nr_frame.o work/nr_image.o
	$(CC) -shared $(INSTALL_NAME) -o $@ $^ $(DL_LIBS) -lm -pthread
# nr_frame.py, the command, in C: the same flags, PNG in and out through libpng.
# pkg-config finds libpng where it is; PNG_CFLAGS / PNG_LIBS override it.
# pkg-config first; then the prefixes a Mac keeps it under (Homebrew, vcpkg, /usr/local);
# then the plain -lpng a distribution's package gives.
# vcpkg's .pc files are on no default pkg-config path, so they are added to the search.
# A prefix counts only if its png.h can be read: a dangling symlink (a Mac after an old
# build tree was deleted) would otherwise pass $(wildcard) and fail at the #include.
PNG_PKG_CONFIG := PKG_CONFIG_PATH="$$PKG_CONFIG_PATH:$(HOME)/Downloads/vcpkg/installed/arm64-osx/lib/pkgconfig" pkg-config
PNG_PREFIX ?= $(firstword $(foreach d,/opt/homebrew /usr/local $(HOME)/Downloads/vcpkg/installed/arm64-osx,$(if $(shell test -r $(d)/include/png.h && echo y),$(d),)))
PNG_CFLAGS ?= $(or $(shell $(PNG_PKG_CONFIG) --cflags libpng 2>/dev/null),$(if $(PNG_PREFIX),-I$(PNG_PREFIX)/include,))
PNG_LIBS   ?= $(or $(shell $(PNG_PKG_CONFIG) --libs libpng 2>/dev/null),$(if $(PNG_PREFIX),-L$(PNG_PREFIX)/lib -lpng -lz -Wl$(COMMA)-rpath$(COMMA)$(PNG_PREFIX)/lib,-lpng -lz))
COMMA := ,
work/nr_frame: src/ref/nr_frame_main.c work/libnr_frame$(SO) src/ref/nr_frame.h src/ref/nr_image.h
	$(CC) -O2 -Wall -Wextra -Isrc/ref $(PNG_CFLAGS) -o $@ $< work/libnr_frame$(SO) -Wl,-rpath,$(CURDIR)/work $(PNG_LIBS) $(DL_LIBS) -lm
# live_rates.py for the C library: the daemon's frame, end to end, in one process
work/nr_frame_rates: src/bench/nr_frame_rates.c work/libnr_frame$(SO) src/ref/nr_frame.h src/ref/nr_image.h
	$(CC) -O2 -Wall -Wextra -Isrc/ref -o $@ $< work/libnr_frame$(SO) -Wl,-rpath,$(CURDIR)/work -lm
# test_nr_frame_c.py in C: the library against its own contract, and against the head the
# Python test writes with --reference, so the bit-identity check survives without Python
work/test_nr_frame: src/ref/test_nr_frame.c work/libnr_frame$(SO) src/ref/nr_frame.h
	$(CC) -O2 -Wall -Wextra -ffp-contract=off -fno-fast-math -Isrc/ref -o $@ $< work/libnr_frame$(SO) -Wl,-rpath,$(CURDIR)/work -lm

# The Vulkan layer that puts the pass inside a running game.
work/libnr_layer$(SO): src/layer/nr_layer.c src/layer/nr_transport.h
	$(CC) $(CFLAGS) -shared -o $@ $< $(VK_LIBS)

work/libnr_layer32$(SO): src/layer/nr_layer.c src/layer/nr_transport.h
	$(CC) $(CFLAGS) -m32 -shared -o $@ $< -lvulkan

work/test_layer_loader: src/layer/test_layer_loader.c
	$(CC) $(CFLAGS) -o $@ $< $(VK_LIBS)

work/test_present: src/layer/test_present.c
	$(CC) $(CFLAGS) -o $@ $< $(VK_LIBS)

work/test_layer_loader32: src/layer/test_layer_loader.c
	$(CC) $(CFLAGS) -m32 -o $@ $< -lvulkan

work/test_settled: src/layer/test_settled.c src/layer/nr_layer.c src/layer/nr_transport.h
	$(CC) $(CFLAGS) -Isrc/layer -o $@ $< $(VK_LIBS)

work/test_exchange: src/layer/test_exchange.c src/layer/nr_layer.c src/layer/nr_transport.h
	$(CC) $(CFLAGS) -o $@ $< $(VK_LIBS) -lpthread

GEMM_GLSL := src/gpu/publish.glsl src/gpu/specialize.glsl src/gpu/residual_epilogue.glsl \
             src/gpu/cosine_tree.glsl src/gpu/qkv_epilogue.glsl
work/gemm_resident.spv: src/gpu/gemm_resident.comp $(GEMM_GLSL)
	$(GLSL) -o $@ $<
# the same source, with a 16x32 block of the output held in one subgroup's registers
work/gemm_tiled.spv: src/gpu/gemm_resident.comp $(GEMM_GLSL) Makefile
	$(GLSL) -DRM=2 -DRN=2 -o $@ $<
work/gemm_staged.spv: src/gpu/gemm_staged.comp $(GEMM_GLSL)
	$(GLSL) -o $@ $<
# 32-row blocks for a bottleneck of 32 tokens or fewer, with a 32- and a 64-deep K step
work/gemm_staged32.spv: src/gpu/gemm_staged.comp $(GEMM_GLSL) Makefile
	$(GLSL) -DSTAGED_BM=32 -o $@ $<
work/gemm_staged32_deep.spv: src/gpu/gemm_staged.comp $(GEMM_GLSL) Makefile
	$(GLSL) -DSTAGED_BM=32 -DSTAGED_BK=64 -o $@ $<
work/resident.spv: src/gpu/resident.comp src/gpu/publish.glsl src/gpu/specialize.glsl
	$(GLSL) -o $@ $<
work/attention.spv: src/gpu/attention.comp src/gpu/publish.glsl src/gpu/specialize.glsl \
                    src/gpu/cosine_tree.glsl
	$(GLSL) -o $@ $<
# the whole-row softmax on 256 lanes (attention.comp, softmax_rows)
work/attention_rows.spv: src/gpu/attention.comp src/gpu/publish.glsl src/gpu/specialize.glsl \
                         src/gpu/cosine_tree.glsl Makefile
	$(GLSL) -DROW_LANES=256 -o $@ $<
work/attention_ab.spv: src/gpu/attention.comp src/gpu/publish.glsl src/gpu/specialize.glsl \
                       src/gpu/cosine_tree.glsl
	$(GLSL) -DSOFTMAX_AB -o $@ $<
work/history.spv: src/gpu/history.comp
	$(GLSL) -o $@ $<
# Configuration 4, the integer twin. Built always; used only where
# notes/improve-int8-bottleneck.md measured the trade as worth taking.
work/gemm_coopmat_int8.spv: src/gpu/gemm_coopmat_int8.comp
	$(GLSL) -o $@ $<
work/gemm_staged_int8.spv: src/gpu/gemm_staged_int8.comp
	$(GLSL) -o $@ $<
work/window_block.spv: src/gpu/window_block.comp src/gpu/publish.glsl src/gpu/specialize.glsl \
                       src/gpu/cosine_tree.glsl
	$(GLSL) -o $@ $<
work/global_attention.spv: src/gpu/global_attention.comp src/gpu/publish.glsl \
                           src/gpu/specialize.glsl src/gpu/cosine_tree.glsl
	$(GLSL) -o $@ $<
# their twins for a device without matrix units (or without the integer features)
work/gemm_staged_int8_portable.spv: src/gpu/gemm_staged_int8_portable.comp
	$(GLSL) -o $@ $<
work/window_block_portable.spv: src/gpu/window_block_portable.comp src/gpu/publish.glsl \
                                src/gpu/specialize.glsl src/gpu/cosine_tree.glsl
	$(GLSL) -o $@ $<
work/global_attention_portable.spv: src/gpu/global_attention_portable.comp src/gpu/publish.glsl \
                                    src/gpu/specialize.glsl src/gpu/cosine_tree.glsl
	$(GLSL) -o $@ $<

# A 32-channel block's feed-forward in one pass, the hidden layer kept on chip.
work/ffn_fused.spv: src/gpu/ffn_fused.comp $(GEMM_GLSL)
	$(GLSL) -o $@ $<
work/ffn_fused_portable.spv: src/gpu/ffn_fused_portable.comp $(GEMM_GLSL)
	$(GLSL) -o $@ $<

# Window attention's QK^T, softmax and PV in one pass (ProjectsCodex's phase42), and its
# twin for a device without matrix units.
work/window_attention.spv: src/gpu/window_attention.comp src/gpu/publish.glsl
	$(GLSL) -o $@ $<
work/window_attention_portable.spv: src/gpu/window_attention_portable.comp src/gpu/publish.glsl
	$(GLSL) -o $@ $<
work/gemm_coopmat.spv: src/gpu/gemm_coopmat.comp
	$(GLSL) -o $@ $<
work/gemm_batched.spv: src/gpu/gemm_coopmat_batched.comp
	$(GLSL) -o $@ $<
work/gemm_f16acc.spv: src/gpu/gemm_coopmat_f16acc.comp
	$(GLSL) -o $@ $<
# the same GEMMs without the cooperative matrix, for a device that has none
work/gemm_portable.spv: src/gpu/gemm_portable.comp $(GEMM_GLSL)
	$(GLSL) -o $@ $<
work/gemm_portable_tiled.spv: src/gpu/gemm_portable.comp $(GEMM_GLSL) Makefile
	$(GLSL) -DRM=2 -DRN=2 -o $@ $<
# 16x64: libxmx finds it beside the tiled build and routes the wide-N GEMMs to it
work/gemm_portable_wide.spv: src/gpu/gemm_portable.comp $(GEMM_GLSL) Makefile
	$(GLSL) -DRM=2 -DRN=4 -o $@ $<
work/gemm_portable_desc.spv: src/gpu/gemm_portable_desc.comp
	$(GLSL) -o $@ $<
work/gemm_portable_batched.spv: src/gpu/gemm_portable_desc.comp
	$(GLSL) -DBATCHED -o $@ $<

work/half_probe.spv: src/bench/half_probe.comp
	$(GLSL) -o $@ $<

bench: all work/half_probe.spv
	$(PYTHON) src/bench/half_probe.py
	$(PYTHON) src/bench/split_cost.py

test: all work/attention_ab.spv work/test_exchange work/test_settled work/test_present work/test_nr_frame $(PLATFORM_TESTS)
	$(PYTHON) src/gpu/test_ffn_batch.py --gpu
	$(PYTHON) src/gpu/test_gemm_contract.py
	$(PYTHON) src/gpu/test_denorm.py
	$(PYTHON) src/gpu/test_input_fp16.py
	$(PYTHON) src/gpu/test_compact_head.py
	$(PYTHON) src/gpu/test_joint_qkv.py
	$(PYTHON) src/layer/test_daemon.py
	work/test_settled
	$(PYTHON) src/layer/test_ui_mask.py
	$(PYTHON) src/layer/test_present.py
	$(PYTHON) src/layer/test_temporal.py
	$(PYTHON) src/layer/test_toggle.py
	$(PYTHON) src/layer/test_panel.py
	$(PYTHON) src/tools/publish_check.py
	$(PYTHON) src/tools/claims_check.py
	$(PYTHON) src/tools/build_check.py
	$(PYTHON) src/tools/test_release.py
	$(PYTHON) src/bench/frame_profile.py --tables
	$(PYTHON) src/gpu/test_portable.py
	XMX_PORTABLE=1 $(PYTHON) src/gpu/test_portable.py
	$(PYTHON) src/gpu/test_epilogue.py
	$(PYTHON) src/gpu/test_specialization.py
	$(PYTHON) src/gpu/test_softmax_pack.py
	$(PYTHON) src/gpu/test_qkv_fusion.py
	$(PYTHON) src/gpu/test_scratch_arena.py
	$(PYTHON) src/gpu/test_graph.py
	$(PYTHON) src/gpu/test_frame_execution.py
	$(PYTHON) src/gpu/test_resident.py
	$(PYTHON) src/ref/test_frame_cache.py
	$(PYTHON) src/ref/test_native_image.py
	NR_HOST_THREADS=1 $(PYTHON) src/ref/test_native_image.py
	NR_HOST_THREADS=3 $(PYTHON) src/ref/test_native_image.py
	$(PYTHON) src/ref/test_nr_frame_c.py --reference work/nr_frame_reference.bin
	work/test_nr_frame --reference work/nr_frame_reference.bin
	$(PYTHON) src/ref/test_nr_frame_live.py
	$(PYTHON) src/ref/test_nr_model.py
	$(PYTHON) src/ref/test_temporal_controls.py
	$(PYTHON) src/gpu/test_gemm_int8.py
	$(PYTHON) src/gpu/test_gemm_int8_staged.py
	$(PYTHON) src/gpu/test_window_block.py
	$(PYTHON) src/gpu/test_global_attention.py
	$(PYTHON) src/gpu/test_int8_quant.py
	$(PYTHON) src/gpu/test_gemm_residual.py
	$(PYTHON) src/gpu/test_window_residual.py
	$(PYTHON) src/gpu/test_window_attention.py
	$(PYTHON) src/gpu/test_gemm_qkv.py
	$(PYTHON) src/gpu/test_glue.py
	$(PYTHON) src/gpu/test_ffn_fused.py
	$(PYTHON) src/gpu/test_staged_partial.py
	$(PYTHON) src/gpu/test_staged32.py
	$(PYTHON) src/gpu/test_staged_packed.py

# Focused checks for the FFN schedule, including a complete frame with both variants.
# Repeat with XMX_STAGING=1 to cover device buffers without host mappings.
test-ffn: all
	$(PYTHON) src/gpu/test_ffn_batch.py --gpu
	$(PYTHON) src/gpu/test_frame_execution.py

test-proton: all work/libnr_layer32$(SO) work/test_layer_loader work/test_layer_loader32
	$(PYTHON) src/layer/test_launcher.py
	$(PYTHON) src/layer/prepare_layer.py work/layer-check
	VK_LAYER_PATH=$(CURDIR)/work/layer-check ENABLE_NR_LAYER=1 work/test_layer_loader
	VK_LAYER_PATH=$(CURDIR)/work/layer-check ENABLE_NR_LAYER=1 work/test_layer_loader32

# Everything the tracked tree must not contain, and — slower — everything the history
# must not either. `make test` runs the first; run the second before publishing.
publish-check:
	$(PYTHON) src/tools/publish_check.py --history

# The prepared weights (`nr_frame --prepare`): the logical file with the loaders' derived
# tensors, F32 first then F16, which the Python graph and the C library both read and which
# the CMake build compiles in (weights/). make embeds nothing itself.
prepare-weights: work/nr_frame
	mkdir -p work/mlxw
	work/nr_frame --prepare work/mlxw/dlssnr-prepared.safetensors $(if $(wildcard work/mlxw/dlssnr-logical.safetensors),--weights work/mlxw/dlssnr-logical.safetensors,)

.PHONY: all test test-metal test-ffn test-proton bench publish-check prepare-weights
