/*
 * nr_d3d.hlsli — what every HLSL kernel here shares: the push block, the four operands,
 * the vendor's rounding points (`publish.glsl`, ported) and the typed access behind a
 * byte offset.
 *
 * These kernels are the GLSL of the `.comp` files in `src/gpu/` written in HLSL, pass for
 * pass and flag for flag, so `libd3dmx` can take the very calls `libxmx` takes. The
 * contract is the numerics, not the syntax:
 *
 *   - `half_round` is `f16tof32(f32tof16(x))`: two real conversion instructions, round to
 *     nearest even with half subnormals kept, as `packHalf2x16` / `unpackHalf2x16` are in
 *     the GLSL. Every narrow store goes through `f32tof16` as well, so what lands in a
 *     float16 buffer is the very value `half_round` would have given — no dependence on
 *     what a driver does for an `fptrunc`.
 *   - `precise` is spelled exactly where the GLSL spells it. D3D lets a driver contract a
 *     multiply and an add into one FMA unless the expression is precise, and the gate
 *     rounds to half between its multiply and its add: a fused pair skips that rounding
 *     and moves every E4M3 publish downstream (`notes/phase67` measured it through
 *     Metal's fast math). The rest of the arithmetic is left to the driver, as it is in
 *     the SPIR-V.
 *   - `round()` is round-to-nearest-even in HLSL (DXIL `Round_ne`), the `roundEven` the
 *     E4M3 quantiser needs.
 *
 * The operands. The SPIR-V takes four 64-bit device addresses in the push constants; HLSL
 * has no pointer to dereference, so the runtime binds each operand's *resource* as a root
 * UAV (u0..u3, a GPU virtual address and nothing else — no descriptor heap) and the push
 * block carries the byte offset of the operand inside it, element offsets folded in by the
 * runtime as libxmx folds them into the address. `pc.oa.x` is that offset for `a`; the high
 * word is zero (no buffer here is 4 GB). A kernel reads `a[i]` as `ld_f32(bufA, pc.oa.x, i)`.
 * The alignment tests the GLSL makes on the address it makes on the offset: a resource's
 * address is 64 KB aligned, so the offset's low bits are the address's.
 *
 * The fifth to eighth operands (u4..u7) are bound the same way, the fifth's and sixth's
 * offsets at push offsets 96 and 120 and the seventh's and eighth's in p0 and p2; see below.
 * The fifth and sixth operands (u4, u5) are bound the same way, their offsets at push
 * offsets 96 and 120; a pass that has no use for one still finds a dummy resource there.
 *
 * `spare`, unused by the SPIR-V, is the first group of this dispatch along the split axis:
 * D3D12 caps a dispatch at 65535 groups per axis and the graph's widest passes exceed it,
 * so `libd3dmx` issues them in pieces and each kernel adds `pc.spare` to its group id —
 * the row groups (`SV_GroupID.y`) in the GEMMs, the element groups (`.x`) everywhere else.
 */
#ifndef NR_D3D_HLSLI
#define NR_D3D_HLSLI

/* The resident push block: `struct push` in libxmx.c / libd3dmx.c, byte for byte — 128 bytes,
 * the six addresses read as (offset, 0) pairs: the four operands at 0..31, the fifth
 * (`residual_cos` in the SPIR-V: the residual's cosine, a pass's second output, the window
 * attention's bias, the QKV epilogue's V target) at 96, the sixth (`qkv_scale`: the QKV
 * epilogue's query scale, the fused feed-forward's projection weights) at 120. A
 * descriptor-path kernel defines NR_D3D_DESC_PUSH and declares its own, smaller block on the
 * same register. */
#ifndef NR_D3D_DESC_PUSH
struct Push {
    uint2 oa, ob, oc, od;
    uint m, n, k, batch;
    uint sa, sb, sc, flags;
    float p0, p1, p2, p3;
    uint lda, ldb, ldc, spare;
    uint2 oe;                                   /* offset 96: residual_cos */
    uint image_h, image_w, window_cols, window_pad;   /* the window residual's geometry */
    uint2 of;                                   /* offset 120: qkv_scale */
};
ConstantBuffer<Push> pc : register(b0);

/* Specialization constant 0 of the SPIR-V fixed the operation flags at pipeline creation;
 * HLSL has no such thing at run time, so the flags come from the push block. A build may
 * still fix them by hand for one kernel with -DSPECIALIZED_FLAGS=0x... */
#ifdef SPECIALIZED_FLAGS
uint operation_flags() { return SPECIALIZED_FLAGS; }
#else
uint operation_flags() { return pc.flags; }
#endif
#endif

RWByteAddressBuffer bufA : register(u0);
RWByteAddressBuffer bufB : register(u1);
RWByteAddressBuffer bufC : register(u2);
RWByteAddressBuffer bufD : register(u3);
RWByteAddressBuffer bufE : register(u4);        /* the operand at push offset 96 */
RWByteAddressBuffer bufF : register(u5);        /* the operand at push offset 120 */
/* The seventh and eighth (u6, u7), for the passes that take more than six (improve-b.md):
 * the SPIR-V reads 64-bit addresses at push offsets 64 and 72 (p0-p1, p2-p3) there, so here
 * those words carry their byte offsets — `asuint(pc.p0)` and `asuint(pc.p2)`. The merged
 * and stem feed-forward's sin-cos table or adapter in u6; the window block's output
 * projection in u6 and its pool or head weights in u7. */
RWByteAddressBuffer bufG : register(u6);
RWByteAddressBuffer bufH : register(u7);

/* -- publish.glsl -------------------------------------------------------- */

float half_round(float x) { return f16tof32(f32tof16(x)); }

float e4m3(float x) {
    float magnitude = min(abs(x), 448.0);
    int exponent = (asint(magnitude) >> 23) & 0xFF;
    exponent = max(exponent, 121) - 3;                 // 121-3 = the 2^-9 subnormal step
    float quantum = asfloat(exponent << 23);
    float reciprocal = asfloat((254 - exponent) << 23);
    float rounded = round(magnitude * reciprocal) * quantum;   // round: to nearest, ties to even
    return x < 0.0 ? -rounded : rounded;
}

float gate_activation(float x) {
    precise float wide = half_round(x);
    precise float clamped = clamp(wide, -4.0, 4.0);
    precise float affine = abs(clamped) * -0.055908203125;
    affine += 0.447265625;
    affine = half_round(affine);
    affine *= clamped;
    affine += 0.89453125;
    affine = half_round(affine);
    return half_round(wide * affine);
}

/* The publish an epilogue applies: bits 8-11 of a pass's `flags` pick the transform. */
float publish(uint epilogue, float value) {
    if (epilogue == 2u || epilogue == 3u) value = gate_activation(value);
    if (epilogue == 1u || epilogue == 3u) value = e4m3(value);
    if (epilogue == 4u) value = half_round(value);
    return value;
}

/* -- the operands behind the offsets ------------------------------------- */

float ld_f32(RWByteAddressBuffer buf, uint base, uint index) {
    return asfloat(buf.Load(base + index * 4u));
}
float ld_f16(RWByteAddressBuffer buf, uint base, uint index) {
    return f16tof32(buf.Load<uint16_t>(base + index * 2u));
}
void st_f32(RWByteAddressBuffer buf, uint base, uint index, float value) {
    buf.Store(base + index * 4u, asuint(value));
}
void st_f16(RWByteAddressBuffer buf, uint base, uint index, float value) {
    buf.Store<uint16_t>(base + index * 2u, (uint16_t)f32tof16(value));
}
/* Four consecutive halves from a byte address the caller has checked is 8-aligned: one
 * 64-bit load where four 16-bit ones went, the same four values. */
float4 ld_f16x4(RWByteAddressBuffer buf, uint byte_address) {
    uint2 w = buf.Load2(byte_address);
    return float4(f16tof32(w.x & 0xffffu), f16tof32(w.x >> 16), f16tof32(w.y & 0xffffu),
                  f16tof32(w.y >> 16));
}
/* Eight consecutive halves from a byte address the caller has checked is 16-aligned: one
 * 128-bit load, the same eight values as two `ld_f16x4` (libxmx's packed staged loader,
 * flag 0x8000000 in gemm_portable.hlsl). */
void ld_f16x8(RWByteAddressBuffer buf, uint byte_address, out float4 lo, out float4 hi) {
    uint4 w = buf.Load4(byte_address);
    lo = float4(f16tof32(w.x & 0xffffu), f16tof32(w.x >> 16), f16tof32(w.y & 0xffffu),
                f16tof32(w.y >> 16));
    hi = float4(f16tof32(w.z & 0xffffu), f16tof32(w.z >> 16), f16tof32(w.w & 0xffffu),
                f16tof32(w.w >> 16));
}
/* Four consecutive elements at a byte address the caller has checked the alignment of. */
void st_f32x4(RWByteAddressBuffer buf, uint byte_address, float4 value) {
    buf.Store4(byte_address, asuint(value));
}
void st_f16x4(RWByteAddressBuffer buf, uint byte_address, float4 value) {
    buf.Store2(byte_address, uint2(f32tof16(value.x) | (f32tof16(value.y) << 16),
                                   f32tof16(value.z) | (f32tof16(value.w) << 16)));
}

#endif
