/*
 * window_attention — QK^T, the bit-affine softmax and PV for one 8x8 window in one
 * dispatch: `window_attention.comp` (and `window_attention_portable.comp`) in MSL.
 *
 * Each must equal, bit for bit, this runtime's own three-pass path on the same kernels:
 * `window_attention` the simdgroup GEMMs (8-wide K slices from 0 upwards, as gemm_simd
 * takes them, and the transposed key load of its B tile), `window_attention_portable` the
 * portable GEMMs (a float32 accumulator taking the K terms one at a time). The softmax
 * between them is the row pass's (`attention.metal`): the same coupled half-pair weights,
 * the float32 denominator in key order, `half_round(1 / half_round(total))`, and each
 * probability `e4m3(half_round(w * reciprocal))` held as half.
 *
 * Push: a = Q, b = K, d = V (half, (window, head, token, 32)), c = the output, n = heads,
 * batch = batches, flags = 1 with a bias at residual_cos (fp32, (head, query, key)).
 * Grid (1, batches up to 65535, the rest on z), 256 threads: one threadgroup a window and
 * head, its K and V loaded into threadgroup memory once, a 16-byte load a thread, and each
 * of its eight simdgroups (on the portable kernel, each 32-thread slice) taking eight
 * query rows — `window_attention.comp`'s change, which on Xe2 halved the pass because
 * every eight-row threadgroup had fetched the whole window's K and V for itself. Function
 * constant 1 selects the merged output: E4M3 halves in (window, token, C) order,
 * merge_heads' work too.
 *
 * Threadgroup memory, 16 KB for both: K and V at 4 KB each and a kilobyte a simdgroup (or
 * slice), the row reciprocals in K's bytes once every simdgroup is past QK^T. The
 * simdgroup kernel stages its logits in that kilobyte half the keys at a time, as
 * `window_attention.comp` does, keeping each pair's weight in registers until both halves
 * are done with the bytes; the portable one never stages them, a thread's two adjacent
 * keys being both its own. Measured on an M3 at 1280x768, the simdgroup kernel went from
 * 39.4 to 31.1 ms a frame this way — and to 51.0 at 32 KB with the logits and the
 * probabilities in separate tiles, 36.7 at 24 KB with the whole logit row staged, so the
 * footprint is what decides it here; the portable one went from 64.4 to 55.5. No value and no order of summation changes.
 */
#include "nr_epilogue.h"

constant bool MERGED_CONSTANT [[function_constant(1)]];
constant bool merged_output = is_function_constant_defined(MERGED_CONSTANT) && MERGED_CONSTANT;

/* The coupled half-pair transform on two adjacent logits of one row, with the float32
 * bias added before the half rounding. */
inline float2 wa_weights(constant Push &pc, float2 logits, uint bias) {
    float2 affine;
    for (uint j = 0u; j < 2u; j++) {
        float logit = logits[j];
        if (pc.flags != 0u) logit += float_ptr(pc.residual_cos)[bias + j];
        float scaled = half_round(logit) * 0.044921875f;
        scaled += 1.30078125f;
        affine[j] = clamp(scaled, 1.03125f, 1.5693359375f);
    }
    uint transformed = (as_type<uint>(half2(affine)) << 5) + 0x7FF88000u;
    return float2(as_type<half2>(transformed));
}

/* K and V of one window-head into threadgroup memory, 16 bytes a thread of 256. */
inline void wa_load_kv(constant Push &pc, uint offset, threadgroup uint4 *kv, uint lid) {
    kv[lid] = reinterpret_cast<device const uint4 *>(half_ptr(pc.b) + offset)[lid];
    kv[256u + lid] = reinterpret_cast<device const uint4 *>(half_ptr(pc.d) + offset)[lid];
}

inline uint merged_target(constant Push &pc, uint batch, uint row, uint r, uint c) {
    return ((batch / pc.n) * 64u + row + r) * pc.n * 32u + (batch % pc.n) * 32u + c;
}

kernel void window_attention(constant Push &pc [[buffer(0)]],
                             uint3 wg [[threadgroup_position_in_grid]],
                             uint lid [[thread_index_in_threadgroup]],
                             uint sg [[simdgroup_index_in_threadgroup]],
                             uint sl [[thread_index_in_simdgroup]]) {
    threadgroup uint words[4096];                /* 16 KB, see the header */
    uint batch = wg.y + wg.z * 65535u;
    if (batch >= pc.batch) return;
    uint offset = batch * 2048u;
    /* K transposed on its way in, (32 channels, 64 keys), so QK^T's B tiles are plain
     * loads rather than transposed ones; V as it is */
    {
        threadgroup half *KT = reinterpret_cast<threadgroup half *>(words);
        uint4 k8 = reinterpret_cast<device const uint4 *>(half_ptr(pc.b) + offset)[lid];
        uint token = lid / 4u, c0 = (lid % 4u) * 8u;
        for (uint w = 0u; w < 4u; w++) {
            half2 two = as_type<half2>(k8[w]);
            KT[(c0 + 2u * w) * 64u + token] = two.x;
            KT[(c0 + 2u * w + 1u) * 64u + token] = two.y;
        }
        reinterpret_cast<threadgroup uint4 *>(words)[256u + lid] =
            reinterpret_cast<device const uint4 *>(half_ptr(pc.d) + offset)[lid];
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);
    threadgroup const half *K = reinterpret_cast<threadgroup const half *>(words);
    threadgroup const half *V = K + 2048u;
    /* this simdgroup's kilobyte: half its rows' logits (8 x 32 float), then all their
     * weights and probabilities (8 x 64 half), then the merged output's stage */
    threadgroup uint *mine = words + 2048u + sg * 256u;
    threadgroup float *stage = reinterpret_cast<threadgroup float *>(mine);
    threadgroup half *probs = reinterpret_cast<threadgroup half *>(mine);
    threadgroup float *reciprocal = reinterpret_cast<threadgroup float *>(words) + sg * 8u;

    uint row = sg * 8u;
    uint head_bias = (batch % pc.n) * 4096u + row * 64u;
    device const half *Q = half_ptr(pc.a) + offset;

    simdgroup_float8x8 acc[8];
    for (int j = 0; j < 8; j++) acc[j] = simdgroup_float8x8(0.0f);
    for (uint c = 0u; c < 32u; c += 8u) {
        simdgroup_half8x8 q;
        simdgroup_load(q, Q + row * 32u + c, 32);
        for (uint j = 0u; j < 8u; j++) {
            simdgroup_half8x8 k;
            simdgroup_load(k, K + c * 64u + j * 8u, 64);
            simdgroup_multiply_accumulate(acc[j], q, k, acc[j]);
        }
    }
    /* keys 0-31, then 32-63, through the same kilobyte, each pair of adjacent keys becoming
     * its weight in registers; the pairs never straddle the halves */
    uint kept[8];
    for (uint half_keys = 0u; half_keys < 2u; half_keys++) {
        for (uint j = 0u; j < 4u; j++) simdgroup_store(acc[half_keys * 4u + j], stage + j * 8u, 32);
        simdgroup_barrier(mem_flags::mem_threadgroup);
        for (uint t = 0u; t < 4u; t++) {
            uint at = sl * 2u + t * 64u, rr = at / 32u, key = half_keys * 32u + at % 32u;
            float2 w = wa_weights(pc, float2(stage[at], stage[at + 1u]), head_bias + rr * 64u + key);
            kept[half_keys * 4u + t] = as_type<uint>(half2(w));          /* exact: half values */
        }
        simdgroup_barrier(mem_flags::mem_threadgroup);
    }
    for (uint half_keys = 0u; half_keys < 2u; half_keys++)
        for (uint t = 0u; t < 4u; t++) {
            uint at = sl * 2u + t * 64u;
            mine[((at / 32u) * 64u + half_keys * 32u + at % 32u) / 2u] = kept[half_keys * 4u + t];
        }
    /* every simdgroup is past QK^T after this, so K's bytes may take the reciprocals */
    threadgroup_barrier(mem_flags::mem_threadgroup);
    if (sl < 8u) {
        float total = 0.0f;
        for (uint i = 0u; i < 64u; i += 2u) {
            float2 two = float2(as_type<half2>(mine[(sl * 64u + i) / 2u]));
            total += two.x;
            total += two.y;
        }
        reciprocal[sl] = half_round(1.0f / half_round(total));
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);
    for (uint at = sl * 2u; at < 8u * 64u; at += 64u) {
        float scale = reciprocal[at / 64u];
        float2 pair = float2(as_type<half2>(mine[at / 2u]));
        mine[at / 2u] = as_type<uint>(half2(float2(e4m3(hmul(pair.x, scale)),
                                                   e4m3(hmul(pair.y, scale)))));
    }
    simdgroup_barrier(mem_flags::mem_threadgroup);

    simdgroup_float8x8 out[4];
    for (int j = 0; j < 4; j++) out[j] = simdgroup_float8x8(0.0f);
    for (uint c = 0u; c < 64u; c += 8u) {
        simdgroup_half8x8 p;
        simdgroup_load(p, probs + c, 64);
        for (uint j = 0u; j < 4u; j++) {
            simdgroup_half8x8 v;
            simdgroup_load(v, V + c * 32u + j * 8u, 32);
            simdgroup_multiply_accumulate(out[j], p, v, out[j]);
        }
    }
    if (!merged_output) {
        for (uint j = 0u; j < 4u; j++)
            simdgroup_store(out[j], float_out(pc.c) + offset + row * 32u + j * 8u, 32);
        return;
    }
    /* the probabilities and the output stage are the same bytes */
    simdgroup_barrier(mem_flags::mem_threadgroup);
    for (uint j = 0u; j < 4u; j++) simdgroup_store(out[j], stage + j * 8u, 32);
    simdgroup_barrier(mem_flags::mem_threadgroup);
    for (uint at = sl * 4u; at < 8u * 32u; at += 128u) {
        float4 values;
        for (uint q = 0u; q < 4u; q++) values[q] = e4m3(stage[at + q]);
        uint target = merged_target(pc, batch, row, at / 32u, at % 32u);
        reinterpret_cast<device half4 *>(half_out(pc.c))[target / 4u] = half4(values);
    }
}

kernel void window_attention_portable(constant Push &pc [[buffer(0)]],
                                      uint3 wg [[threadgroup_position_in_grid]],
                                      uint lid [[thread_index_in_threadgroup]]) {
    threadgroup uint words[4096];                /* 16 KB, see the header */
    uint batch = wg.y + wg.z * 65535u;
    if (batch >= pc.batch) return;
    uint offset = batch * 2048u;
    wa_load_kv(pc, offset, reinterpret_cast<threadgroup uint4 *>(words), lid);
    threadgroup_barrier(mem_flags::mem_threadgroup);
    threadgroup const half *K = reinterpret_cast<threadgroup const half *>(words);
    threadgroup const half *V = K + 2048u;

    uint slice = lid >> 5u, local = lid & 31u, row = slice * 8u;
    threadgroup uint *mine = words + 2048u + slice * 256u;         /* 8 x 64 weights */
    threadgroup float *reciprocal = reinterpret_cast<threadgroup float *>(words) + row;
    uint head_bias = (batch % pc.n) * 4096u + row * 64u;
    device const half *Q = half_ptr(pc.a) + offset;
    /* the portable GEMM's lane: row r of the block, four consecutive columns cq.. */
    uint r = local >> 2u, cq = (local & 3u) * 4u;

    {   /* QK^T as the transposed-B portable GEMM: acc += q[row][c] * k[key][c], c in order */
        float4 acc[4];
        for (int j = 0; j < 4; j++) acc[j] = float4(0.0f);
        for (uint c = 0u; c < 32u; c++) {
            float4 kv[4];
            for (uint j = 0u; j < 4u; j++) {
                uint at = (cq + j * 16u) * 32u + c;
                kv[j] = float4(float(K[at]), float(K[at + 32u]), float(K[at + 64u]), float(K[at + 96u]));
            }
            float qv = float(Q[(row + r) * 32u + c]);
            for (uint j = 0u; j < 4u; j++) acc[j] += qv * kv[j];
        }
        /* each pair of adjacent keys becomes its weight here, both being this thread's */
        for (uint j = 0u; j < 4u; j++)
            for (uint e = 0u; e < 4u; e += 2u) {
                uint key = cq + j * 16u + e;
                float2 w = wa_weights(pc, float2(acc[j][e], acc[j][e + 1u]), head_bias + r * 64u + key);
                mine[(r * 64u + key) / 2u] = as_type<uint>(half2(w));   /* exact: half values */
            }
    }
    /* every slice is past QK^T after this, so K's bytes may take the reciprocals */
    threadgroup_barrier(mem_flags::mem_threadgroup);
    if (local < 8u) {
        float total = 0.0f;
        for (uint i = 0u; i < 64u; i += 2u) {
            float2 two = float2(as_type<half2>(mine[(local * 64u + i) / 2u]));
            total += two.x;
            total += two.y;
        }
        reciprocal[local] = half_round(1.0f / half_round(total));
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);
    for (uint at = local * 2u; at < 8u * 64u; at += 64u) {
        float scale = reciprocal[at / 64u];
        float2 pair = float2(as_type<half2>(mine[at / 2u]));
        mine[at / 2u] = as_type<uint>(half2(float2(e4m3(hmul(pair.x, scale)),
                                                   e4m3(hmul(pair.y, scale)))));
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);
    threadgroup const half *probs = reinterpret_cast<threadgroup const half *>(mine);

    /* PV as the plain portable GEMM: acc += p[row][key] * v[key][col], key in order */
    float4 acc[2];
    for (int j = 0; j < 2; j++) acc[j] = float4(0.0f);
    for (uint key = 0u; key < 64u; key++) {
        float4 vv[2];
        for (uint j = 0u; j < 2u; j++) {
            uint at = key * 32u + cq + j * 16u;
            vv[j] = float4(float(V[at]), float(V[at + 1u]), float(V[at + 2u]), float(V[at + 3u]));
        }
        float p = float(probs[r * 64u + key]);
        for (uint j = 0u; j < 2u; j++) acc[j] += p * vv[j];
    }
    for (uint j = 0u; j < 2u; j++) {
        if (merged_output) {
            float4 values;
            for (uint q = 0u; q < 4u; q++) values[q] = e4m3(acc[j][q]);
            uint target = merged_target(pc, batch, row, r, cq + j * 16u);
            reinterpret_cast<device half4 *>(half_out(pc.c))[target / 4u] = half4(values);
        } else {
            for (uint q = 0u; q < 4u; q++)
                float_out(pc.c)[offset + (row + r) * 32u + cq + j * 16u + q] = acc[j][q];
        }
    }
}
