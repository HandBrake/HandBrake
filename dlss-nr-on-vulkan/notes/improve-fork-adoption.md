# Selected changes from andyvand's Vulkan fork

Source: https://github.com/andyvand/dlss-nr-on-vulkan
Pinned revision: `1519a7a9035a94bc40f2d45d52caafe5a2ad9800` (Apache-2.0).
The source files and NOTICE credit the adaptation. No build products, dependency
checkouts, generated manifests or logs from the fork were imported.

## What was taken and changed

- CMake's shader/target/test organization and dependency discovery were adapted to
  this branch's Linux/XMX sources. Output names remain compatible with the Python
  runtime. The optional C frame library, portable shaders and platform-specific
  runtime changes are not present, so the build explicitly rejects other platforms
  instead of advertising support the sources do not implement.
- The GEMM contract test was adapted to the existing runtime. It covers batched and
  transposed products, all epilogues with float/half outputs, explicit row strides
  and offsets, and descriptor-based GEMMs. Added a shared-memory-staged shape and
  exact guard checks outside a destination slice. Epilogue inputs are multiples of
  1/16 so their dot products are exactly representable and the epilogue comparison
  can require equality instead of guessing a tolerance from the output's spacing.
- Both Make and CTest run the added GEMM test. CTest keeps tests serial: concurrent
  GPU/daemon tests can exhaust the iGPU's shared memory or interfere with timing.
- Release builds keep C test assertions enabled. Host image passes retain the same
  FP rounding flags as Make, and `-march=native` is not used for cross compilation.

## Scope of the next experiment

The fork's C frame API is a useful integration interface, but its graph still records
the same operations and its input/output buffers still use sixteen float32 channels.
Importing it alone does not remove PCIe traffic or the current daemon/socket round trip.
It also has a second graph implementation that would need the FFN batching change.
Evaluate it separately with equal input features and end-to-end frame timings; do not
attribute a speedup to removing Python without measuring which host work disappeared.

## Validation

Clean CMake configure/build completed with GCC 16.2.1. All 23 CTest entries passed
on Intel Graphics (LNL), including full-frame execution and Vulkan presentation;
total suite time was 72.97 seconds. The new GEMM checks also passed separately with
`XMX_STAGING=1`; all ten epilogue comparisons were exact and the destination guards
remained intact. A compute-only configuration with the layer and tests disabled
also configured and built successfully. `claims_check.py` and `git diff --check`
passed. No B580, macOS or Windows run is claimed by this adaptation.
