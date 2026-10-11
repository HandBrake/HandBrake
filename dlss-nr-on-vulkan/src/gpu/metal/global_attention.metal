/*
 * global_attention — a bottleneck block's attention, every token over every token, in one
 * pass: `global_attention.comp` (simdgroup, `global_attention`) and
 * `global_attention_portable.comp` (`global_attention_portable`) in MSL. QK^T, the ViT's
 * softmax, PV and the head merge, no score stored.
 *
 * The ViT's softmax is the vendor's (OpenDLSS-NR's `vit.wgsl`, notes/opendlss-reference.md):
 * the weights are published unnormalised, the value sum is multiplied by the reciprocal
 * afterwards, and the denominator is a fixed tree of half adds over 64-key blocks, padded
 * with zero keys and their weight taken off at the end. With the normalisation after the
 * PV, one trip through the keys does: a 256-thread threadgroup takes one head and 64 query
 * rows, a simdgroup (slice) eight of them, and the keys and values come through
 * threadgroup memory 64 at a time. For each block the logits are made, the weights
 * `vit_weight`, each row's lane folds them into its running denominator in the tree's
 * order, and the published weights go into the PV.
 *
 * Each must equal this runtime's own four passes on its path, bit for bit: the batched
 * transposed-B GEMM for QK^T, attention.metal's `vit_softmax`, the batched GEMM for PV over
 * every padded key, and merge_heads scaled by the row's reciprocal with the E4M3 publish.
 * On the simdgroup path each 8x8 tile is summed from zero over 8-wide K slices ascending,
 * as every gemm_simd kernel sums; on the portable path a float32 accumulator takes one K
 * term at a time.
 *
 * Threadgroup memory: a block of keys and its values as halves (8 KB) and two kilobytes a
 * simdgroup (slice) — the simdgroup kernel stages its logits there 32 keys at a time and
 * keeps the block's 64 weights as halves beside them; the portable kernel keeps all 64
 * logits as floats — and the rows' reciprocals: 24.25 KB.
 *
 * Push: a Q, b K, d V, each (heads, rows, 32) half; c the merged output (rows, 32 * heads)
 * half; m the rows (a multiple of 16, no more than the tokens' 64-block), n the tokens,
 * batch the heads. Grid (ceil(rows / 64), heads).
 */
#include "nr_epilogue.h"

/* attention.metal's `vit_weight`: the ViT's exponential. */
inline float ga_vit_weight(float logit) {
    float affine = half_round(logit) * 0.08953857421875f;
    affine += 1.708984375f;
    affine = clamp(half_round(affine), 1.439453125f, 1.9775390625f);
    uint bits = uint(as_type<ushort>(half(affine)));
    return float(as_type<half>(ushort(((bits << 4) + 0x4000u) & 0xFFFFu)));
}

/* One row's 64 weights of a block into its running denominator, in vit_softmax's order:
 * per column c of eight the keys c, c+8, ..., c+56 a pair at a time and the pairs in turn,
 * the even columns, the odd ones, the two. */
template <typename T>
inline float ga_block_total(float total, T w) {
    float column[8];
    for (uint c = 0u; c < 8u; c++) {
        float pair[4];
        for (uint j = 0u; j < 4u; j++)
            pair[j] = hadd(float(w[c + 16u * j]), float(w[c + 16u * j + 8u]));
        column[c] = hadd(hadd(hadd(pair[0], pair[1]), pair[2]), pair[3]);
    }
    float even = hadd(hadd(hadd(column[0], column[2]), column[4]), column[6]);
    float odd = hadd(hadd(hadd(column[1], column[3]), column[5]), column[7]);
    return hadd(total, hadd(even, odd));
}

/* The row's reciprocal once every block is in: the padding's weight taken off. */
inline float ga_reciprocal(float total, uint blocks, uint tokens) {
    uint padding = blocks * 64u - tokens;
    if (padding > 0u) total = hadd(total, -hmul(ga_vit_weight(0.0f), float(padding)));
    return half_round(1.0f / total);
}

/* A block of keys and values into threadgroup memory, zero past `count`. */
inline void ga_load(constant Push &pc, uint plane, uint first, uint count,
                    threadgroup half *keys, threadgroup half *vals, uint lid) {
    for (uint e = lid; e < 64u * 32u; e += 256u) {
        bool in = e / 32u < count;
        keys[e] = in ? half_ptr(pc.b)[plane + first * 32u + e] : 0.0h;
        vals[e] = in ? half_ptr(pc.d)[plane + first * 32u + e] : 0.0h;
    }
}

kernel void global_attention(constant Push &pc [[buffer(0)]],
                             uint3 wg [[threadgroup_position_in_grid]],
                             uint lid [[thread_index_in_threadgroup]],
                             uint sg [[simdgroup_index_in_threadgroup]],
                             uint sl [[thread_index_in_simdgroup]]) {
    threadgroup half keys[64 * 32], vals[64 * 32];
    threadgroup float area[8 * 512];
    threadgroup float reciprocal[64];
    uint rows = pc.m, tokens = pc.n, heads = pc.batch, head = wg.y;
    uint row = wg.x * 64u + sg * 8u;
    bool busy = row < rows;                     /* uniform across the simdgroup */
    threadgroup float *stage = area + sg * 512u;                     /* 8 x 32 float */
    threadgroup half *weights = reinterpret_cast<threadgroup half *>(stage + 256u);   /* 8 x 64 half */
    uint plane = head * rows * 32u;
    uint blocks = (rows + 63u) / 64u;            /* the tokens' 64-blocks too */

    simdgroup_half8x8 query[4];
    if (busy)
        for (uint c = 0u; c < 4u; c++) simdgroup_load(query[c], half_ptr(pc.a) + plane + row * 32u + c * 8u, 32);
    float total = 0.0f;                          /* lane r < 8: row r's sum */
    simdgroup_float8x8 context[4];
    for (uint j = 0u; j < 4u; j++) context[j] = simdgroup_float8x8(0.0f);

    for (uint block = 0u; block < blocks; block++) {
        uint first = block * 64u, count = min(64u, rows - first);
        threadgroup_barrier(mem_flags::mem_threadgroup);    /* the last block is done with */
        ga_load(pc, plane, first, count, keys, vals, lid);
        threadgroup_barrier(mem_flags::mem_threadgroup);
        if (!busy) continue;
        for (uint half_keys = 0u; half_keys < 2u; half_keys++) {
            /* keys past the rows are zero, and past the tokens their weight is the
             * padding's whatever the logit, so both halves are always made */
            simdgroup_float8x8 logits[4];
            for (uint j = 0u; j < 4u; j++) logits[j] = simdgroup_float8x8(0.0f);
            if (half_keys * 32u < count)
                for (uint c = 0u; c < 4u; c++)
                    for (uint j = 0u; j < 4u; j++) {
                        simdgroup_half8x8 k;
                        simdgroup_load(k, keys + (half_keys * 32u + j * 8u) * 32u + c * 8u, 32, ulong2(0, 0), true);
                        simdgroup_multiply_accumulate(logits[j], query[c], k, logits[j]);
                    }
            for (uint j = 0u; j < 4u; j++) simdgroup_store(logits[j], stage + j * 8u, 32);
            simdgroup_barrier(mem_flags::mem_threadgroup);
            for (uint t = 0u; t < 8u; t++) {
                uint at = sl + t * 32u, rr = at / 32u, key = half_keys * 32u + at % 32u;
                weights[rr * 64u + key] = half(ga_vit_weight(first + key < tokens ? stage[at] : 0.0f));
            }
            simdgroup_barrier(mem_flags::mem_threadgroup);
        }
        if (sl < 8u) total = ga_block_total(total, weights + sl * 64u);
        simdgroup_barrier(mem_flags::mem_threadgroup);
        /* published, unnormalised, as the PV's A; zero past the tokens */
        for (uint t = 0u; t < 16u; t++) {
            uint at = sl + t * 32u;
            weights[at] = first + at % 64u < tokens ? half(e4m3(float(weights[at]))) : 0.0h;
        }
        simdgroup_barrier(mem_flags::mem_threadgroup);
        for (uint c = 0u; c < count; c += 8u) {
            simdgroup_half8x8 p;
            simdgroup_load(p, weights + c, 64);
            for (uint j = 0u; j < 4u; j++) {
                simdgroup_half8x8 v;
                simdgroup_load(v, vals + c * 32u + j * 8u, 32);
                simdgroup_multiply_accumulate(context[j], p, v, context[j]);
            }
        }
        simdgroup_barrier(mem_flags::mem_threadgroup);
    }
    if (!busy) return;
    if (sl < 8u) reciprocal[sg * 8u + sl] = ga_reciprocal(total, blocks, tokens);
    for (uint j = 0u; j < 4u; j++) simdgroup_store(context[j], stage + j * 8u, 32);
    simdgroup_barrier(mem_flags::mem_threadgroup);
    /* the head merge's store: the value sum rounded to half times its row's reciprocal,
     * published */
    uint channels = heads * 32u;
    for (uint e = sl * 4u; e < 256u; e += 128u) {
        uint r = row + e / 32u, d = e % 32u;
        float scale = reciprocal[sg * 8u + e / 32u];
        half4 values;
        for (uint i = 0u; i < 4u; i++) values[i] = half(e4m3(hmul(half_round(stage[e + i]), scale)));
        reinterpret_cast<device half4 *>(half_out(pc.c))[(r * channels + head * 32u + d) >> 2] = values;
    }
}

kernel void global_attention_portable(constant Push &pc [[buffer(0)]],
                                      uint3 wg [[threadgroup_position_in_grid]],
                                      uint lid [[thread_index_in_threadgroup]]) {
    threadgroup half keys[64 * 32], vals[64 * 32];
    threadgroup float region[8 * 512];          /* 8 rows x 64 keys a slice */
    threadgroup float reciprocal[64];
    uint rows = pc.m, tokens = pc.n, heads = pc.batch, head = wg.y;
    uint slice = lid >> 5u, lane = lid & 31u;
    uint r = lane >> 2u, cq = (lane & 3u) * 4u;
    uint row = wg.x * 64u + slice * 8u;
    bool busy = row < rows;
    threadgroup float *mine = region + slice * 512u;
    uint plane = head * rows * 32u;
    uint blocks = (rows + 63u) / 64u;

    float total = 0.0f;
    float4 context[2];
    for (uint j = 0u; j < 2u; j++) context[j] = float4(0.0f);

    for (uint block = 0u; block < blocks; block++) {
        uint first = block * 64u, count = min(64u, rows - first);
        threadgroup_barrier(mem_flags::mem_threadgroup);
        ga_load(pc, plane, first, count, keys, vals, lid);
        threadgroup_barrier(mem_flags::mem_threadgroup);
        if (busy) {
            /* QK^T for this block's keys, as the transposed-B portable GEMM sums it */
            float4 acc[4];
            for (uint j = 0u; j < 4u; j++) acc[j] = float4(0.0f);
            for (uint c = 0u; c < 32u; c++) {
                float4 kv[4];
                for (uint j = 0u; j < 4u; j++) {
                    uint at = (cq + j * 16u) * 32u + c;
                    kv[j] = float4(float(keys[at]), float(keys[at + 32u]), float(keys[at + 64u]), float(keys[at + 96u]));
                }
                float qv = float(half_ptr(pc.a)[plane + (row + r) * 32u + c]);
                for (uint j = 0u; j < 4u; j++) acc[j] += qv * kv[j];
            }
            for (uint j = 0u; j < 4u; j++)
                for (uint e = 0u; e < 4u; e++) mine[r * 64u + cq + j * 16u + e] = acc[j][e];
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);
        if (busy)
            for (uint at = lane; at < 512u; at += 32u)
                mine[at] = ga_vit_weight(first + at % 64u < tokens ? mine[at] : 0.0f);
        threadgroup_barrier(mem_flags::mem_threadgroup);
        if (busy && lane < 8u) total = ga_block_total(total, mine + lane * 64u);
        threadgroup_barrier(mem_flags::mem_threadgroup);
        if (busy)
            for (uint at = lane; at < 512u; at += 32u)
                mine[at] = first + at % 64u < tokens ? e4m3(mine[at]) : 0.0f;
        threadgroup_barrier(mem_flags::mem_threadgroup);
        if (busy) {
            for (uint key = 0u; key < count; key++) {
                float4 vv[2];
                for (uint j = 0u; j < 2u; j++) {
                    uint at = key * 32u + cq + j * 16u;
                    vv[j] = float4(float(vals[at]), float(vals[at + 1u]), float(vals[at + 2u]), float(vals[at + 3u]));
                }
                float p = mine[r * 64u + key];
                for (uint j = 0u; j < 2u; j++) context[j] += p * vv[j];
            }
        }
    }
    if (busy && lane < 8u) reciprocal[slice * 8u + lane] = ga_reciprocal(total, blocks, tokens);
    threadgroup_barrier(mem_flags::mem_threadgroup);
    if (!busy) return;
    uint channels = heads * 32u;
    float scale = reciprocal[slice * 8u + r];
    for (uint j = 0u; j < 2u; j++) {
        half4 values;
        for (uint e = 0u; e < 4u; e++) values[e] = half(e4m3(hmul(half_round(context[j][e]), scale)));
        reinterpret_cast<device half4 *>(half_out(pc.c))[((row + r) * channels + head * 32u + cq + j * 16u) >> 2] = values;
    }
}
