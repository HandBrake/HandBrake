/*
 * window_attention_portable — `window_attention_portable.comp` in HLSL: QK^T, the
 * bit-affine softmax and PV for one 8x8 window in one dispatch, the scores and
 * probabilities in groupshared memory. -DMERGED_OUTPUT=1 is the build that also does the
 * head merge (the SPIR-V's specialisation constant 0; DXIL has none at run time).
 *
 * It must equal this backend's three-pass reference bit for bit — the batched
 * `gemm_portable.hlsl` for QK^T (transposed B), the softmax of `attention.hlsl`, the
 * batched `gemm_portable.hlsl` for PV — so the products are the portable GEMM's lane
 * layout and order (row r of the 8-row block, four consecutive columns cq, a float4
 * accumulator taking one K term at a time from zero, `acc += a * b`), and the weights,
 * the sum and the normalise are `attention.hlsl`'s expressions.
 *
 * Operands: u0 Q, u1 K, u3 V (half, (window, head, token, 32)); u2 the output, float32
 * context (batch, 64, 32) or, merged, half E4M3 in (window, token, C) order; u4 the bias
 * (float32, heads x 64 x 64) when pc.flags is 1. pc.n = heads, pc.batch = batches. The
 * batch rides on y and z as in the SPIR-V (y < 65535), one 256-lane group a window-head.
 *
 * As `window_attention.comp` and Metal's kernels, not as the Vulkan portable twin (which
 * measured slower that way on MoltenVK, the one portable Vulkan device measured): the group
 * loads the window's K and V into groupshared memory once, 16 bytes a lane, and each
 * 32-lane slice takes eight query rows. Not yet run on a Direct3D 12 device.
 * A lane's two adjacent keys are both its own, so each pair becomes its weight in
 * registers and only the weights, as halves, are stored; K and V at 4 KB each and a
 * kilobyte of weights a slice make 16 KB, and the row reciprocals take K's first words
 * once every slice is past QK^T. No value and no order of summation changes.
 */
#include "nr_d3d.hlsli"

#ifndef MERGED_OUTPUT
#define MERGED_OUTPUT 0
#endif

/* as words, two halves each: K at 0, V at 1024, slice s's 8 x 64 weights at 2048 + 256 s */
groupshared uint words[4096];
static const uint VALUES = 1024u, WEIGHTS = 2048u;

float lo(uint w) { return f16tof32(w & 0xFFFFu); }
float hi(uint w) { return f16tof32(w >> 16); }
float half_at(uint base, uint i) { uint w = words[base + (i >> 1u)]; return (i & 1u) ? hi(w) : lo(w); }
uint pack2(float a, float b) { return f32tof16(a) | (f32tof16(b) << 16); }

/* attention.hlsl's weights_at_fast on two adjacent logits of one row, the bias from u4 */
void weights(float l0, float l1, uint bias_at, out float w0, out float w1) {
    float logits[2] = { l0, l1 };
    float affine[2];
    [unroll] for (uint j = 0u; j < 2u; j++) {
        float logit = logits[j];
        if (pc.flags != 0u) logit += ld_f32(bufE, pc.oe.x, bias_at + j);
        precise float scaled = half_round(logit) * 0.044921875;
        scaled += 1.30078125;
        affine[j] = clamp(scaled, 1.03125, 1.5693359375);
    }
    uint transformed = (pack2(affine[0], affine[1]) << 5) + 0x7FF88000u;
    w0 = lo(transformed);
    w1 = hi(transformed);
}

[numthreads(256, 1, 1)]
void main(uint3 gid : SV_GroupID, uint lane : SV_GroupIndex) {
    uint batch = gid.y + gid.z * 65535u;
    if (batch >= pc.batch) return;               // uniform across the group
    uint offset = batch * 64u * 32u;
    uint Q = pc.oa.x, K = pc.ob.x, V = pc.od.x;
    {
        uint4 k4 = bufB.Load4(K + offset * 2u + lane * 16u);
        uint4 v4 = bufD.Load4(V + offset * 2u + lane * 16u);
        [unroll] for (uint w = 0u; w < 4u; w++) {
            words[lane * 4u + w] = k4[w];
            words[VALUES + lane * 4u + w] = v4[w];
        }
    }
    GroupMemoryBarrierWithGroupSync();

    uint slice = lane >> 5u, local = lane & 31u;
    uint row = slice * 8u;
    uint mine = WEIGHTS + slice * 256u;
    uint head_bias = (batch % pc.n) * 4096u + row * 64u;
    uint r = local >> 2u, cq = (local & 3u) * 4u;

    /* QK^T as the transposed-B portable GEMM: acc[key] += q[row][c] * k[key][c], c in order */
    {
        float4 acc[4];
        [unroll] for (uint j0 = 0u; j0 < 4u; j0++) acc[j0] = float4(0.0, 0.0, 0.0, 0.0);
        uint qrow = offset + (row + r) * 32u;
        for (uint c = 0u; c < 32u; c++) {
            float4 kv[4];
            [unroll] for (uint j = 0u; j < 4u; j++) {
                uint at = (cq + j * 16u) * 32u + c;
                kv[j] = float4(half_at(0u, at), half_at(0u, at + 32u),
                               half_at(0u, at + 64u), half_at(0u, at + 96u));
            }
            float qv = ld_f16(bufA, Q, qrow + c);
            [unroll] for (uint j1 = 0u; j1 < 4u; j1++)
                acc[j1] += qv * kv[j1];
        }
        /* each pair of adjacent keys becomes its weight here, both being this lane's */
        [unroll] for (uint j2 = 0u; j2 < 4u; j2++)
            [unroll] for (uint e = 0u; e < 4u; e += 2u) {
                uint key = cq + j2 * 16u + e;
                float w0, w1;
                weights(acc[j2][e], acc[j2][e + 1u], head_bias + r * 64u + key, w0, w1);
                words[mine + (r * 64u + key) / 2u] = pack2(w0, w1);   // exact: half values
            }
    }
    GroupMemoryBarrierWithGroupSync();
    /* the row's denominator in float32, in key order, as attention.hlsl sums it; K's first
     * words take the reciprocals, no slice reading K any more */
    if (local < 8u) {
        float total = 0.0;
        for (uint i = 0u; i < 64u; i += 2u) {
            uint two = words[mine + (local * 64u + i) / 2u];
            total += lo(two);
            total += hi(two);
        }
        precise float reciprocal = 1.0 / half_round(total);
        words[row + local] = asuint(half_round(reciprocal));
    }
    GroupMemoryBarrierWithGroupSync();
    for (uint p = local * 2u; p < 8u * 64u; p += 64u) {
        float scale = asfloat(words[row + p / 64u]);
        uint pair = words[mine + p / 2u];
        precise float first = lo(pair) * scale;       // attention.hlsl's hmul
        precise float second = hi(pair) * scale;
        words[mine + p / 2u] = pack2(e4m3(half_round(first)), e4m3(half_round(second)));
    }
    GroupMemoryBarrierWithGroupSync();
    /* PV as the plain portable GEMM: acc[col] += p[row][key] * v[key][col], key in order */
    float4 ctx[2];
    [unroll] for (uint j3 = 0u; j3 < 2u; j3++) ctx[j3] = float4(0.0, 0.0, 0.0, 0.0);
    for (uint key = 0u; key < 64u; key++) {
        float4 vv[2];
        [unroll] for (uint j = 0u; j < 2u; j++) {
            uint at = VALUES + (key * 32u + cq + j * 16u) / 2u;
            uint w0 = words[at], w1 = words[at + 1u];
            vv[j] = float4(lo(w0), hi(w0), lo(w1), hi(w1));
        }
        float pk = half_at(mine, r * 64u + key);
        [unroll] for (uint j4 = 0u; j4 < 2u; j4++)
            ctx[j4] += pk * vv[j4];
    }
    uint C = pc.oc.x;
    [unroll] for (uint j5 = 0u; j5 < 2u; j5++) {
#if MERGED_OUTPUT
        /* merge_heads(..., EPI_E4M3, narrow): E4M3 halves in (window, token, C) order */
        uint target = ((batch / pc.n) * 64u + row + r) * pc.n * 32u
                    + (batch % pc.n) * 32u + cq + j5 * 16u;
        float4 values;
        [unroll] for (uint e = 0u; e < 4u; e++) values[e] = e4m3(ctx[j5][e]);
        st_f16x4(bufC, C + target * 2u, values);
#else
        [unroll] for (uint e = 0u; e < 4u; e++)
            st_f32(bufC, C, offset + (row + r) * 32u + cq + j5 * 16u + e, ctx[j5][e]);
#endif
    }
}
