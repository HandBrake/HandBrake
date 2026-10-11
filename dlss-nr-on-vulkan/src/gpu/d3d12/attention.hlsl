/*
 * attention — the row-wise operators, `attention.comp` in HLSL: the cosine publish and
 * the bit-affine softmax, one thread per row, the group's rows staged through group
 * shared memory so the global traffic is contiguous. -DSOFTMAX_AB is the `attention_ab`
 * build, whose bit-for-bit reference transform is test-only.
 *
 * Both reductions are reproduced in the order the reference uses: `sum(dtype=float16)`
 * in numpy runs sequentially in float32 and rounds once at the end, and the cosine
 * normalise uses a fixed fragment tree of half operations whose shape is the kernel's.
 * The staging is fenced with group-wide barriers, as the GLSL is since `phase73`.
 */
#include "nr_d3d.hlsli"
#include "nr_epilogue.hlsli"      /* hmul/hadd/hfma and cosine_reciprocal: one definition */

// VIT_SOFTMAX is 6: 3-5 are the fused attention kernels' profile kinds
static const uint COSINE_PUBLISH = 0u, SOFTMAX = 1u, QKV_PREPARE = 2u, VIT_SOFTMAX = 6u;

/* ROW_LANES: 32 for every row pass; the `attention_rows` build takes 256, for the
 * whole-row softmax alone — the same 8 KB of shared memory serving eight times the lanes
 * (attention.comp, softmax_rows). */
#ifndef ROW_LANES
#define ROW_LANES 32
#endif

groupshared float stage[32 * 64];

/* `count` consecutive floats from `base`, spread across the 32-wide group. */
void gather_part(uint flags, uint lid, uint base, uint count, uint part) {
    uint A = pc.oa.x;
    if ((flags & 0x20000u) != 0u) {
        // Q/K directly from (window, token, 3, head, channel). The cosine operation
        // performs the split's half rounding before its reduction.
        if (pc.n % 32u == 0u) {
            // A group's 32 rows stay in one head: compute its base once.
            uint row = base / 32u;
            uint head = (row / pc.n) % pc.batch, token = row % pc.n;
            uint window = row / (pc.n * pc.batch);
            uint at = ((window * pc.n + token) * 3u * pc.batch + part * pc.batch + head) * 32u;
            for (uint i = lid; i < count; i += 32u)
                stage[i] = ld_f32(bufA, A, at + (i / 32u) * pc.batch * 96u + i % 32u);
        } else {
            for (uint i = lid; i < count; i += 32u) {
                uint row = (base + i) / 32u, channel = (base + i) % 32u;
                uint head = (row / pc.n) % pc.batch, token = row % pc.n;
                uint window = row / (pc.n * pc.batch);
                uint at = ((window * pc.n + token) * 3u * pc.batch + part * pc.batch + head) * 32u + channel;
                stage[i] = ld_f32(bufA, A, at);
            }
        }
    } else if ((flags & 0x8000u) != 0u) {
        /* Bit 15: the input is float16. The cosine publish rounds its input to half as
         * its first act, so a producer that already wrote half loses nothing by it. */
        for (uint i = lid; i < count; i += 32u) stage[i] = ld_f16(bufA, A, base + i);
    } else {
        for (uint i = lid; i < count; i += 32u) stage[i] = ld_f32(bufA, A, base + i);
    }
    GroupMemoryBarrierWithGroupSync();
}

void gather(uint flags, uint lid, uint base, uint count) {
    gather_part(flags, lid, base, count, (flags >> 18) & 1u);
}

/* `stage` to one of the operand resources: 1 = b, 2 = c, 3 = d. The target is uniform
 * over the group, so each branch names one resource, as HLSL wants it. */
void scatter_rows(RWByteAddressBuffer target, uint offset, bool narrow, uint lid, uint count, uint base) {
    if (narrow) {
        for (uint i = lid; i < count; i += 32u) st_f16(target, offset, base + i, stage[i]);
    } else {
        for (uint i = lid; i < count; i += 32u) st_f32(target, offset, base + i, stage[i]);
    }
}

void scatter_to(uint flags, uint lid, uint base, uint count, uint which) {
    GroupMemoryBarrierWithGroupSync();
    bool narrow = (flags & 0x1000u) != 0u;
    if (which == 1u)      scatter_rows(bufB, pc.ob.x, narrow, lid, count, base);
    else if (which == 3u) scatter_rows(bufD, pc.od.x, narrow, lid, count, base);
    else                  scatter_rows(bufC, pc.oc.x, narrow, lid, count, base);
}

void scatter(uint flags, uint lid, uint base, uint count) { scatter_to(flags, lid, base, count, 2u); }

/* The publish this pass ends with: bits 8-11 of `flags` pick the transform and bit 12
 * narrows the output to float16. Both of these rows end in an E4M3 value, which is exact
 * in half, so narrowing here changes nothing. */
void store(uint flags, uint index, float value) {
    uint epilogue = (flags >> 8) & 0xFu;
    if (epilogue == 1u) value = e4m3(value);
    else if (epilogue == 4u) value = half_round(value);
    if ((flags & 0x1000u) != 0u) st_f16(bufC, pc.oc.x, index, value);
    else                         st_f32(bufC, pc.oc.x, index, value);
}

/* `scale_from`: 0 none, 1 the scale in d (the plain cosine publish, `pc.k != 0`), 2 the
 * scale at push offset 120 (QKV_PREPARE, where d is the V target). Flag 0x80000: the ViT's
 * normalisation and query scale (`vit_reciprocal`, VIT_ROOT). */
void cosine_publish(uint row, uint local, uint scale_from) {
    uint base = local * 32u;
    bool vit = (operation_flags() & 0x80000u) != 0u;
    float h[32];
    [unroll] for (uint i = 0u; i < 32u; i++) h[i] = half_round(stage[base + i]);
    float reciprocal = vit ? vit_reciprocal(h) : cosine_reciprocal(h);   // nr_epilogue.hlsli
    float scale = 1.0;
    uint head = (row / pc.n) % pc.batch;              // rows are (batch, head, token)
    if (scale_from == 1u) scale = half_round(ld_f32(bufD, pc.od.x, head));
    else if (scale_from == 2u) scale = half_round(ld_f32(bufF, pc.of.x, head));
    [unroll] for (uint i = 0u; i < 32u; i++) {
        float value = hmul(h[i], reciprocal);
        if (scale_from != 0u) value = hmul(vit ? hmul(value, VIT_ROOT) : value, scale);
        stage[base + i] = e4m3(value);
    }
}

/* A logit as the transform takes it: the per-head attention bias added here rather than
 * by a pass of its own — it is a few tens of kilobytes and stays in cache, so folding it
 * in saves a whole read-modify-write of the scores — then the symmetric clamp. */
float prepared(uint flags, float logit, uint bias_index) {
    if ((flags & 0x2000u) != 0u) logit += ld_f32(bufB, pc.ob.x, bias_index);
    if (pc.p0 != 0.0) logit = clamp(logit, -pc.p0, pc.p0);
    return logit;
}

/* One pair of attention weights from two prepared logits. The vendor's exp is a
 * bit-affine map on an `f16x2` register, so the pair is coupled — the shift moves bits
 * across the halves and the add can carry between them — and the two elements cannot be
 * computed apart (notes/phase5-softmax-found.md). The clamped affine values encode as
 * 0x3c20..0x3e47, and for every pair the transform below yields finite normal half values,
 * so the native pack and unpack reproduce the bit reconstruction exactly, carry included. */
void pair_fast(float l0, float l1, out float w0, out float w1) {
    float affine[2];
    [unroll] for (uint j = 0u; j < 2u; j++) {
        precise float scaled = half_round(j == 0u ? l0 : l1) * 0.044921875;
        scaled += 1.30078125;
        affine[j] = clamp(scaled, 1.03125, 1.5693359375);
    }
    uint packed = f32tof16(affine[0]) | (f32tof16(affine[1]) << 16);
    uint transformed = (packed << 5) + 0x7FF88000u;
    w0 = f16tof32(transformed & 0xFFFFu);
    w1 = f16tof32(transformed >> 16);
}

#ifdef SOFTMAX_AB
void pair_reference(float l0, float l1, out float w0, out float w1) {
    uint bits[2];
    [unroll] for (uint j = 0u; j < 2u; j++) {
        precise float scaled = half_round(j == 0u ? l0 : l1) * 0.044921875;
        scaled += 1.30078125;
        float affine = clamp(scaled, 1.03125, 1.5693359375);
        // the float16 bit pattern of the affine value
        int f = asint(half_round(affine));
        bits[j] = uint(((f >> 13) & 0x3FF) | (((((f >> 23) & 0xFF) - 112) & 0x1F) << 10)
                       | ((f < 0) ? 0x8000 : 0));
    }
    uint transformed = ((bits[0] | (bits[1] << 16)) << 5) + 0x7FF88000u;
    float out2[2];
    [unroll] for (uint j = 0u; j < 2u; j++) {
        uint half_bits = (j == 0u) ? (transformed & 0xFFFFu) : ((transformed >> 16) & 0xFFFFu);
        uint exponent = (half_bits >> 10) & 0x1Fu, mantissa = half_bits & 0x3FFu;
        float weight = (exponent == 0u)
            ? float(mantissa) * 5.9604644775390625e-08
            : asfloat(int(((exponent + 112u) << 23) | (mantissa << 13)));
        out2[j] = ((half_bits & 0x8000u) != 0u) ? -weight : weight;
    }
    w0 = out2[0]; w1 = out2[1];
}
#endif

void pair_weights(uint flags, float l0, float l1, out float w0, out float w1) {
#ifdef SOFTMAX_AB
    // Test-only specialization bit; removed entirely from the shipped shader.
    if ((flags & 0x40000000u) != 0u) {
        pair_reference(l0, l1, w0, w1);
        return;
    }
#endif
    pair_fast(l0, l1, w0, w1);
}

void weights_at(uint flags, bool staged, uint base, uint bias, uint i, out float w0, out float w1) {
    float l0 = staged ? stage[base + i] : ld_f32(bufA, pc.oa.x, base + i);
    float l1 = staged ? stage[base + i + 1u] : ld_f32(bufA, pc.oa.x, base + i + 1u);
    pair_weights(flags, prepared(flags, l0, bias + i), prepared(flags, l1, bias + i + 1u), w0, w1);
}

/* scores are (windows*heads, tokens, tokens) and the bias (heads, tokens, tokens) */
uint bias_offset(uint row) {
    return ((row / pc.n) % max(pc.batch, 1u)) * pc.n * pc.n + (row % pc.n) * pc.n;
}

/* A row short enough to stage whole — every window's, 64 tokens — one row a lane, out of
 * the rows the group gathered into shared memory. */
void softmax(uint flags, uint row, uint local) {
    uint base = local * pc.n;
    uint bias = bias_offset(row);
    float total = 0.0;
    float w0, w1;
    for (uint i = 0u; i < pc.n; i += 2u) {
        weights_at(flags, true, base, bias, i, w0, w1);
        /* The weights are parked where the logits were, so the normalising pass does not
         * repeat the bit-affine transform: the output buffer is the one being narrowed,
         * and a rounded weight would give a different product. */
        stage[base + i] = w0; stage[base + i + 1u] = w1;
        total += w0;                              // float32 accumulation, as numpy does
        total += w1;
    }
    precise float reciprocal = 1.0 / half_round(total);
    reciprocal = half_round(reciprocal);
    for (uint i = 0u; i < pc.n; i++)
        stage[base + i] = e4m3(hmul(stage[base + i], reciprocal));
}

/* The rows too wide to stage 32 at a time — the global blocks', as many columns as the
 * bottleneck has tokens, padded in `sa` — attention.comp's softmax_rows: a group takes as
 * many whole rows as fit in its shared memory, `stride + 1` floats apart, reads them once
 * along the rows, turns them into weights in place, and each of its first lanes adds its
 * own row in index order — the same weights in the same order as the one-row-a-lane loop,
 * so the same total — then the whole group normalises and stores along the rows again. A
 * pair that starts below `n` counts both halves, and the pad past it is zeroed. The last 32
 * floats hold the reciprocals, so a group takes at most 32 rows; libd3dmx dispatches one per
 * `min(32, WHOLE / (stride + 1))` to match. */
static const uint WHOLE = 2016u;

void softmax_rows(uint flags, uint group, uint local) {
    uint stride = pc.sa != 0u ? pc.sa : pc.n, pitch = stride + 1u;
    uint per = min(32u, WHOLE / pitch), first = group * per;
    uint rows = min(per, pc.m - first);
    uint paired = (pc.n + 1u) & ~1u, pairs = paired / 2u;
    for (uint q = local; q < rows * stride; q += ROW_LANES)
        stage[(q / stride) * pitch + q % stride] = ld_f32(bufA, pc.oa.x, first * stride + q);
    GroupMemoryBarrierWithGroupSync();
    for (uint q2 = local; q2 < rows * pairs; q2 += ROW_LANES) {
        uint r = q2 / pairs, j = (q2 % pairs) * 2u, at = r * pitch + j;
        uint bias = bias_offset(first + r) + j;
        float w0, w1;
        pair_weights(flags, prepared(flags, stage[at], bias), prepared(flags, stage[at + 1u], bias + 1u),
                     w0, w1);
        stage[at] = w0;
        stage[at + 1u] = w1;
    }
    GroupMemoryBarrierWithGroupSync();
    if (local < rows) {
        float total = 0.0;
        uint base = local * pitch;
        for (uint j = 0u; j < paired; j++) total += stage[base + j];   // index order
        precise float reciprocal = 1.0 / half_round(total);
        stage[WHOLE + local] = half_round(reciprocal);
    }
    GroupMemoryBarrierWithGroupSync();
    for (uint q3 = local; q3 < rows * stride; q3 += ROW_LANES) {
        uint r = q3 / stride, j = q3 % stride;
        store(flags, first * stride + q3,
              j < paired ? e4m3(hmul(stage[r * pitch + j], stage[WHOLE + r])) : 0.0);
    }
}

/* Rows wider than shared memory holds whole: one row a lane, each row read twice. */
void softmax_long(uint flags, uint row) {
    uint stride = pc.sa != 0u ? pc.sa : pc.n;
    uint base = row * stride, bias = bias_offset(row);
    for (uint z = pc.n; z < stride; z++) store(flags, base + z, 0.0);
    float total = 0.0;
    float w0, w1;
    for (uint i = 0u; i < pc.n; i += 2u) {
        weights_at(flags, false, base, bias, i, w0, w1);
        total += w0;
        total += w1;
    }
    precise float reciprocal = 1.0 / half_round(total);
    reciprocal = half_round(reciprocal);
    for (uint i2 = 0u; i2 < pc.n; i2 += 2u) {
        weights_at(flags, false, base, bias, i2, w0, w1);
        store(flags, base + i2,      e4m3(hmul(w0, reciprocal)));
        store(flags, base + i2 + 1u, e4m3(hmul(w1, reciprocal)));
    }
}

/* The ViT's exponential (OpenDLSS-NR's `vit_exp_weight`): its own affine, evaluated in
 * half, and a 4-bit shift where the window blocks' is 5 (attention.comp, vit_weight). */
float vit_weight(float logit) {
    precise float affine = half_round(logit) * 0.08953857421875;
    affine += 1.708984375;
    affine = clamp(half_round(affine), 1.439453125, 1.9775390625);
    uint bits = f32tof16(affine) & 0xFFFFu;
    return f16tof32(((bits << 4) + 0x4000u) & 0xFFFFu);
}

/* The ViT's softmax, one row a lane (attention.comp, `vit_softmax`): the weights published
 * unnormalised into c, the row's reciprocal into d for the head merge; the denominator in
 * the vendor's tree order over 64-key blocks padded with zero keys, whose weight is taken
 * off at the end. Columns past the tokens count as padding and publish zero. */
void vit_softmax(uint flags, uint row) {
    uint stride = pc.sa != 0u ? pc.sa : pc.n, tokens = pc.n, base = row * stride;
    uint blocks = (tokens + 63u) / 64u;
    uint A = pc.oa.x;
    float zero = vit_weight(0.0), total = 0.0;
    for (uint b = 0u; b < blocks; b++) {
        float column[8];
        [unroll] for (uint c = 0u; c < 8u; c++) {
            float pair[4];
            [unroll] for (uint j = 0u; j < 4u; j++) {
                uint k0 = b * 64u + c + 16u * j, k1 = k0 + 8u;
                float w0 = k0 < tokens ? vit_weight(ld_f32(bufA, A, base + k0)) : zero;
                float w1 = k1 < tokens ? vit_weight(ld_f32(bufA, A, base + k1)) : zero;
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
    st_f32(bufD, pc.od.x, row, half_round(1.0 / total));
    for (uint k = 0u; k < stride; k++)
        store(flags, base + k, k < tokens ? e4m3(vit_weight(ld_f32(bufA, A, base + k))) : 0.0);
}

[numthreads(ROW_LANES, 1, 1)]
void main(uint3 gid : SV_GroupID, uint lid : SV_GroupIndex) {
    uint group = gid.x + pc.spare;
    uint flags = operation_flags();
    uint kind = flags & 0xFFu;
#if ROW_LANES != 32
    /* the 256-lane build serves the whole-row softmax only (libd3dmx routes nothing else) */
    if (kind == SOFTMAX) softmax_rows(flags, group, lid);
#else
    uint row = group * 32u + lid;
    if (kind == QKV_PREPARE) {
        /* attention.comp's QKV_PREPARE: three independent group planes on y — Q and K
         * normalised (Q scaled), V published — the arithmetic of the two cosine
         * publishes and the V split, recorded as one dispatch. Targets b, c, d. */
        uint part = gid.y;
        uint base = group * 32u * 32u;
        uint count = min(32u * 32u, pc.m * 32u - base);
        gather_part(flags, lid, base, count, part);
        if (part < 2u) {
            if (row < pc.m) cosine_publish(row, lid, part == 0u ? 2u : 0u);
        } else {
            for (uint i = lid; i < count; i += 32u) stage[i] = e4m3(stage[i]);
        }
        scatter_to(flags, lid, base, count, part + 1u);
    } else if (kind == COSINE_PUBLISH) {
        uint base = group * 32u * 32u;
        gather(flags, lid, base, min(32u * 32u, pc.m * 32u - base));
        if (row < pc.m) cosine_publish(row, lid, pc.k != 0u ? 1u : 0u);
        scatter(flags, lid, base, min(32u * 32u, pc.m * 32u - base));
    } else if (kind == VIT_SOFTMAX) {
        if (row < pc.m) vit_softmax(flags, row);
    } else if (kind == SOFTMAX) {
        uint stride = pc.sa != 0u ? pc.sa : pc.n;
        if (stride == pc.n && stride <= 64u) {
            uint base = group * 32u * stride;
            uint count = min(32u * stride, pc.m * stride - base);
            gather(flags, lid, base, count);
            if (row < pc.m) softmax(flags, row, lid);
            scatter(flags, lid, base, count);
        } else if (stride + 1u <= WHOLE) {
            softmax_rows(flags, group, lid);
        } else if (row < pc.m) {
            softmax_long(flags, row);
        }
    }
#endif
}
