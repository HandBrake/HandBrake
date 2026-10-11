# Cooperative matrix capability — VERIFIED

Probed 2026-09-07 with `src/probe/coopmat_probe.c` (built against Khronos
Vulkan-Headers v1.4.321, run against the system loader). Raw output:
`notes/coopmat-table.txt`. This supersedes the "open questions 1–3" in notes/CLAUDE.md.

Device: `Intel(R) Graphics (LNL)`, Vulkan 1.4.354, Mesa ANV. Single physical device.

## Feature state

| property | value |
|---|---|
| `subgroupSize` | 32 |
| `cooperativeMatrix` | true |
| `cooperativeMatrixRobustBufferAccess` | **false** |
| `cooperativeMatrixSupportedStages` | compute only |
| `VK_KHR_shader_bfloat16` | present |
| `shaderBFloat16Type` | true |
| `shaderBFloat16CooperativeMatrix` | true |
| `shaderBFloat16DotProduct` | true |
| `VK_NV_cooperative_matrix2` | advertised |

## The configuration table (open question 1 — ANSWERED)

`vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR` returns **6** configs.
Every one is scope = `subgroup`, `saturatingAccumulation` = false, M = 8, N = 16.

| idx | M | N | K | A | B | C | Result |
|---|---|---|---|---|---|---|---|
| 0 | 8 | 16 | 16 | fp16 | fp16 | fp16 | fp16 |
| 1 | 8 | 16 | 16 | fp16 | fp16 | fp32 | fp32 |
| 2 | 8 | 16 | 16 | bf16 | bf16 | bf16 | bf16 |
| 3 | 8 | 16 | 16 | bf16 | bf16 | **fp32** | **fp32** | ← primary path |
| 4 | 8 | 16 | 32 | sint8 | sint8 | sint32 | sint32 |
| 5 | 8 | 16 | 32 | uint8 | uint8 | uint32 | uint32 |

## FP32 accumulation for BF16 (open question 3 — ANSWERED: yes)

Config 3 is exactly what the project needs: BF16 inputs, FP32 accumulator and
FP32 result. No need to fall back to BF16 accumulation, and no need to consider
FP16 for numerical-range reasons.

## OpenCL cross-check (open question 2 — ANSWERED)

`clinfo` on `Intel(R) Arc(TM) Graphics` reports both:

- `cl_intel_subgroup_matrix_multiply_accumulate`
- `cl_intel_subgroup_matrix_multiply_accumulate_tf32`
- (also `cl_intel_bfloat16_conversions`, `cl_intel_subgroup_2d_block_io`)

So the DPAS hardware *does* have TF32, but **Mesa does not expose TF32 through
`VK_KHR_cooperative_matrix`** — it is not in the 6-config table. Same for INT4/INT2,
which notes/CLAUDE.md lists as XMX capabilities: not reachable from Vulkan.
Reaching TF32 would mean an OpenCL path, which is a different backend, not a flag.

## Consequences for Phase 4 — read these before writing shaders

1. **One shape, no choices.** 8×16×16 is the only float shape. GEMM blocking is
   fixed; there is no shape-selection tuning knob. Tile dimensions must be
   multiples of M = 8, N = 16, K = 16.
2. **`cooperativeMatrixRobustBufferAccess = false`.** Out-of-bounds cooperative
   matrix loads are undefined behaviour, not zero-fill. Combined with the mandatory
   tiling from the memory constraint, every edge tile must be explicitly padded or
   guarded in our own code. This is a correctness issue, not a performance one —
   design it in from the first shader.
3. **`VK_NV_cooperative_matrix2` is advertised but empty for our purposes.**
   `vkGetPhysicalDeviceCooperativeMatrixFlexibleDimensionsPropertiesNV` returns
   **count = 0**. There are no flexible-dimension configs, so the coopmat2
   larger-tile / workgroup-scope path is not available via that mechanism.
   *(Not yet checked: `VkPhysicalDeviceCooperativeMatrix2FeaturesNV` — the extension
   also carries reduction/conversion ops that may still be usable. Worth a look
   before finalising the Phase 4 design.)*
4. Compute stage only, as notes/CLAUDE.md already assumed. Confirmed, not inferred.
