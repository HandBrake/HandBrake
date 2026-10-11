/*
 * window_block — a 32-channel window block's attention half, one 8x8 window a 256-thread
 * threadgroup: `window_block.comp` (simdgroup, `window_block`) and
 * `window_block_portable.comp` (`window_block_portable`) in MSL.
 *
 * Each must equal, bit for bit, this runtime's own three passes on the same path: the QKV
 * projection with the window gather and the QKV epilogue, merged window attention, and the
 * output projection with the window residual (block 0: the pooled epilogue; block 70: the
 * head GEMM after it). On the simdgroup path every product is the simdgroup GEMMs' — each
 * 8x8 tile from zero, the K slices ascending eight at a time, as gemm_simd's staged, tiled
 * and base kernels all take them — and the softmax is window_attention.metal's. On the
 * portable path every product is the portable GEMM's, a float32 accumulator taking one K
 * term at a time. The normalisation is nr_epilogue.h's cosine tree, the residual its
 * `branch + skip * cosine` (nothing is contracted in this build), the publishes nr_metal.h's.
 *
 * Eight simdgroups (slices), a window row — eight tokens — each. Threadgroup memory: the
 * window's K and V as halves (8 KB; the portable kernel keeps Q there too, 12 KB), two
 * kilobytes a simdgroup for its row, projections, logits, weights, attention output and
 * block output in turn, and the rows' reciprocals: 24.25 KB (portable 28.25 KB).
 *
 * Push: a the image (float32, or half with flag 0x8000), also the residual's skip; b the
 * QKV weights (32 x 96); c the target; d the attention bias (64 x 64 float32); p0-p1 the
 * output projection (32 x 32) as a 64-bit address, p2-p3 block 0's pooled output
 * (0x800000) or block 70's head weights (0x1000000, 32 x 16); residual_cos the residual's
 * cosine; qkv_scale the query's scale; m the windows; image_h, image_w, window_cols and
 * window_pad the geometry; bits 8-12 the publish and a half target; 0x2000000 all sixteen
 * head columns. Grid (windows up to 65535, the rest on y).
 */
#include "nr_epilogue.h"

constant uint KT_STRIDE = 64u;                /* K transposed: (32 channels, 64 keys) */
constant uint WB_POOL = 0x800000u, WB_HEAD = 0x1000000u, WB_HEAD16 = 0x2000000u;

inline float wb_image(constant Push &pc, uint flags, uint at) {
    return (flags & 0x8000u) != 0u ? float(half_ptr(pc.a)[at]) : float_ptr(pc.a)[at];
}

/* window_attention.metal's transform, the bias already added to the logits */
inline float2 wb_weights(float2 logits) {
    float2 affine;
    for (uint j = 0u; j < 2u; j++) {
        float scaled = half_round(logits[j]) * 0.044921875f;
        scaled += 1.30078125f;
        affine[j] = clamp(scaled, 1.03125f, 1.5693359375f);
    }
    uint transformed = (as_type<uint>(half2(affine)) << 5) + 0x7FF88000u;
    return float2(as_type<half2>(transformed));
}

/* The pool of block 0: pooled pixel (py, px) takes window rows 2py and 2py + 1 — the float
 * stages of those two simdgroups (slices), `pitch` floats apart — at columns 2px and
 * 2px + 1, POOL2's taps in POOL2's order. */
inline void wb_pool(constant Push &pc, threadgroup const float *rows, uint pitch, uint window, uint lid) {
    uint top = pc.window_pad >> 16, left = pc.window_pad & 0xffffu;
    for (uint e = lid; e < 16u * 32u; e += 256u) {
        uint p = e / 32u, c = e % 32u, py = p / 4u, px = p % 4u;
        int yy = int((window / pc.window_cols) * 8u + 2u * py) - int(top);
        int xx = int((window % pc.window_cols) * 8u + 2u * px) - int(left);
        if (yy < 0 || xx < 0 || yy >= int(pc.image_h) || xx >= int(pc.image_w)) continue;
        uint upper = (2u * py) * pitch + (2u * px) * 32u + c, lower = upper + pitch;
        float total = rows[upper] + rows[lower];
        total += rows[upper + 32u];
        total += rows[lower + 32u];
        uint at = ((uint(yy) / 2u) * (pc.image_w / 2u) + uint(xx) / 2u) * 32u + c;
        half_out(push_address(pc.p2))[at] = half(e4m3(total * 0.25f));
    }
}

kernel void window_block(constant Push &pc [[buffer(0)]],
                         uint3 wg [[threadgroup_position_in_grid]],
                         uint lid [[thread_index_in_threadgroup]],
                         uint sg [[simdgroup_index_in_threadgroup]],
                         uint sl [[thread_index_in_simdgroup]]) {
    threadgroup half kv[KT_STRIDE * 32 + 64 * 32]; /* K transposed, then V (token, 32) */
    threadgroup float area[8 * 512];             /* two kilobytes a simdgroup */
    threadgroup float reciprocal[64];
    uint window = wg.x + wg.y * 65535u;
    if (window >= pc.m) return;                  /* uniform across the threadgroup */
    uint flags = operation_flags(pc);
    threadgroup half *K = kv, *V = kv + KT_STRIDE * 32u;
    threadgroup float *mine = area + sg * 512u;
    threadgroup half *mine_h = reinterpret_cast<threadgroup half *>(mine);
    threadgroup float *upper = mine + 256u;      /* the second kilobyte, as float */
    uint row = sg * 8u;

    uint top = pc.window_pad >> 16, left = pc.window_pad & 0xffffu;
    int y = int((window / pc.window_cols) * 8u + sg) - int(top);
    int x0 = int((window % pc.window_cols) * 8u) - int(left);
    bool row_in = y >= 0 && y < int(pc.image_h);

    /* the row, gathered as the staged kernel's loader gathers it: a lane eight channels of
     * one token, published to E4M3 (flag 0x4000 there) — the projection reads the
     * feed-forward output published and only the residual takes it raw — zero outside the
     * image */
    {
        uint t = sl / 4u, c0 = (sl % 4u) * 8u;
        int x = x0 + int(t);
        bool in = row_in && x >= 0 && x < int(pc.image_w);
        /* eight channels as two 4-wide loads and stores, the same conversions */
        half4 lo = half4(0.0h), hi = half4(0.0h);
        if (in) {
            uint at = (uint(y) * pc.image_w + uint(x)) * 32u + c0;
            float4 wlo, whi;
            if ((flags & 0x8000u) != 0u) {
                device const half4 *src = reinterpret_cast<device const half4 *>(half_ptr(pc.a) + at);
                wlo = float4(src[0]); whi = float4(src[1]);
            } else {
                device const float4 *src = reinterpret_cast<device const float4 *>(float_ptr(pc.a) + at);
                wlo = src[0]; whi = src[1];
            }
            for (uint e = 0u; e < 4u; e++) { wlo[e] = e4m3(wlo[e]); whi[e] = e4m3(whi[e]); }
            lo = half4(wlo); hi = half4(whi);
        }
        reinterpret_cast<threadgroup half4 *>(mine_h + t * 32u + c0)[0] = lo;
        reinterpret_cast<threadgroup half4 *>(mine_h + t * 32u + c0)[1] = hi;
    }
    simdgroup_barrier(mem_flags::mem_threadgroup);
    simdgroup_half8x8 a[4];
    for (uint s = 0u; s < 4u; s++) simdgroup_load(a[s], mine_h + s * 8u, 32);

    /* Q, K and V, 32 columns each, then the QKV epilogue on the part's stage: Q and K a lane
     * a row through the fragment tree, every value published */
    device const half *W = half_ptr(pc.b);
    float scale = half_round(float_ptr(pc.qkv_scale)[0]);
    for (uint part = 0u; part < 3u; part++) {
        simdgroup_float8x8 acc[4];
        for (uint j = 0u; j < 4u; j++) acc[j] = simdgroup_float8x8(0.0f);
        for (uint s = 0u; s < 4u; s++)
            for (uint j = 0u; j < 4u; j++) {
                simdgroup_half8x8 w;
                simdgroup_load(w, W + (s * 8u) * 96u + part * 32u + j * 8u, 96);
                simdgroup_multiply_accumulate(acc[j], a[s], w, acc[j]);
            }
        for (uint j = 0u; j < 4u; j++) simdgroup_store(acc[j], upper + j * 8u, 32);
        simdgroup_barrier(mem_flags::mem_threadgroup);
        if (part < 2u) {
            /* Four lanes a row, eight channels a lane — 2l and 2l + 1 of each group of
             * eight — which is how the fragment tree of cosine_reciprocal is laid out: each
             * lane's two partial sums, then the butterflies across lanes 2 and 1 apart as
             * shuffles. The same operations on the same operands as the one-lane-a-row
             * form (window_block.comp's layout), with all 32 lanes busy instead of 8. */
            uint r = sl / 4u, l = sl % 4u;
            float h[8];
            for (uint g = 0u; g < 4u; g++)
                for (uint p = 0u; p < 2u; p++) h[2u * g + p] = half_round(upper[r * 32u + g * 8u + 2u * l + p]);
            float partial[2];
            for (uint p = 0u; p < 2u; p++) {
                float first  = hfma(h[2u + p], h[2u + p], hmul(h[p], h[p]));
                float second = hfma(h[6u + p], h[6u + p], hmul(h[4u + p], h[4u + p]));
                partial[p] = hadd(first, second);
            }
            float pairs[2], whole[2];
            for (uint p = 0u; p < 2u; p++) pairs[p] = hadd(partial[p], simd_shuffle_xor(partial[p], 2u));
            for (uint p = 0u; p < 2u; p++) whole[p] = hadd(pairs[p], simd_shuffle_xor(pairs[p], 1u));
            float norm = max(hadd(whole[0], whole[1]), half_round(COSINE_NORM_FLOOR));
            norm = half_round(precise::rsqrt(norm));
            for (uint g = 0u; g < 4u; g++)
                for (uint p = 0u; p < 2u; p++) {
                    float value = hmul(h[2u * g + p], norm);
                    if (part == 0u) value = hmul(value, scale);
                    upper[r * 32u + g * 8u + 2u * l + p] = value;
                }
        }
        simdgroup_barrier(mem_flags::mem_threadgroup);
        /* Q into this simdgroup's first kilobyte past its row (in registers by now), K and V
         * into the window's */
        if (part == 1u) {
            /* K transposed — channel sl's eight keys of this row, one 16-byte store — so
             * QK^T's B tiles are plain loads, not transposed ones (1.8 ms of the block's
             * 8.6 at 1280x768: 8.57 -> 8.23 ms; a padded stride measured the same) */
            half4 lo, hi;
            for (uint r = 0u; r < 4u; r++) {
                lo[r] = half(e4m3(upper[r * 32u + sl]));
                hi[r] = half(e4m3(upper[(r + 4u) * 32u + sl]));
            }
            reinterpret_cast<threadgroup half4 *>(K + sl * KT_STRIDE + row)[0] = lo;
            reinterpret_cast<threadgroup half4 *>(K + sl * KT_STRIDE + row)[1] = hi;
        } else {
            threadgroup half *to = part == 0u ? mine_h + 256u : V + row * 32u;
            for (uint e = sl; e < 256u; e += 32u) to[e] = half(e4m3(upper[e]));
        }
        simdgroup_barrier(mem_flags::mem_threadgroup);
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);  /* every row's K and V in place */

    /* QK^T as window_attention.metal takes it: the key's transposed load, c ascending */
    simdgroup_float8x8 logits[8];
    for (uint j = 0u; j < 8u; j++) logits[j] = simdgroup_float8x8(0.0f);
    for (uint c = 0u; c < 32u; c += 8u) {
        simdgroup_half8x8 q;
        simdgroup_load(q, mine_h + 256u + c, 32);
        for (uint j = 0u; j < 8u; j++) {
            simdgroup_half8x8 k;
            simdgroup_load(k, K + c * KT_STRIDE + j * 8u, KT_STRIDE);
            simdgroup_multiply_accumulate(logits[j], q, k, logits[j]);
        }
    }
    /* the logits through the second kilobyte, half the keys at a time, each pair's weight
     * with its bias (one head: the bias row is the query's) into the first as halves */
    device const float *bias = float_ptr(pc.d);
    for (uint half_keys = 0u; half_keys < 2u; half_keys++) {
        for (uint j = 0u; j < 4u; j++) simdgroup_store(logits[half_keys * 4u + j], upper + j * 8u, 32);
        simdgroup_barrier(mem_flags::mem_threadgroup);
        for (uint t = 0u; t < 4u; t++) {
            uint at = sl * 2u + t * 64u, rr = at / 32u, key = half_keys * 32u + at % 32u;
            uint b = (row + rr) * 64u + key;
            float2 w = wb_weights(float2(upper[at] + bias[b], upper[at + 1u] + bias[b + 1u]));
            mine_h[rr * 64u + key] = half(w.x);
            mine_h[rr * 64u + key + 1u] = half(w.y);
        }
        simdgroup_barrier(mem_flags::mem_threadgroup);
    }
    if (sl < 8u) {
        /* in key order, eight weights to a load, so the chain waits on adds, not loads */
        threadgroup const half4 *weights4 = reinterpret_cast<threadgroup const half4 *>(mine_h + sl * 64u);
        float total = 0.0f;
        for (uint i = 0u; i < 16u; i += 2u) {
            half4 lo = weights4[i], hi = weights4[i + 1u];
            for (uint e = 0u; e < 4u; e++) total += float(lo[e]);
            for (uint e = 0u; e < 4u; e++) total += float(hi[e]);
        }
        reciprocal[row + sl] = half_round(1.0f / half_round(total));
    }
    simdgroup_barrier(mem_flags::mem_threadgroup);
    for (uint at = sl * 2u; at < 8u * 64u; at += 64u) {
        float scale_row = reciprocal[row + at / 64u];
        mine_h[at] = half(e4m3(hmul(float(mine_h[at]), scale_row)));
        mine_h[at + 1u] = half(e4m3(hmul(float(mine_h[at + 1u]), scale_row)));
    }
    simdgroup_barrier(mem_flags::mem_threadgroup);
    /* PV, the probability slices ascending */
    simdgroup_float8x8 context[4];
    for (uint j = 0u; j < 4u; j++) context[j] = simdgroup_float8x8(0.0f);
    for (uint c = 0u; c < 64u; c += 8u) {
        simdgroup_half8x8 p;
        simdgroup_load(p, mine_h + c, 64);
        for (uint j = 0u; j < 4u; j++) {
            simdgroup_half8x8 v;
            simdgroup_load(v, V + c * 32u + j * 8u, 32);
            simdgroup_multiply_accumulate(context[j], p, v, context[j]);
        }
    }
    for (uint j = 0u; j < 4u; j++) simdgroup_store(context[j], upper + j * 8u, 32);
    simdgroup_barrier(mem_flags::mem_threadgroup);
    /* the merged store's publish, into the first kilobyte as the projection's A */
    for (uint e = sl; e < 256u; e += 32u) mine_h[e] = half(e4m3(upper[e]));
    simdgroup_barrier(mem_flags::mem_threadgroup);

    /* the output projection as the tiled GEMM takes it */
    simdgroup_half8x8 attended[4];
    for (uint s = 0u; s < 4u; s++) simdgroup_load(attended[s], mine_h + s * 8u, 32);
    device const half *P = half_ptr(push_address(pc.p0));
    simdgroup_float8x8 projected[4];
    for (uint j = 0u; j < 4u; j++) projected[j] = simdgroup_float8x8(0.0f);
    for (uint s = 0u; s < 4u; s++)
        for (uint j = 0u; j < 4u; j++) {
            simdgroup_half8x8 w;
            simdgroup_load(w, P + (s * 8u) * 32u + j * 8u, 32);
            simdgroup_multiply_accumulate(projected[j], attended[s], w, projected[j]);
        }
    for (uint j = 0u; j < 4u; j++) simdgroup_store(projected[j], upper + j * 8u, 32);
    simdgroup_barrier(mem_flags::mem_threadgroup);

    /* the window residual's epilogue and the block's publish */
    uint epilogue = (flags >> 8) & 0xFu;
    bool narrow = (flags & 0x1000u) != 0u;
    bool pooling = (flags & WB_POOL) != 0u, head = (flags & WB_HEAD) != 0u;
    device const float *cosine = float_ptr(pc.residual_cos);
    for (uint e = sl * 4u; e < 256u; e += 128u) {
        uint t = e / 32u, c = e % 32u;
        int x = x0 + int(t);
        if (!row_in || x < 0 || x >= int(pc.image_w)) continue;
        uint at = (uint(y) * pc.image_w + uint(x)) * 32u + c;
        float4 out4;
        for (uint q = 0u; q < 4u; q++)
            out4[q] = publish(epilogue, upper[e + q] + wb_image(pc, flags, at + q) * cosine[c + q]);
        if (head) {
            for (uint q = 0u; q < 4u; q++) mine_h[e + q] = half(out4[q]);
        } else if (pooling) {
            half4 published;
            for (uint q = 0u; q < 4u; q++) { upper[e + q] = out4[q]; published[q] = half(e4m3(out4[q])); }
            reinterpret_cast<device half4 *>(half_out(pc.c))[at >> 2] = published;
        } else if (narrow) {
            reinterpret_cast<device half4 *>(half_out(pc.c))[at >> 2] = half4(out4);
        } else {
            reinterpret_cast<device float4 *>(float_out(pc.c))[at >> 2] = out4;
        }
    }
    if (head) {
        /* block 70's head as the base GEMM takes it: 32 -> 16 columns, K slices ascending */
        simdgroup_barrier(mem_flags::mem_threadgroup);
        simdgroup_half8x8 block_out[4];
        for (uint s = 0u; s < 4u; s++) simdgroup_load(block_out[s], mine_h + s * 8u, 32);
        device const half *H = half_ptr(push_address(pc.p2));
        simdgroup_float8x8 acc[2];
        for (uint j = 0u; j < 2u; j++) acc[j] = simdgroup_float8x8(0.0f);
        for (uint s = 0u; s < 4u; s++)
            for (uint j = 0u; j < 2u; j++) {
                simdgroup_half8x8 w;
                simdgroup_load(w, H + (s * 8u) * 16u + j * 8u, 16);
                simdgroup_multiply_accumulate(acc[j], block_out[s], w, acc[j]);
            }
        simdgroup_barrier(mem_flags::mem_threadgroup);
        for (uint j = 0u; j < 2u; j++) simdgroup_store(acc[j], upper + j * 8u, 16);
        simdgroup_barrier(mem_flags::mem_threadgroup);
        if ((flags & WB_HEAD16) == 0u) {
            uint t = sl / 4u, c = sl % 4u;
            int x = x0 + int(t);
            if (row_in && x >= 0 && x < int(pc.image_w))
                float_out(pc.c)[(uint(y) * pc.image_w + uint(x)) * 4u + c] = upper[t * 16u + c];
        } else {
            uint t = sl / 4u, c = (sl % 4u) * 4u;
            int x = x0 + int(t);
            if (row_in && x >= 0 && x < int(pc.image_w))
                reinterpret_cast<device float4 *>(float_out(pc.c))[((uint(y) * pc.image_w + uint(x)) * 16u + c) >> 2] =
                    float4(upper[t * 16u + c], upper[t * 16u + c + 1u], upper[t * 16u + c + 2u], upper[t * 16u + c + 3u]);
        }
        return;
    }
    if (!pooling) return;
    threadgroup_barrier(mem_flags::mem_threadgroup);
    wb_pool(pc, area + 256u, 512u, window, lid);
}

kernel void window_block_portable(constant Push &pc [[buffer(0)]],
                                  uint3 wg [[threadgroup_position_in_grid]],
                                  uint lid [[thread_index_in_threadgroup]]) {
    threadgroup half qkv[3 * 64 * 32];           /* Q, K, V as (token, 32) */
    threadgroup float region[8 * 512];           /* two kilobytes a slice */
    threadgroup float reciprocal[64];
    uint window = wg.x + wg.y * 65535u;
    if (window >= pc.m) return;
    uint flags = operation_flags(pc);
    uint slice = lid >> 5u, lane = lid & 31u;
    /* the portable GEMM's lane: row r of the slice's eight, four columns cq.. a tile */
    uint r = lane >> 2u, cq = (lane & 3u) * 4u;
    threadgroup float *mine = region + slice * 512u;
    uint token = slice * 8u + r;
    threadgroup const half *K = qkv + 2048u, *V = qkv + 4096u;

    uint top = pc.window_pad >> 16, left = pc.window_pad & 0xffffu;
    int y = int((window / pc.window_cols) * 8u + slice) - int(top);
    int x = int((window % pc.window_cols) * 8u + r) - int(left);
    bool inside = y >= 0 && y < int(pc.image_h) && x >= 0 && x < int(pc.image_w);
    uint pixel = inside ? (uint(y) * pc.image_w + uint(x)) * 32u : 0u;

    /* Q, K and V as the tiled portable GEMM computes the gathered projection, the image
     * published on the way in */
    {
        device const half *W = half_ptr(pc.b);
        float4 acc[6];
        for (uint j = 0u; j < 6u; j++) acc[j] = float4(0.0f);
        for (uint k = 0u; k < 32u; k++) {
            float4 bv[6];
            for (uint j = 0u; j < 6u; j++) {
                uint at = k * 96u + cq + j * 16u;
                bv[j] = float4(float(W[at]), float(W[at + 1u]), float(W[at + 2u]), float(W[at + 3u]));
            }
            float av = 0.0f;
            if (inside)
                av = e4m3((flags & 0x8000u) != 0u ? float(half_ptr(pc.a)[pixel + k])
                                                  : float_ptr(pc.a)[pixel + k]);
            for (uint j = 0u; j < 6u; j++) acc[j] += av * bv[j];
        }
        float scale = half_round(float_ptr(pc.qkv_scale)[0]);
        for (uint part = 0u; part < 3u; part++) {
            for (uint j = 0u; j < 2u; j++)
                for (uint e = 0u; e < 4u; e++) mine[r * 32u + cq + j * 16u + e] = acc[part * 2u + j][e];
            threadgroup_barrier(mem_flags::mem_threadgroup);
            if (part < 2u && lane < 8u) {
                float h[32];
                for (uint i = 0u; i < 32u; i++) h[i] = half_round(mine[lane * 32u + i]);
                float norm = cosine_reciprocal(h);
                for (uint i = 0u; i < 32u; i++) {
                    float value = hmul(h[i], norm);
                    if (part == 0u) value = hmul(value, scale);
                    mine[lane * 32u + i] = value;
                }
            }
            threadgroup_barrier(mem_flags::mem_threadgroup);
            for (uint e = lane; e < 256u; e += 32u) qkv[part * 2048u + slice * 256u + e] = half(e4m3(mine[e]));
            threadgroup_barrier(mem_flags::mem_threadgroup);
        }
    }

    /* QK^T as the transposed-B portable GEMM computes it */
    {
        float4 acc[4];
        for (uint j = 0u; j < 4u; j++) acc[j] = float4(0.0f);
        for (uint c = 0u; c < 32u; c++) {
            float4 kvv[4];
            for (uint j = 0u; j < 4u; j++) {
                uint at = (cq + j * 16u) * 32u + c;
                kvv[j] = float4(float(K[at]), float(K[at + 32u]), float(K[at + 64u]), float(K[at + 96u]));
            }
            float qv = float(qkv[token * 32u + c]);
            for (uint j = 0u; j < 4u; j++) acc[j] += qv * kvv[j];
        }
        for (uint j = 0u; j < 4u; j++)
            for (uint e = 0u; e < 4u; e++) mine[r * 64u + cq + j * 16u + e] = acc[j][e];
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);
    device const float *bias = float_ptr(pc.d);
    for (uint at = lane * 2u; at < 512u; at += 64u) {
        uint rr = at / 64u, key = at % 64u, b = (slice * 8u + rr) * 64u + key;
        float2 w = wb_weights(float2(mine[at] + bias[b], mine[at + 1u] + bias[b + 1u]));
        mine[at] = w.x;
        mine[at + 1u] = w.y;
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);
    if (lane < 8u) {
        float total = 0.0f;
        for (uint i = 0u; i < 64u; i += 2u) {
            total += mine[lane * 64u + i];
            total += mine[lane * 64u + i + 1u];
        }
        reciprocal[slice * 8u + lane] = half_round(1.0f / half_round(total));
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);
    for (uint at = lane; at < 512u; at += 32u)
        mine[at] = e4m3(hmul(mine[at], reciprocal[slice * 8u + at / 64u]));
    threadgroup_barrier(mem_flags::mem_threadgroup);
    /* PV as the plain portable GEMM computes it, then the merged store's publish */
    float4 context[2];
    for (uint j = 0u; j < 2u; j++) context[j] = float4(0.0f);
    for (uint key = 0u; key < 64u; key++) {
        float4 vv[2];
        for (uint j = 0u; j < 2u; j++) {
            uint at = key * 32u + cq + j * 16u;
            vv[j] = float4(float(V[at]), float(V[at + 1u]), float(V[at + 2u]), float(V[at + 3u]));
        }
        float p = mine[r * 64u + key];
        for (uint j = 0u; j < 2u; j++) context[j] += p * vv[j];
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);
    for (uint j = 0u; j < 2u; j++)
        for (uint e = 0u; e < 4u; e++) mine[r * 32u + cq + j * 16u + e] = e4m3(context[j][e]);
    threadgroup_barrier(mem_flags::mem_threadgroup);

    /* the output projection as the tiled portable GEMM computes it */
    float4 projected[2];
    for (uint j = 0u; j < 2u; j++) projected[j] = float4(0.0f);
    device const half *P = half_ptr(push_address(pc.p0));
    for (uint k = 0u; k < 32u; k++) {
        float4 bv[2];
        for (uint j = 0u; j < 2u; j++) {
            uint at = k * 32u + cq + j * 16u;
            bv[j] = float4(float(P[at]), float(P[at + 1u]), float(P[at + 2u]), float(P[at + 3u]));
        }
        float av = mine[r * 32u + k];
        for (uint j = 0u; j < 2u; j++) projected[j] += av * bv[j];
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);

    uint epilogue = (flags >> 8) & 0xFu;
    bool narrow = (flags & 0x1000u) != 0u;
    bool pooling = (flags & WB_POOL) != 0u, head = (flags & WB_HEAD) != 0u;
    device const float *cosine = float_ptr(pc.residual_cos);
    for (uint j = 0u; j < 2u; j++) {
        if (!inside) continue;
        uint c0 = cq + j * 16u, at = pixel + c0;
        float4 out4;
        for (uint e = 0u; e < 4u; e++)
            out4[e] = publish(epilogue, projected[j][e] + wb_image(pc, flags, at + e) * cosine[c0 + e]);
        if (head) {
            for (uint e = 0u; e < 4u; e++) mine[r * 32u + c0 + e] = half_round(out4[e]);
        } else if (pooling) {
            half4 published;
            for (uint e = 0u; e < 4u; e++) { mine[r * 32u + c0 + e] = out4[e]; published[e] = half(e4m3(out4[e])); }
            reinterpret_cast<device half4 *>(half_out(pc.c))[at >> 2] = published;
        } else if (narrow) {
            reinterpret_cast<device half4 *>(half_out(pc.c))[at >> 2] = half4(out4);
        } else {
            reinterpret_cast<device float4 *>(float_out(pc.c))[at >> 2] = out4;
        }
    }
    if (head) {
        threadgroup_barrier(mem_flags::mem_threadgroup);
        /* the head as the base portable GEMM computes it: 32 -> 16 columns */
        device const half *H = half_ptr(push_address(pc.p2));
        float4 acc = float4(0.0f);
        for (uint k = 0u; k < 32u; k++) {
            uint at = k * 16u + cq;
            float4 bv = float4(float(H[at]), float(H[at + 1u]), float(H[at + 2u]), float(H[at + 3u]));
            float av = mine[r * 32u + k];
            acc += av * bv;
        }
        if (!inside) return;
        uint at = uint(y) * pc.image_w + uint(x);
        if ((flags & WB_HEAD16) == 0u) {
            if (cq == 0u)
                for (uint e = 0u; e < 4u; e++) float_out(pc.c)[at * 4u + e] = acc[e];
        } else {
            reinterpret_cast<device float4 *>(float_out(pc.c))[(at * 16u + cq) >> 2] = acc;
        }
        return;
    }
    if (!pooling) return;
    threadgroup_barrier(mem_flags::mem_threadgroup);
    wb_pool(pc, region, 512u, window, lid);
}
