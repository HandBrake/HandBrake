/*
 * gemm_portable_desc — the descriptor-bound benchmark and test GEMM without a matrix unit:
 * `gemm_portable_desc.comp` (and, with -DBATCHED, its batched build; with -DACC16 the half
 * accumulator of `gemm_coopmat_f16acc.comp`) in HLSL. A (u0) and B (u1) are half, C (u2)
 * float — or half under ACC16 — each from byte 0 of its resource; the push block is the
 * seven words of the SPIR-V plus the group base `libd3dmx` splits a tall dispatch with.
 * FP16 operands, FP32 accumulation in ordinary multiply-adds, an 8x16 block per 32-lane
 * group. On Direct3D 12 this is also what `gemm_coopmat` and `gemm_batched` resolve to.
 */
#define NR_D3D_DESC_PUSH
#include "nr_d3d.hlsli"

struct DescPush { uint M, N, K, sa, sb, sc, bt, base; };
ConstantBuffer<DescPush> dp : register(b0);

static const uint TM = 8, TN = 16;

#ifdef ACC16
typedef float16_t ACC;
typedef float16_t4 ACC4;
#else
typedef float ACC;
typedef float4 ACC4;
#endif

[numthreads(32, 1, 1)]
void main(uint3 gid : SV_GroupID, uint lid : SV_GroupIndex) {
    uint row = (gid.y + dp.base) * TM, col = gid.x * TN;
    if (row >= dp.M || col >= dp.N) return;
#ifdef BATCHED
    uint batch = gid.z;
    uint ao = batch * dp.sa, bo = batch * dp.sb, co = batch * dp.sc;
    bool transposed = dp.bt != 0u;
#else
    uint ao = 0u, bo = 0u, co = 0u;
    bool transposed = false;
#endif
    uint r = lid >> 2u, cq = (lid & 3u) * 4u;
    ACC4 acc = ACC4(0, 0, 0, 0);
    uint arow = ao + (row + r) * dp.K;
    if (transposed) {
        /* B stored (N, K): the key's own layout, read down four rows. */
        uint b0 = bo + (col + cq) * dp.K;
        for (uint k = 0; k < dp.K; k++) {
            ACC4 bv = ACC4(ACC(ld_f16(bufB, 0u, b0 + k)), ACC(ld_f16(bufB, 0u, b0 + dp.K + k)),
                           ACC(ld_f16(bufB, 0u, b0 + 2u * dp.K + k)), ACC(ld_f16(bufB, 0u, b0 + 3u * dp.K + k)));
            acc += ACC(ld_f16(bufA, 0u, arow + k)) * bv;
        }
    } else {
        for (uint k = 0; k < dp.K; k++) {
            uint at = bo + k * dp.N + col + cq;
            ACC4 bv = ACC4(ACC(ld_f16(bufB, 0u, at)), ACC(ld_f16(bufB, 0u, at + 1u)),
                           ACC(ld_f16(bufB, 0u, at + 2u)), ACC(ld_f16(bufB, 0u, at + 3u)));
            acc += ACC(ld_f16(bufA, 0u, arow + k)) * bv;
        }
    }
    uint at = co + (row + r) * dp.N + col + cq;
#ifdef ACC16
    /* the accumulator is already a half: the conversion in st_f16 is exact */
    [unroll] for (uint e = 0; e < 4u; e++) st_f16(bufC, 0u, at + e, (float)acc[e]);
#else
    [unroll] for (uint e = 0; e < 4u; e++) st_f32(bufC, 0u, at + e, acc[e]);
#endif
}
