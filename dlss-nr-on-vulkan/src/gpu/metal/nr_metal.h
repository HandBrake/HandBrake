/*
 * nr_metal.h — what every Metal kernel here shares: the push block, the specialization
 * constant and the vendor's rounding points (`publish.glsl`, ported).
 *
 * These kernels are the GLSL of the `.comp` files in `src/gpu/` written in the Metal Shading Language,
 * pass for pass and flag for flag, so `libmetalmx` can take the very calls `libxmx`
 * takes. The contract is the numerics, not the syntax:
 *
 *   - The whole library is compiled with `-fno-fast-math -ffp-contract=off`. Metal's
 *     default is fast math *with* contraction, and that is the one setting that would
 *     silently break the port: the gate rounds to half between its multiply and its add,
 *     and an FMA skips that rounding (`notes/phase67` measured exactly this through
 *     MoltenVK: 3e-03 on the gate epilogue, a quantum on every E4M3 publish after it).
 *     Every `precise` in the GLSL is therefore the default here and needs no spelling.
 *   - `half_round` is `float(half(x))`: a real conversion instruction pair, not foldable
 *     without fast math on Apple GPUs, and round-to-nearest-even like `packHalf2x16` on
 *     Xe2. AMD's compiler folds it anyway, and there it is the bits (function constant 2).
 *     Round to half through `half_round`, never with a cast of one's own.
 *   - Buffers arrive as 64-bit GPU addresses in the push block (the buffer's device
 *     address, read back through an `MTLArgumentEncoder` so a macOS 11 SDK suffices,
 *     plus an element offset the runtime folds in), exactly as the SPIR-V takes
 *     `buffer_reference`s, so a pass needs no argument table and a block of dispatches
 *     records as push-and-dispatch.
 */
#ifndef NR_METAL_H
#define NR_METAL_H
#include <metal_stdlib>
using namespace metal;

/* The resident push block: `struct push` in libxmx.c / libmetalmx.m, byte for byte. */
struct Push {
    ulong a, b, c, d;
    uint m, n, k, batch;
    uint sa, sb, sc, flags;
    float p0, p1, p2, p3;
    uint lda, ldb, ldc, spare;
    /* Appended with the fusions, as in libxmx.c: the residual's per-channel cosine (and
     * the fifth operand of any pass that needs one: a second output, the attention bias,
     * the V target), the window-layout geometry, and the QKV epilogue's query scale (the
     * fused feed-forward's projection weights). 128 bytes in all. */
    ulong residual_cos;
    uint image_h, image_w, window_cols, window_pad;
    ulong qkv_scale;
};

/* Function constant 0 fixes the operation flags at pipeline creation, as specialization
 * constant 0 does for the SPIR-V (`specialize.glsl`): dead transpose, publish and width
 * branches then cost no registers. Left undefined, the flags come from the push block. */
constant uint SPECIALIZED_FLAGS [[function_constant(0)]];
constant bool specialized = is_function_constant_defined(SPECIALIZED_FLAGS);
inline uint operation_flags(constant Push &pc) { return specialized ? SPECIALIZED_FLAGS : pc.flags; }

/* -- publish.glsl -------------------------------------------------------- */

/* Function constant 2, set by libmetalmx where the cast does not round (its start-up probe,
 * `probe_half_rounding`): half_round on the float's bits. AMD's Metal compiler folds
 * float(half(x)) -- and every other spelling of the round trip, through half2, the bits or
 * a volatile -- to x, so every rounding point of the graph vanishes there; integer
 * arithmetic it cannot fold. Round to nearest even, half subnormals in steps of 2^-24,
 * 65520 and up to infinity, NaN kept. */
constant bool HALF_BY_BITS [[function_constant(2)]];
constant bool half_by_bits = is_function_constant_defined(HALF_BY_BITS) && HALF_BY_BITS;
/* Function constant 3, libxmx's `STAGED_PACKED` (gemm_staged.comp, specialization constant 2):
 * the staged kernel copies its operands global-to-threadgroup as raw 128-bit vectors where the
 * addresses allow it. The same halves in the same places, so the bytes are the same;
 * libmetalmx sets it on every pipeline (XMX_STAGED_PACKED=0 to compare). Function constant 4,
 * `PORTABLE_PACKED` (gemm_portable.comp's constant 3): the portable GEMM fetches A eight K
 * terms at a time — off by default, slower on an M3 (XMX_PORTABLE_PACKED=1 to try). */
constant bool STAGED_PACKED [[function_constant(3)]];
constant bool staged_packed = is_function_constant_defined(STAGED_PACKED) && STAGED_PACKED;
constant bool PORTABLE_PACKED [[function_constant(4)]];
constant bool portable_packed = is_function_constant_defined(PORTABLE_PACKED) && PORTABLE_PACKED;

inline float half_round_bits(float x) {
    uint bits = as_type<uint>(x), magnitude = bits & 0x7FFFFFFFu, sign = bits & 0x80000000u;
    if (magnitude > 0x7F800000u) return x;                                     // NaN
    if (magnitude >= 0x477FF000u) return as_type<float>(sign | 0x7F800000u);   // to infinity
    if (magnitude >= 0x38800000u)                                              // a normal half
        return as_type<float>(sign | ((magnitude + 0x0FFFu + ((magnitude >> 13) & 1u)) & ~0x1FFFu));
    uint exponent = magnitude >> 23;
    if (exponent < 101u) return as_type<float>(sign);                          // under 2^-25
    /* a subnormal half: the mantissa, its implicit bit included, in whole 2^-24 steps */
    uint mantissa = (magnitude & 0x7FFFFFu) | 0x800000u, drop = 126u - exponent;
    uint steps = (mantissa + ((1u << (drop - 1u)) - 1u) + ((mantissa >> drop) & 1u)) >> drop;
    return as_type<float>(sign | as_type<uint>(float(steps) * 5.9604644775390625e-08f));
}

inline float half_round(float x) { return half_by_bits ? half_round_bits(x) : float(half(x)); }

inline float e4m3(float x) {
    float magnitude = min(abs(x), 448.0f);
    int exponent = (as_type<int>(magnitude) >> 23) & 0xFF;
    exponent = max(exponent, 121) - 3;                 // 121-3 = the 2^-9 subnormal step
    float step = as_type<float>(exponent << 23);
    float reciprocal = as_type<float>((254 - exponent) << 23);
    float rounded = rint(magnitude * reciprocal) * step;  // rint: to nearest, ties to even
    return x < 0.0f ? -rounded : rounded;
}

inline float gate_activation(float x) {
    float wide = half_round(x);
    float clamped = clamp(wide, -4.0f, 4.0f);
    float linear = abs(clamped) * -0.055908203125f;
    linear += 0.447265625f;
    linear = half_round(linear);
    linear *= clamped;
    linear += 0.89453125f;
    linear = half_round(linear);
    return half_round(wide * linear);
}

/* The publish an epilogue applies: bits 8-11 of a pass's `flags` pick the transform. */
inline float publish(uint epilogue, float value) {
    if (epilogue == 2u || epilogue == 3u) value = gate_activation(value);
    if (epilogue == 1u || epilogue == 3u) value = e4m3(value);
    if (epilogue == 4u) value = half_round(value);
    return value;
}

/* The operands behind the addresses. */
inline device const half  *half_ptr(ulong address)  { return reinterpret_cast<device const half *>(address); }
inline device const float *float_ptr(ulong address) { return reinterpret_cast<device const float *>(address); }
inline device half  *half_out(ulong address)  { return reinterpret_cast<device half *>(address); }
inline device float *float_out(ulong address) { return reinterpret_cast<device float *>(address); }

/* An address carried in two adjacent 32-bit push fields, low half first (p0/p1, p2/p3,
 * lda/ldb), read as the one aligned 8-byte value it is. Joined from its halves instead --
 * `ulong(lo) | ulong(hi) << 32`, or `as_type<ulong>` of a two-element vector -- the
 * address makes AMD's Metal compiler fail ("Compiler encountered an internal error", a
 * Vega 64 on macOS 15) in any kernel that loads through it. */
template <typename T>
inline ulong push_address(constant T &low) { return *reinterpret_cast<constant ulong *>(&low); }
static_assert(__builtin_offsetof(Push, p0) % 8 == 0 && __builtin_offsetof(Push, p2) % 8 == 0
              && __builtin_offsetof(Push, lda) % 8 == 0, "push_address() reads 8 aligned bytes");

#endif
