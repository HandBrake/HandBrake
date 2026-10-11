/*
 * nr_epilogue.h — what the fused passes share, in MSL: `cosine_tree.glsl` (the cosine
 * normalise's reciprocal norm, one definition for the row pass and the QKV epilogue),
 * `residual_epilogue.glsl` (the residual folded into a GEMM's store, plain or through a
 * window layout) and `qkv_epilogue.glsl` (the QKV projection finished on its own stage).
 * Every GEMM kernel here — simdgroup and portable, tiled and staged — and the fused
 * feed-forward finish through these, so the arithmetic exists once.
 *
 * The expression shapes are the GLSL's; the build contracts nothing, so `l * r` then
 * `+ acc` stay two operations and every `half_round` lands where the reference rounds.
 */
#ifndef NR_EPILOGUE_H
#define NR_EPILOGUE_H
#include "nr_metal.h"

/* -- cosine_tree.glsl ------------------------------------------------------ */

constant float COSINE_NORM_FLOOR = 0.00006198883056640625f;

/* half(l*r), half(l+r), half(l*r + acc): one float32 expression, one rounding. */
inline float hmul(float l, float r) { float t = l * r; return half_round(t); }
inline float hadd(float l, float r) { float t = l + r; return half_round(t); }
inline float hfma(float l, float r, float acc) { float t = l * r; t = t + acc; return half_round(t); }

/* `h` is one head's 32 channels, already rounded to half: the vendor's fragment tree,
 * eight partial sums of four squares, two butterfly levels, the last add. */
inline float cosine_reciprocal(thread const float *h) {
    float partial[4][2];
    for (uint lane = 0u; lane < 4u; lane++)
        for (uint parity = 0u; parity < 2u; parity++) {
            uint ch = lane * 2u + parity;
            float first  = hfma(h[ch + 8u],  h[ch + 8u],  hmul(h[ch],       h[ch]));
            float second = hfma(h[ch + 24u], h[ch + 24u], hmul(h[ch + 16u], h[ch + 16u]));
            partial[lane][parity] = hadd(first, second);
        }
    float two[4][2], one[4][2];
    for (uint lane = 0u; lane < 4u; lane++)
        for (uint p = 0u; p < 2u; p++) two[lane][p] = hadd(partial[lane][p], partial[lane ^ 2u][p]);
    for (uint lane = 0u; lane < 4u; lane++)
        for (uint p = 0u; p < 2u; p++) one[lane][p] = hadd(two[lane][p], two[lane ^ 1u][p]);
    float norm = max(hadd(one[0][0], one[0][1]), half_round(COSINE_NORM_FLOOR));
    return half_round(precise::rsqrt(norm));
}

/* The ViT's own (OpenDLSS-NR's `vit.wgsl`, notes/opendlss-reference.md): each pair square
 * summed in float32 — the low one exact, the high one rounded to half first — and rounded
 * once, then a stride 8, 4 and 2 tree of half adds, the last two pairs apart, and the
 * reciprocal square root as a divide. The floor keeps a zero head zero. */
inline float vit_reciprocal(thread const float *h) {
    float r[16];
    for (uint c = 0u; c < 16u; c++) r[c] = hfma(h[c], h[c], hmul(h[c + 16u], h[c + 16u]));
    for (uint c = 0u; c < 8u; c++) r[c] = hadd(r[c], r[c + 8u]);
    for (uint c = 0u; c < 4u; c++) r[c] = hadd(r[c], r[c + 4u]);
    float norm = max(hadd(hadd(r[0], r[2]), hadd(r[1], r[3])), half_round(COSINE_NORM_FLOOR));
    return half_round(1.0f / precise::sqrt(norm));
}

/* half(sqrt(32)): the ViT's query takes it as a half multiply of its own, between the norm
 * and the learned scale. */
constant float VIT_ROOT = 5.65625f;

/* -- residual_epilogue.glsl ------------------------------------------------ */

/* Bit 0x80000: the rows are a window block's, padded and in window order, and the first
 * of four consecutive channels is mapped into the unpadded image. A padding token has
 * no destination and must not read the skip. */
inline bool residual_output_index(constant Push &pc, uint flags, thread uint &index) {
    if ((flags & 0x80000u) == 0u) return true;
    uint pixel = index / pc.n, window = pixel / 64u, token = pixel % 64u;
    uint y = (window / pc.window_cols) * 8u + token / 8u;
    uint x = (window % pc.window_cols) * 8u + token % 8u;
    uint top = pc.window_pad >> 16, left = pc.window_pad & 0xffffu;
    if (y < top || x < left || y - top >= pc.image_h || x - left >= pc.image_w) return false;
    index = ((y - top) * pc.image_w + x - left) * pc.n + index % pc.n;
    return true;
}

/* Bit 0x20000: `branch + skip * cosine` before the publish, the skip in `d` (half with
 * 0x40000), the cosine per channel at `residual_cos` — the RESIDUAL pass's expression. */
inline float add_gemm_residual(constant Push &pc, uint flags, float branch, uint index) {
    if ((flags & 0x20000u) == 0u) return branch;
    float skip = (flags & 0x40000u) != 0u ? float(half_ptr(pc.d)[index]) : float_ptr(pc.d)[index];
    return branch + skip * float_ptr(pc.residual_cos)[index % pc.n];
}

/* The generic store of a BM x BN block staged raw in threadgroup memory: every element
 * finished on its own — the window mapping, the residual, the publish — then four
 * consecutive elements a lane, narrow or wide. `lanes` cover the whole block. */
inline void store_staged(constant Push &pc, uint flags, threadgroup const float *stage, uint BM, uint BN,
                         uint row, uint col, uint co, uint ldc, uint lane, uint lanes,
                         uint last = 0xFFFFFFFFu) {
    uint epilogue = (flags >> 8) & 0xFu;
    bool narrow = (flags & 0x1000u) != 0u;
    device float *C = float_out(pc.c);
    device half *Ch = half_out(pc.c);
    for (uint e = lane * 4u; e < BM * BN; e += lanes * 4u) {
        if (row + e / BN > last) continue;          // a staged block's rows past M
        uint at = co + (row + e / BN) * ldc + col + e % BN;
        if (!residual_output_index(pc, flags, at)) continue;
        float4 out4;
        for (uint q = 0; q < 4u; ++q) out4[q] = publish(epilogue, add_gemm_residual(pc, flags, stage[e + q], at + q));
        if (!narrow) {
            if ((at & 3u) == 0u && (pc.c & 15u) == 0u)
                reinterpret_cast<device float4 *>(C)[at >> 2] = out4;
            else
                for (uint q = 0; q < 4u; ++q) C[at + q] = out4[q];
        } else if ((at & 3u) == 0u && (pc.c & 7u) == 0u) {
            reinterpret_cast<device half4 *>(Ch)[at >> 2] = half4(out4);
        } else {
            for (uint q = 0; q < 4u; ++q) Ch[at + q] = half(out4[q]);
        }
    }
}

/* Bit 0x200000: the same block also published into `d`, as E4M3 halves (what an e4m3_half
 * of the float32 output would write): block 0's stem, which its feed-forward's residual
 * takes raw and its GEMM published. */
inline void store_half_copy(constant Push &pc, threadgroup const float *stage, uint BM, uint BN,
                            uint row, uint col, uint co, uint ldc, uint lane, uint lanes) {
    device half *Dh = half_out(pc.d);
    for (uint e = lane * 4u; e < BM * BN; e += lanes * 4u) {
        uint at = co + (row + e / BN) * ldc + col + e % BN;
        float4 v = float4(e4m3(stage[e]), e4m3(stage[e + 1]), e4m3(stage[e + 2]), e4m3(stage[e + 3]));
        if ((at & 3u) == 0u && (pc.d & 7u) == 0u)
            reinterpret_cast<device half4 *>(Dh)[at >> 2] = half4(v);
        else
            for (uint q = 0; q < 4u; ++q) Dh[at + q] = half(v[q]);
    }
}

/* -- qkv_epilogue.glsl ------------------------------------------------------ */

template <bool GROUP>
inline void qkv_barrier() {
    if (GROUP) threadgroup_barrier(mem_flags::mem_threadgroup);
    else       simdgroup_barrier(mem_flags::mem_threadgroup);
}

/* Bit 0x100000: the workgroup's 32-column block is one head of one of Q, K or V. Q and K
 * rows are normalised in place in `stage` (Q also by its head's scale), then every part
 * is published as E4M3 halves in (window, head, token, 32) order — c is Q, d is K,
 * residual_cos is V, qkv_scale the query scale, image_h tokens, image_w heads. GROUP
 * says whether the block belongs to several simdgroups (the staged kernel). */
template <bool GROUP>
inline void qkv_epilogue(constant Push &pc, threadgroup float *stage, uint BM, uint BN,
                         uint row, uint col, uint lane, uint lanes) {
    uint channels = pc.n / 3u, tokens = pc.image_h, heads = pc.image_w;
    uint part = col / channels, head = (col % channels) / 32u;
    if (part < 2u) {
        /* flag 0x2000: the ViT's normalisation, its query taking half(sqrt(32)) and then
         * its learned scale, two half multiplies */
        bool vit = (operation_flags(pc) & 0x2000u) != 0u;
        float scale = part == 0u ? half_round(float_ptr(pc.qkv_scale)[head]) : 1.0f;
        for (uint r = lane; r < BM; r += lanes) {
            float h[32];
            for (uint i = 0u; i < 32u; i++) h[i] = half_round(stage[r * BN + i]);
            float reciprocal = vit ? vit_reciprocal(h) : cosine_reciprocal(h);
            for (uint i = 0u; i < 32u; i++) {
                float value = hmul(h[i], reciprocal);
                if (part == 0u) value = hmul(vit ? hmul(value, VIT_ROOT) : value, scale);
                stage[r * BN + i] = value;                 // a half value: exact in float
            }
        }
        qkv_barrier<GROUP>();
    }
    ulong target = part == 0u ? pc.c : (part == 1u ? pc.d : pc.residual_cos);
    bool wide = (target & 7u) == 0u;
    for (uint e = lane * 4u; e < BM * BN; e += lanes * 4u) {
        uint r = row + e / BN, window = r / tokens, token = r % tokens;
        if (r >= pc.m) continue;                 // a partial last block's rows past M
        uint at = ((window * heads + head) * tokens + token) * 32u + e % BN;
        float4 out4;
        for (uint i = 0u; i < 4u; i++) out4[i] = e4m3(stage[e + i]);
        if (wide) reinterpret_cast<device half4 *>(half_out(target))[at >> 2] = half4(out4);
        else for (uint i = 0u; i < 4u; i++) half_out(target)[at + i] = half(out4[i]);
    }
}

/* -- the window-gathered A (bit 0x400000) ----------------------------------- */

/* Token row `r` of a window block: its image element base for channel 0, or -1 outside
 * the image. `sa` is the image width, `sb` its height, `sc` the windows per row,
 * `window_pad` (top << 16) | left, `k` the channel count. */
inline int window_row_base(constant Push &pc, uint r) {
    uint window = r / 64u, token = r % 64u;
    int y = int((window / pc.sc) * 8u + token / 8u) - int(pc.window_pad >> 16);
    int x = int((window % pc.sc) * 8u + token % 8u) - int(pc.window_pad & 0xffffu);
    if (y < 0 || x < 0 || y >= int(pc.sb) || x >= int(pc.sa)) return -1;
    return int((uint(y) * pc.sa + uint(x)) * pc.k);
}

/* Four consecutive channels of a gathered row: zero outside the image, the half values
 * inside, a float32 image rounded to half on the way in as the partition's store did —
 * or, with bit 0x4000, published to E4M3 as it is loaded: a 32-channel block's
 * feed-forward output, which the projection reads published and the residual raw
 * (notes/opendlss-reference.md). */
inline half4 window_load4(constant Push &pc, uint flags, int base, uint k) {
    if (base < 0) return half4(0.0h);
    float4 v = (flags & 0x8000u) != 0u
        ? float4(reinterpret_cast<device const half4 *>(half_ptr(pc.a) + uint(base) + k)[0])
        : reinterpret_cast<device const float4 *>(float_ptr(pc.a) + uint(base) + k)[0];
    if ((flags & 0x4000u) != 0u)
        for (uint e = 0u; e < 4u; e++) v[e] = e4m3(v[e]);
    return half4(v);
}

inline float window_load(constant Push &pc, uint flags, int base, uint k) {
    if (base < 0) return 0.0f;
    float v = (flags & 0x8000u) != 0u ? float(half_ptr(pc.a)[uint(base) + k])
                                      : float_ptr(pc.a)[uint(base) + k];
    return (flags & 0x4000u) != 0u ? e4m3(v) : half_round(v);
}

#endif
