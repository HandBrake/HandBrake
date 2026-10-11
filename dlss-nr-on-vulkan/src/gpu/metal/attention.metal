/*
 * attention — the row-wise operators, `attention.comp` in MSL: the cosine publish and the
 * bit-affine softmax, one invocation per row, the rows staged through threadgroup memory
 * so the global traffic is contiguous. The template parameter is the `-DSOFTMAX_AB` build
 * (`attention_ab`), whose bit-for-bit reference transform is test-only.
 */
#include "nr_metal.h"

// VIT_SOFTMAX is 6: 3-5 are the fused attention kernels' profile kinds
constant uint COSINE_PUBLISH = 0u, SOFTMAX = 1u, QKV_PREPARE = 2u, VIT_SOFTMAX = 6u;
constant float COSINE_NORM_FLOOR = 0.00006198883056640625f;

/* `count` consecutive floats from `base`, spread across the 32-wide threadgroup. */
inline void gather_part(constant Push &pc, uint flags, threadgroup float *stage, uint lid,
                        uint base, uint count, uint part) {
    device const float *a = float_ptr(pc.a);
    if ((flags & 0x20000u) != 0u) {
        // Q/K (and, for QKV_PREPARE, V) directly from (window, token, 3, head, channel)
        if (pc.n % 32u == 0u) {
            uint row = base / 32u;
            uint head = (row / pc.n) % pc.batch, token = row % pc.n;
            uint window = row / (pc.n * pc.batch);
            uint at = ((window * pc.n + token) * 3u * pc.batch + part * pc.batch + head) * 32u;
            for (uint i = lid; i < count; i += 32u)
                stage[i] = a[at + (i / 32u) * pc.batch * 96u + i % 32u];
        } else {
            for (uint i = lid; i < count; i += 32u) {
                uint row = (base + i) / 32u, channel = (base + i) % 32u;
                uint head = (row / pc.n) % pc.batch, token = row % pc.n;
                uint window = row / (pc.n * pc.batch);
                uint at = ((window * pc.n + token) * 3u * pc.batch + part * pc.batch + head) * 32u + channel;
                stage[i] = a[at];
            }
        }
    } else if ((flags & 0x8000u) != 0u) {
        device const half *h = half_ptr(pc.a);
        for (uint i = lid; i < count; i += 32u) stage[i] = float(h[base + i]);
    } else {
        for (uint i = lid; i < count; i += 32u) stage[i] = a[base + i];
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);
}

inline void gather(constant Push &pc, uint flags, threadgroup float *stage, uint lid,
                   uint base, uint count) {
    gather_part(pc, flags, stage, lid, base, count, (flags >> 18) & 1u);
}

inline void scatter_to(uint flags, threadgroup float *stage, uint lid, uint base, uint count, ulong target) {
    threadgroup_barrier(mem_flags::mem_threadgroup);
    if ((flags & 0x1000u) != 0u) {
        device half *h = half_out(target);
        for (uint i = lid; i < count; i += 32u) h[base + i] = half(stage[i]);
    } else {
        device float *c = float_out(target);
        for (uint i = lid; i < count; i += 32u) c[base + i] = stage[i];
    }
}

inline void scatter(constant Push &pc, uint flags, threadgroup float *stage, uint lid,
                    uint base, uint count) {
    scatter_to(flags, stage, lid, base, count, pc.c);
}

/* half(l*r), half(l+r), half(l*r + acc): one float32 expression, one rounding. The build
 * contracts nothing, so the multiply and the add stay two operations. */
inline float hmul(float l, float r) { float t = l * r; return half_round(t); }
inline float hadd(float l, float r) { float t = l + r; return half_round(t); }
inline float hfma(float l, float r, float acc) { float t = l * r; t = t + acc; return half_round(t); }

inline void store(constant Push &pc, uint flags, uint index, float value) {
    uint epilogue = (flags >> 8) & 0xFu;
    if (epilogue == 1u) value = e4m3(value);
    else if (epilogue == 4u) value = half_round(value);
    if ((flags & 0x1000u) != 0u) half_out(pc.c)[index] = half(value);
    else                         float_out(pc.c)[index] = value;
}

/* The ViT's normalisation (nr_epilogue.h's `vit_reciprocal`, cosine_tree.glsl): pair
 * squares summed in float32 and rounded once, a stride 8, 4 and 2 tree of half adds, the
 * last two pairs apart, and the reciprocal square root as a divide. */
inline float vit_reciprocal(thread const float *h) {
    float r[16];
    for (uint c = 0u; c < 16u; c++) r[c] = hfma(h[c], h[c], hmul(h[c + 16u], h[c + 16u]));
    for (uint c = 0u; c < 8u; c++) r[c] = hadd(r[c], r[c + 8u]);
    for (uint c = 0u; c < 4u; c++) r[c] = hadd(r[c], r[c + 4u]);
    float norm = max(hadd(hadd(r[0], r[2]), hadd(r[1], r[3])), half_round(COSINE_NORM_FLOOR));
    return half_round(1.0f / precise::sqrt(norm));
}

/* half(sqrt(32)), the ViT's query's own multiply before its learned scale */
constant float VIT_ROOT = 5.65625f;

/* Flag 0x80000: the ViT's normalisation and query scale. */
inline void cosine_publish(constant Push &pc, threadgroup float *stage, uint row, uint local,
                           bool scaled, ulong scales) {
    uint base = local * 32u;
    bool vit = (operation_flags(pc) & 0x80000u) != 0u;
    float h[32];
    for (uint i = 0u; i < 32u; i++) h[i] = half_round(stage[base + i]);
    if (vit) {
        float reciprocal = vit_reciprocal(h);
        float scale = 1.0f;
        if (scaled) scale = half_round(float_ptr(scales)[(row / pc.n) % pc.batch]);
        for (uint i = 0u; i < 32u; i++) {
            float value = hmul(h[i], reciprocal);
            if (scaled) value = hmul(hmul(value, VIT_ROOT), scale);
            stage[base + i] = e4m3(value);
        }
        return;
    }

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
    float reciprocal = half_round(precise::rsqrt(norm));
    float scale = 1.0f;
    if (scaled) {
        uint head = (row / pc.n) % pc.batch;      // rows are (batch, head, token)
        scale = half_round(float_ptr(scales)[head]);
    }
    for (uint i = 0u; i < 32u; i++) {
        float value = hmul(h[i], reciprocal);
        if (scaled) value = hmul(value, scale);
        stage[base + i] = e4m3(value);
    }
}

/* One pair of attention weights: the vendor's exp as a bit-affine map on an f16x2 register,
 * the pair coupled through the shift and the carry (notes/phase5-softmax-found.md). */
inline void weights_at_fast(constant Push &pc, uint flags, threadgroup const float *stage,
                            bool staged, uint base, uint bias, uint i,
                            thread float &w0, thread float &w1) {
    device const float *a = float_ptr(pc.a);
    device const float *b = float_ptr(pc.b);
    float2 affine;
    for (uint j = 0u; j < 2u; j++) {
        float logit = staged ? stage[base + i + j] : a[base + i + j];
        if ((flags & 0x2000u) != 0u) logit += b[bias + i + j];
        if (pc.p0 != 0.0f) logit = clamp(logit, -pc.p0, pc.p0);
        float scaled = half_round(logit) * 0.044921875f;
        scaled += 1.30078125f;
        affine[j] = clamp(scaled, 1.03125f, 1.5693359375f);
    }
    uint transformed = (as_type<uint>(half2(affine)) << 5) + 0x7FF88000u;
    float2 weights = float2(as_type<half2>(transformed));
    w0 = weights.x; w1 = weights.y;
}

inline void weights_at_reference(constant Push &pc, uint flags, threadgroup const float *stage,
                                 bool staged, uint base, uint bias, uint i,
                                 thread float &w0, thread float &w1) {
    device const float *a = float_ptr(pc.a);
    device const float *b = float_ptr(pc.b);
    uint bits[2];
    for (uint j = 0u; j < 2u; j++) {
        float logit = staged ? stage[base + i + j] : a[base + i + j];
        if ((flags & 0x2000u) != 0u) logit += b[bias + i + j];
        if (pc.p0 != 0.0f) logit = clamp(logit, -pc.p0, pc.p0);
        float scaled = half_round(logit) * 0.044921875f;
        scaled += 1.30078125f;
        float affine = clamp(scaled, 1.03125f, 1.5693359375f);
        int f = as_type<int>(half_round(affine));
        bits[j] = uint(((f >> 13) & 0x3FF) | (((((f >> 23) & 0xFF) - 112) & 0x1F) << 10)
                       | ((f < 0) ? 0x8000 : 0));
    }
    uint transformed = ((bits[0] | (bits[1] << 16)) << 5) + 0x7FF88000u;
    float out2[2];
    for (uint j = 0u; j < 2u; j++) {
        uint half_bits = (j == 0u) ? (transformed & 0xFFFFu) : ((transformed >> 16) & 0xFFFFu);
        uint exponent = (half_bits >> 10) & 0x1Fu, mantissa = half_bits & 0x3FFu;
        float weight = (exponent == 0u)
            ? float(mantissa) * 5.9604644775390625e-08f
            : as_type<float>(int(((exponent + 112u) << 23) | (mantissa << 13)));
        out2[j] = ((half_bits & 0x8000u) != 0u) ? -weight : weight;
    }
    w0 = out2[0]; w1 = out2[1];
}

template <bool AB>
inline void weights_at(constant Push &pc, uint flags, threadgroup const float *stage,
                       bool staged, uint base, uint bias, uint i,
                       thread float &w0, thread float &w1) {
    if (AB && (flags & 0x40000000u) != 0u) {
        weights_at_reference(pc, flags, stage, staged, base, bias, i, w0, w1);
        return;
    }
    weights_at_fast(pc, flags, stage, staged, base, bias, i, w0, w1);
}

template <bool AB>
inline void softmax(constant Push &pc, uint flags, threadgroup float *stage, uint row, uint local) {
    uint stride = pc.sa != 0u ? pc.sa : pc.n;
    bool staged = stride == pc.n && stride <= 64u;
    uint base = staged ? local * stride : row * stride;
    uint bias = ((row / pc.n) % max(pc.batch, 1u)) * pc.n * pc.n + (row % pc.n) * pc.n;
    if (!staged) for (uint i = pc.n; i < stride; i++) store(pc, flags, base + i, 0.0f);
    float total = 0.0f;
    float w0, w1;
    for (uint i = 0u; i < pc.n; i += 2u) {
        weights_at<AB>(pc, flags, stage, staged, base, bias, i, w0, w1);
        if (staged) { stage[base + i] = w0; stage[base + i + 1u] = w1; }
        total += w0;                              // float32 accumulation, as numpy does
        total += w1;
    }
    float reciprocal = half_round(1.0f / half_round(total));
    if (staged) {
        for (uint i = 0u; i < pc.n; i++)
            stage[base + i] = e4m3(hmul(stage[base + i], reciprocal));
    } else {
        for (uint i = 0u; i < pc.n; i += 2u) {
            weights_at<AB>(pc, flags, stage, false, base, bias, i, w0, w1);
            store(pc, flags, base + i,      e4m3(hmul(w0, reciprocal)));
            store(pc, flags, base + i + 1u, e4m3(hmul(w1, reciprocal)));
        }
    }
}

/* The ViT's exponential (OpenDLSS-NR's `vit_exp_weight`): its own affine, evaluated in
 * half, and a 4-bit shift where the window blocks' is 5. `attention.comp`'s vit_weight. */
inline float vit_weight(float logit) {
    float affine = half_round(logit) * 0.08953857421875f;
    affine += 1.708984375f;
    affine = clamp(half_round(affine), 1.439453125f, 1.9775390625f);
    uint bits = uint(as_type<ushort>(half(affine)));
    return float(as_type<half>(ushort(((bits << 4) + 0x4000u) & 0xFFFFu)));
}

/* The ViT's softmax, one row a lane (attention.comp, `vit_softmax`): the weights published
 * unnormalised into c, the row's reciprocal into d for the head merge; the denominator in
 * the vendor's tree order over 64-key blocks padded with zero keys, whose weight is taken
 * off at the end. Columns past the tokens count as padding and publish zero. */
inline void vit_softmax(constant Push &pc, uint flags, uint row) {
    uint stride = pc.sa != 0u ? pc.sa : pc.n, tokens = pc.n, base = row * stride;
    uint blocks = (tokens + 63u) / 64u;
    device const float *a = float_ptr(pc.a);
    float zero = vit_weight(0.0f), total = 0.0f;
    for (uint b = 0u; b < blocks; b++) {
        float column[8];
        for (uint c = 0u; c < 8u; c++) {
            float pair[4];
            for (uint j = 0u; j < 4u; j++) {
                uint k0 = b * 64u + c + 16u * j, k1 = k0 + 8u;
                float w0 = k0 < tokens ? vit_weight(a[base + k0]) : zero;
                float w1 = k1 < tokens ? vit_weight(a[base + k1]) : zero;
                pair[j] = hadd(w0, w1);
            }
            column[c] = hadd(hadd(hadd(pair[0], pair[1]), pair[2]), pair[3]);
        }
        float even = hadd(hadd(hadd(column[0], column[2]), column[4]), column[6]);
        float odd = hadd(hadd(hadd(column[1], column[3]), column[5]), column[7]);
        total = hadd(total, hadd(even, odd));
    }
    uint padding = blocks * 64u - tokens;
    if (padding > 0u) total = hadd(total, -hmul(zero, float(padding)));
    float_out(pc.d)[row] = half_round(1.0f / total);
    for (uint k = 0u; k < stride; k++)
        store(pc, flags, base + k, k < tokens ? e4m3(vit_weight(a[base + k])) : 0.0f);
}

/* -- the rows too wide to stage 32 at a time (attention.comp softmax_rows) ------------ */

/* A logit as the transform takes it: the per-head bias added (bit 13), then the clamp. */
inline float prepared(constant Push &pc, uint flags, float logit, uint bias_index) {
    if ((flags & 0x2000u) != 0u) logit += float_ptr(pc.b)[bias_index];
    if (pc.p0 != 0.0f) logit = clamp(logit, -pc.p0, pc.p0);
    return logit;
}

/* weights_at_fast / _reference on two prepared logits */
template <bool AB>
inline float2 pair_weights(uint flags, float l0, float l1) {
    if (AB && (flags & 0x40000000u) != 0u) {
        uint bits[2];
        for (uint j = 0u; j < 2u; j++) {
            float scaled = half_round(j == 0u ? l0 : l1) * 0.044921875f;
            scaled += 1.30078125f;
            float affine = clamp(scaled, 1.03125f, 1.5693359375f);
            int f = as_type<int>(half_round(affine));
            bits[j] = uint(((f >> 13) & 0x3FF) | (((((f >> 23) & 0xFF) - 112) & 0x1F) << 10)
                           | ((f < 0) ? 0x8000 : 0));
        }
        uint transformed = ((bits[0] | (bits[1] << 16)) << 5) + 0x7FF88000u;
        float out2[2];
        for (uint j = 0u; j < 2u; j++) {
            uint half_bits = (j == 0u) ? (transformed & 0xFFFFu) : ((transformed >> 16) & 0xFFFFu);
            uint exponent = (half_bits >> 10) & 0x1Fu, mantissa = half_bits & 0x3FFu;
            float weight = (exponent == 0u)
                ? float(mantissa) * 5.9604644775390625e-08f
                : as_type<float>(int(((exponent + 112u) << 23) | (mantissa << 13)));
            out2[j] = ((half_bits & 0x8000u) != 0u) ? -weight : weight;
        }
        return float2(out2[0], out2[1]);
    }
    float2 affine;
    for (uint j = 0u; j < 2u; j++) {
        float scaled = half_round(j == 0u ? l0 : l1) * 0.044921875f;
        scaled += 1.30078125f;
        affine[j] = clamp(scaled, 1.03125f, 1.5693359375f);
    }
    uint transformed = (as_type<uint>(half2(affine)) << 5) + 0x7FF88000u;
    return float2(as_type<half2>(transformed));
}

inline uint bias_offset(constant Push &pc, uint row) {
    return ((row / pc.n) % max(pc.batch, 1u)) * pc.n * pc.n + (row % pc.n) * pc.n;
}

/* The global blocks' rows, as many columns as the bottleneck has tokens: a threadgroup
 * takes as many whole rows as fit in its 8 KB, `stride + 1` floats apart, reads them once
 * along the rows, makes the weights in place, and each of its first lanes adds its own row
 * in index order — the one-row loop's order, so the same total bit for bit. The last 32
 * floats hold the reciprocals, so at most 32 rows a threadgroup, as libmetalmx
 * dispatches it. A pair starting below `n` counts both halves; the pad past it is zeroed. */
constant uint WHOLE = 2016u;

template <bool AB, uint LANES>
inline void softmax_rows(constant Push &pc, uint flags, threadgroup float *stage, uint local, uint group) {
    uint stride = pc.sa != 0u ? pc.sa : pc.n, pitch = stride + 1u;
    uint per = min(32u, WHOLE / pitch), first = group * per;
    uint rows = min(per, pc.m - first);
    uint paired = (pc.n + 1u) & ~1u, pairs = paired / 2u;
    device const float *a = float_ptr(pc.a);
    for (uint q = local; q < rows * stride; q += LANES)
        stage[(q / stride) * pitch + q % stride] = a[first * stride + q];
    threadgroup_barrier(mem_flags::mem_threadgroup);
    for (uint q = local; q < rows * pairs; q += LANES) {
        uint r = q / pairs, j = (q % pairs) * 2u, at = r * pitch + j;
        uint bias = bias_offset(pc, first + r) + j;
        float2 w = pair_weights<AB>(flags, prepared(pc, flags, stage[at], bias),
                                    prepared(pc, flags, stage[at + 1u], bias + 1u));
        stage[at] = w.x;
        stage[at + 1u] = w.y;
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);
    if (local < rows) {
        float total = 0.0f;
        uint base = local * pitch;
        for (uint j = 0u; j < paired; j++) total += stage[base + j];
        stage[WHOLE + local] = half_round(1.0f / half_round(total));
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);
    for (uint q = local; q < rows * stride; q += LANES) {
        uint r = q / stride, j = q % stride;
        store(pc, flags, first * stride + q,
              j < paired ? e4m3(hmul(stage[r * pitch + j], stage[WHOLE + r])) : 0.0f);
    }
}

template <bool AB>
kernel void attention_t(constant Push &pc [[buffer(0)]],
                        uint local [[thread_index_in_threadgroup]],
                        uint3 group3 [[threadgroup_position_in_grid]]) {
    threadgroup float stage[32 * 64];
    uint group = group3.x;
    uint row = group * 32u + local;
    uint flags = operation_flags(pc);
    uint kind = flags & 0xFFu;
    if (kind == QKV_PREPARE) {
        /* Three independent planes on y: Q and K normalised (Q by its head's scale, read
         * from the 64-bit address in lda and ldb), V published; each into its own
         * target — b, c, d. The arithmetic is the two cosine publishes' and the split's. */
        uint part = group3.y;
        uint base = group * 32u * 32u;
        uint count = min(32u * 32u, pc.m * 32u - base);
        gather_part(pc, flags, stage, local, base, count, part);
        if (part < 2u) {
            ulong scales = push_address(pc.lda);
            if (row < pc.m) cosine_publish(pc, stage, row, local, part == 0u, scales);
        } else {
            for (uint i = local; i < count; i += 32u) stage[i] = e4m3(stage[i]);
        }
        ulong target = part == 0u ? pc.b : (part == 1u ? pc.c : pc.d);
        scatter_to(flags, stage, local, base, count, target);
    } else if (kind == COSINE_PUBLISH) {
        uint base = group * 32u * 32u;
        gather(pc, flags, stage, local, base, min(32u * 32u, pc.m * 32u - base));
        if (row < pc.m) cosine_publish(pc, stage, row, local, pc.k != 0u, pc.d);
        scatter(pc, flags, stage, local, base, min(32u * 32u, pc.m * 32u - base));
    } else if (kind == VIT_SOFTMAX) {
        if (row < pc.m) vit_softmax(pc, flags, row);
    } else if (kind == SOFTMAX) {
        uint stride = pc.sa != 0u ? pc.sa : pc.n;
        if (stride == pc.n && stride <= 64u) {
            uint base = group * 32u * stride;
            uint count = min(32u * stride, pc.m * stride - base);
            gather(pc, flags, stage, local, base, count);
            if (row < pc.m) softmax<AB>(pc, flags, stage, row, local);
            scatter(pc, flags, stage, local, base, count);
        } else if (stride + 1u <= WHOLE) {
            softmax_rows<AB, 32u>(pc, flags, stage, local, group);
        } else if (row < pc.m) {
            softmax<AB>(pc, flags, stage, row, local);     /* the one-row loop, softmax_long */
        }
    }
}

/* The whole-row softmax on 256 threads (attention.comp built with ROW_LANES=256): the same
 * 8 KB serving eight simdgroups instead of one. libmetalmx routes only those rows here. */
kernel void attention_rows(constant Push &pc [[buffer(0)]],
                           uint local [[thread_index_in_threadgroup]],
                           uint3 group3 [[threadgroup_position_in_grid]]) {
    threadgroup float stage[32 * 64];
    softmax_rows<false, 256u>(pc, operation_flags(pc), stage, local, group3.x);
}

template [[host_name("attention")]]    kernel void attention_t<false>(constant Push &, uint, uint3);
template [[host_name("attention_ab")]] kernel void attention_t<true>(constant Push &, uint, uint3);
